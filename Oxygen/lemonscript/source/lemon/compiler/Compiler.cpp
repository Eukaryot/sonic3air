/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/Compiler.h"
#include "lemon/compiler/FunctionCompiler.h"
#include "lemon/compiler/Node.h"
#include "lemon/compiler/Parser.h"
#include "lemon/compiler/ParserTokens.h"
#include "lemon/compiler/Preprocessor.h"
#include "lemon/compiler/TokenProcessing.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/compiler/Utility.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"
#include "lemon/translator/Nativizer.h"
#include "lemon/translator/Translator.h"


namespace lemon
{
	namespace
	{
		template<typename T>
		T& addNode(std::vector<BlockNode*>& blockStack, uint32 lineNumber)
		{
			T& node = blockStack.back()->mNodes.createBack<T>();
			node.setLineNumber(lineNumber);
			return node;
		}

		bool isOperator(const Token& token, Operator op)
		{
			return (token.getType() == Token::Type::OPERATOR && token.as<OperatorToken>().mOperator == op);
		}
	}


	struct Compiler::NodesIterator
	{
		BlockNode& mBlockNode;
		size_t mCurrentIndex = 0;

		inline NodesIterator(BlockNode& blockNode) : mBlockNode(blockNode) {}
		inline void operator++()		{ ++mCurrentIndex; }
		inline Node& operator*() const	{ return mBlockNode.mNodes[mCurrentIndex]; }
		inline bool valid() const		{ return (mCurrentIndex < mBlockNode.mNodes.size()); }
		inline Node* peek() const		{ return (mCurrentIndex + 1 < mBlockNode.mNodes.size()) ? &mBlockNode.mNodes[mCurrentIndex + 1] : nullptr; }
		inline void eraseCurrent()		{ mBlockNode.mNodes.erase(mCurrentIndex); --mCurrentIndex; }
	};


	Compiler::LineNumberTranslation::TranslationResult Compiler::LineNumberTranslation::translateLineNumber(uint32 lineNumber) const
	{
		TranslationResult result;
		if (mIntervals.empty())
		{
			CHECK_ERROR_NOLINE(false, "Error resolving line number");
			return result;
		}

		// TODO: Possible optimization with binary search
		size_t index = 0;
		while (index+1 < mIntervals.size() && mIntervals[index+1].mStartLineNumber < lineNumber)
			++index;

		const Interval& interval = mIntervals[index];
		result.mFilename = interval.mFilename;
		result.mLineNumber = lineNumber - interval.mStartLineNumber + interval.mLineOffsetInFile;
		return result;
	}

	void Compiler::LineNumberTranslation::push(uint32 currentLineNumber, const std::wstring& filename, uint32 lineOffsetInFile)
	{
		const bool updateLastEntry = (!mIntervals.empty() && mIntervals.back().mStartLineNumber == currentLineNumber);
		Interval& interval = updateLastEntry ? mIntervals.back() : vectorAdd(mIntervals);
		interval.mStartLineNumber = currentLineNumber;
		interval.mFilename = filename;
		interval.mLineOffsetInFile = lineOffsetInFile;
	}


	Compiler::Compiler(Module& module, GlobalsLookup& globalsLookup) :
		mModule(module),
		mGlobalsLookup(globalsLookup),
		mPreprocessor(*new Preprocessor(mGlobalCompilerConfig))
	{
	}

	Compiler::Compiler(Module& module, GlobalsLookup& globalsLookup, const CompileOptions& compileOptions) :
		mModule(module),
		mGlobalsLookup(globalsLookup),
		mCompileOptions(compileOptions),
		mPreprocessor(*new Preprocessor(mGlobalCompilerConfig))
	{
	}

	Compiler::~Compiler()
	{
		// Free some memory again, by shrinking at least the largest pools
		genericmanager::Manager<Node>::shrinkAllPools();
		genericmanager::Manager<Token>::shrinkAllPools();

		delete &mPreprocessor;
	}

	bool Compiler::loadScript(const std::wstring& path)
	{
		mErrors.clear();
		mModule.startCompiling(mGlobalsLookup);
		mGlobalCompilerConfig.mExternalAddressType = mCompileOptions.mExternalAddressType;

		// Read input file(s)
		std::vector<std::string_view> inputLines;
		bool compileSuccess = loadCodeLines(inputLines, path);
		if (!compileSuccess)
			return false;

		// Compile
		compileSuccess = compileLines(inputLines);
		return compileSuccess;
	}

	bool Compiler::loadCodeLines(std::vector<std::string_view>& outLines, const std::wstring& path)
	{
		// Split into base path and file name
		WString basepath;
		WString filename = path;
		int pos = filename.findChars(L"/\\", filename.length() - 1, -1);
		if (pos > 0)
		{
			basepath = filename.getSubString(0, pos);
			basepath.add('/');
			filename.makeSubString(pos + 1, -1);
		}

		mScriptFiles.clear();
		mScriptFiles.reserve(0x200);

		// Recursively load script files
		if (!loadScriptInternal(*basepath, *filename, outLines, 0))
			return false;

		if (!mCompileOptions.mOutputCombinedSource.empty())
		{
			String output;
			for (const std::string_view& line : outLines)
				output << line << "\r\n";
			output.saveFile(mCompileOptions.mOutputCombinedSource);
		}

		return true;
	}

	bool Compiler::compileLines(const std::vector<std::string_view>& lines)
	{
		try
		{
			// Build a node hierarchy
			BlockNode rootNode;
			compileLinesToNode(rootNode, lines);

			// Process and compile function contents
			for (FunctionNode* node : mFunctionNodes)
			{
				processSingleFunction(*node);
			}

			// Optional translation
			if (!mCompileOptions.mOutputTranslatedSource.empty())
			{
				String output;
				Translator::translateToCpp(output, rootNode);
				output.saveFile(mCompileOptions.mOutputTranslatedSource);
			}

			// Success
			return true;
		}
		catch (const CompilerException& e)
		{
			const auto& translated = mLineNumberTranslation.translateLineNumber(e.mError.mLineNumber);
			ErrorMessage& error = vectorAdd(mErrors);
			error.mMessage = e.what();
			error.mFilename = translated.mFilename;
			error.mError = e.mError;
			error.mError.mLineNumber = translated.mLineNumber + 1;	// Add one because line numbers always start at 1 for user display
		}

		mFunctionNodes.clear();		// All entries got invalid anyway with root node destruction
		return false;
	}

	void Compiler::compileLinesToNode(BlockNode& outNode, const std::vector<std::string_view>& lines)
	{
		// Parse all lines and make nodes out of them
		buildNodesFromCodeLines(outNode, lines);

		// Identify all globals definitions (functions, global variables)
		processGlobalDefinitions(outNode);
	}

	bool Compiler::loadScriptInternal(const std::wstring& basepath, const std::wstring& filename, std::vector<std::string_view>& outLines, int inclusionDepth)
	{
		// Check for cycle in includes
		++inclusionDepth;
		if (inclusionDepth >= 50)
		{
			ErrorMessage& error = vectorAdd(mErrors);
			error.mMessage = "Unusually high recursion depth in lemon script includes while loading script file '" + WString(filename).toStdString() + "' at '" + WString(basepath).toStdString() + "' (possibly some kind of cycle in the includes)";
			return false;
		}

		ScriptFile& scriptFile = mScriptFilesPool.createObject();
		mScriptFiles.push_back(&scriptFile);
		scriptFile.mBasePath = basepath;
		scriptFile.mFilename = filename;
		scriptFile.mFirstLine = outLines.size() + 1;

		if (!scriptFile.mContent.loadFile(basepath + filename))
		{
			ErrorMessage& error = vectorAdd(mErrors);
			error.mMessage = "Failed to load script file '" + WString(filename).toStdString() + "' at '" + WString(basepath).toStdString() + "'";
			return false;
		}

		// Update line number translation
		mLineNumberTranslation.push((uint32)outLines.size() + 1, filename, 0);

		// Split input into lines
		std::vector<std::string_view> fileLines;
		fileLines.reserve((size_t)scriptFile.mContent.length() / 20);	// Rough estimate: At least 20 characters per average line
		{
			int pos = 0;
			while (pos < scriptFile.mContent.length())
			{
				const int start = pos;
				size_t length;
				pos = scriptFile.mContent.getLine(length, start);
				fileLines.emplace_back(&scriptFile.mContent[start], length);
			}
		}

		// Your turn, preprocessor
		try
		{
			mPreprocessor.mPreprocessorDefinitions = &mCompileOptions.mPreprocessorDefinitions;
			mPreprocessor.processLines(fileLines);
		}
		catch (const CompilerException& e)
		{
			ErrorMessage& error = vectorAdd(mErrors);
			error.mMessage = e.what();
			error.mFilename = filename;
			error.mError = e.mError;
			return false;
		}

		// Build output
		for (uint32 fileLineIndex = 0; fileLineIndex < (uint32)fileLines.size(); ++fileLineIndex)
		{
			const std::string_view& line = fileLines[fileLineIndex];

			// Resolve include
			if (line.rfind("include ", 0) == 0)
			{
				String includeFilename = line.substr(8);
				includeFilename.makeSubString(0, includeFilename.findChar(' ', 0, +1));

				// Use only forward slashes
				includeFilename.replace('\\', '/');

				// Split into base path and file name
				String includeBasepath;
				int pos = includeFilename.findChar('/', includeFilename.length()-1, -1);
				if (pos > 0)
				{
					includeBasepath = includeFilename.getSubString(0, pos);
					includeBasepath.add('/');
					includeFilename.makeSubString(pos+1, -1);
				}

				// Wildcard support
				if (includeFilename == "?")
				{
					std::vector<rmx::FileIO::FileEntry> fileEntries;
					fileEntries.reserve(8);
					FTX::FileSystem->listFilesByMask(basepath + *includeBasepath.toWString() + L"*.lemon", false, fileEntries);
					for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
					{
						if (!loadScriptInternal(basepath + *includeBasepath.toWString(), fileEntry.mFilename, outLines, inclusionDepth))
							return false;
					}
				}
				else
				{
					if (!loadScriptInternal(basepath + *includeBasepath.toWString(), *(includeFilename + ".lemon").toWString(), outLines, inclusionDepth))
						return false;
				}

				// Update line number translation
				//  -> Back to this file
				const uint32 currentLineNumber = (uint32)outLines.size() + 1;
				mLineNumberTranslation.push(currentLineNumber, filename, fileLineIndex);
			}
			else
			{
				outLines.emplace_back(std::move(fileLines[fileLineIndex]));
			}
		}

		return true;
	}

	void Compiler::buildNodesFromCodeLines(BlockNode& rootNode, const std::vector<std::string_view>& lines)
	{
		// Parse text lines and build blocks hierarchy
		Parser parser;

		std::vector<BlockNode*> blockStack = { &rootNode };
		uint32 lineNumber = 0;

		for (const std::string_view& line : lines)
		{
			++lineNumber;	// First line will have number 1

			// Parse text line
			ParserTokenList parserTokens;
			parser.splitLineIntoTokens(line, lineNumber, parserTokens);
			if (parserTokens.empty())
				continue;

			// Collect all string literals
			for (size_t i = 0; i < parserTokens.size(); ++i)
			{
				if (parserTokens[i].getType() == ParserToken::Type::STRING_LITERAL)
				{
					mModule.addStringLiteral(parserTokens[i].as<StringLiteralParserToken>().mString);
				}
			}

			// Check for block begin and end
			bool isUndefined = true;
			if (parserTokens[0].getType() == ParserToken::Type::KEYWORD)
			{
				const Keyword keyword = parserTokens[0].as<KeywordParserToken>().mKeyword;
				switch (keyword)
				{
					case Keyword::BLOCK_BEGIN:
					{
						CHECK_ERROR(parserTokens.size() == 1, "Curly brace must use its own line", lineNumber);

						// Start new block
						BlockNode& node = addNode<BlockNode>(blockStack, lineNumber);
						blockStack.push_back(&node);

						isUndefined = false;
						break;
					}

					case Keyword::BLOCK_END:
					{
						CHECK_ERROR(parserTokens.size() == 1, "Curly brace must use its own line", lineNumber);

						// Close block
						blockStack.pop_back();

						CHECK_ERROR(!blockStack.empty(), "Closed too many blocks", lineNumber);

						isUndefined = false;
						break;
					}

					default:
						break;
				}
			}

			// Check for pragma
			if (parserTokens[0].getType() == ParserToken::Type::PRAGMA)
			{
				std::string& content = parserTokens[0].as<PragmaParserToken>().mContent;
				if (!processGlobalPragma(content))
				{
					PragmaNode& node = addNode<PragmaNode>(blockStack, lineNumber);
					node.mContent.swap(content);
				}
				isUndefined = false;
			}

			if (isUndefined)
			{
				// Add undefined node containing the token list, translated from parser token to (compiler) tokens
				UndefinedNode& node = addNode<UndefinedNode>(blockStack, lineNumber);
				node.mTokenList.reserve(parserTokens.size());
				for (size_t i = 0; i < parserTokens.size(); ++i)
				{
					ParserToken& parserToken = parserTokens[i];
					switch (parserToken.getType())
					{
						case ParserToken::Type::KEYWORD:
						{
							node.mTokenList.createBack<KeywordToken>().mKeyword = parserToken.as<KeywordParserToken>().mKeyword;
							break;
						}
						case ParserToken::Type::VARTYPE:
						{
							node.mTokenList.createBack<VarTypeToken>().mDataType = parserToken.as<VarTypeParserToken>().mDataType;
							break;
						}
						case ParserToken::Type::OPERATOR:
						{
							node.mTokenList.createBack<OperatorToken>().mOperator = parserToken.as<OperatorParserToken>().mOperator;
							break;
						}
						case ParserToken::Type::LABEL:
						{
							node.mTokenList.createBack<LabelToken>().mName.swap(parserToken.as<LabelParserToken>().mName);
							break;
						}
						case ParserToken::Type::PRAGMA:
						{
							// Just ignore this one
							break;
						}
						case ParserToken::Type::CONSTANT:
						{
							node.mTokenList.createBack<ConstantToken>().mValue = parserToken.as<ConstantParserToken>().mValue;
							break;
						}
						case ParserToken::Type::STRING_LITERAL:
						{
							const std::string& str = parserToken.as<StringLiteralParserToken>().mString;
							const uint64 hash = rmx::getMurmur2_64(str);
							const StoredString* storedString = mGlobalsLookup.getStringLiteralByHash(hash);
							if (nullptr == storedString)
							{
								// Add as a new string literal
								storedString = mModule.addStringLiteral(str, hash);
								CHECK_ERROR(nullptr != storedString, "Failed to create new string literal, there's possibly too many (more than 65536)", 0);
							}
							ConstantToken& constantToken = node.mTokenList.createBack<ConstantToken>();
							constantToken.mValue = storedString->getHash();
							constantToken.mDataType = &PredefinedDataTypes::STRING;
							break;
						}
						case ParserToken::Type::IDENTIFIER:
						{
							node.mTokenList.createBack<IdentifierToken>().mIdentifier.swap(parserToken.as<IdentifierParserToken>().mIdentifier);
							break;
						}
					}
				}


				static int nodesAdded = 0;
				static int nodesThreshold = 0x1b000;
				static int counters1[64] = { 0 };
				static int counters2[64] = { 0 };
				++counters1[std::min<size_t>(parserTokens.size(), 63)];
				++counters2[std::min<size_t>(node.mTokenList.size(), 63)];
				++nodesAdded;
				if (nodesAdded >= nodesThreshold)
				{
					nodesThreshold += 0x1000;
				}

			}
		}

		CHECK_ERROR(blockStack.size() == 1, "More blocks opened than closed", lineNumber);
	}

	void Compiler::processGlobalDefinitions(BlockNode& rootNode)
	{
		NodeList& nodes = rootNode.mNodes;
		std::vector<PragmaNode*> currentPragmas;
		std::vector<size_t> indicesToErase;
		indicesToErase.reserve(nodes.size() / 2);

		// Cycle through all top-level nodes to find global definitions (functions, global variables, defines)
		for (size_t nodeIndex = 0; nodeIndex < nodes.size(); ++nodeIndex)
		{
			Node& node = nodes[nodeIndex];
			if (node.getType() == Node::Type::PRAGMA)
			{
				currentPragmas.push_back(&node.as<PragmaNode>());
			}
			else if (node.getType() == Node::Type::UNDEFINED)
			{
				TokenList& tokens = node.as<UndefinedNode>().mTokenList;

				// Check for keywords
				if (tokens[0].getType() == Token::Type::KEYWORD)
				{
					switch (tokens[0].as<KeywordToken>().mKeyword)
					{
						case Keyword::FUNCTION:
						{
							// Next node must be a block node
							CHECK_ERROR(nodeIndex+1 < nodes.size(), "Function definition as last node is not allowed", node.getLineNumber());
							CHECK_ERROR(nodes[nodeIndex+1].getType() == Node::Type::BLOCK, "Expected block node after function header", node.getLineNumber());

							// Process tokens
							ScriptFunction& function = processFunctionHeader(node, tokens);

							// Create function node, replacing the undefined node
							FunctionNode& newNode = nodes.createReplaceAt<FunctionNode>(nodeIndex);
							newNode.mFunction = &function;
							newNode.mContent = nodes[nodeIndex+1].as<BlockNode>();
							newNode.setLineNumber(node.getLineNumber());

							mFunctionNodes.push_back(&newNode);

							// Erase block node pointer
							//  -> For performance reasons, don't erase it here already, but all of these on one go later on
							indicesToErase.push_back(nodeIndex + 1);
							++nodeIndex;	// Skip next index

							// Add all pragmas associated with this function, i.e. all pragma nodes in front
							for (PragmaNode* pragmaNode : currentPragmas)
							{
								function.mPragmas.push_back(pragmaNode->mContent);
							}
							break;
						}

						case Keyword::GLOBAL:
						{
							size_t offset = 1;
							CHECK_ERROR(offset < tokens.size() && tokens[offset].getType() == Token::Type::VARTYPE, "Expected a typename after 'global' keyword", node.getLineNumber());
							const DataTypeDefinition* dataType = tokens[offset].as<VarTypeToken>().mDataType;
							++offset;

							CHECK_ERROR(offset < tokens.size() && tokens[offset].getType() == Token::Type::IDENTIFIER, "Expected an identifier in global variable definition", node.getLineNumber());
							const std::string identifier = tokens[offset].as<IdentifierToken>().mIdentifier;
							++offset;

							// Create global variable
							GlobalVariable& variable = mModule.addGlobalVariable(identifier, dataType);
							mGlobalsLookup.registerVariable(variable);

							if (offset+2 <= tokens.size() && isOperator(tokens[offset], Operator::ASSIGN))
							{
								CHECK_ERROR(offset+2 == tokens.size() && tokens[offset+1].getType() == Token::Type::CONSTANT, "Expected a constant value for initializing the global variable", node.getLineNumber());
								variable.mInitialValue = tokens[offset+1].as<ConstantToken>().mValue;
							}
							break;
						}

						case Keyword::CONSTANT:
						{
							CHECK_ERROR(tokens.size() >= 5, "Syntax error in constant definition", node.getLineNumber());
							CHECK_ERROR(tokens[1].getType() == Token::Type::VARTYPE, "Expected a type in constant definition", node.getLineNumber());
							CHECK_ERROR(tokens[2].getType() == Token::Type::IDENTIFIER, "Expected an identifier for constant definition", node.getLineNumber());
							CHECK_ERROR(isOperator(tokens[3], Operator::ASSIGN), "Missing assignment in constant definition", node.getLineNumber());

							// TODO: Support all statements that result in a compile-time constant
							CHECK_ERROR(tokens[4].getType() == Token::Type::CONSTANT, "", node.getLineNumber());

							const DataTypeDefinition* dataType = tokens[1].as<VarTypeToken>().mDataType;
							const std::string identifier = tokens[2].as<IdentifierToken>().mIdentifier;
							const uint64 constantValue = tokens[4].as<ConstantToken>().mValue;

							Constant& constant = mModule.addConstant(identifier, dataType, constantValue);
							mGlobalsLookup.registerConstant(constant);
							break;
						}

						case Keyword::DEFINE:
						{
							size_t offset = 1;
							const DataTypeDefinition* dataType = nullptr;	// Not specified

							// Typename is optional
							if (offset < tokens.size() && tokens[offset].getType() == Token::Type::VARTYPE)
							{
								dataType = tokens[offset].as<VarTypeToken>().mDataType;
								++offset;
							}

							CHECK_ERROR(offset < tokens.size() && tokens[offset].getType() == Token::Type::IDENTIFIER, "Expected an identifier for define", node.getLineNumber());
							const std::string identifier = tokens[offset].as<IdentifierToken>().mIdentifier;
							++offset;

							CHECK_ERROR(offset < tokens.size() && isOperator(tokens[offset], Operator::ASSIGN), "Expected '=' in define", node.getLineNumber());
							++offset;

							// Rest is define content
							CHECK_ERROR(offset < tokens.size(), "Missing define content", node.getLineNumber());

							// Find out the data type if not specified yet
							if (nullptr == dataType)
							{
								if (tokens[offset].getType() == Token::Type::VARTYPE)
								{
									dataType = tokens[offset].as<VarTypeToken>().mDataType;
								}
								else
								{
									CHECK_ERROR(false, "Data type of define could not be determined", node.getLineNumber());
								}
							}

							// Create define
							Define& define = mModule.addDefine(identifier, dataType);
							mGlobalsLookup.registerDefine(define);
							for (size_t i = offset; i < tokens.size(); ++i)
							{
								define.mContent.add(tokens[i]);
							}
							break;
						}

						case Keyword::DECLARE:
						{
							// Completely ignore this line, we don't evaluate declarations at all (yet)
							// TODO: However, it could make sense to check them just to see if there's a matching function definition at all
							break;
						}

						default:
							break;
					}
				}

				currentPragmas.clear();
			}
		}

		nodes.erase(indicesToErase);
	}

	ScriptFunction& Compiler::processFunctionHeader(Node& node, const TokenList& tokens)
	{
		const uint32 lineNumber = node.getLineNumber();

		size_t offset = 1;
		CHECK_ERROR(offset < tokens.size() && tokens[offset].getType() == Token::Type::VARTYPE, "Expected a typename after 'function' keyword", lineNumber);
		const DataTypeDefinition* returnType = tokens[offset].as<VarTypeToken>().mDataType;

		++offset;
		CHECK_ERROR(offset < tokens.size() && tokens[offset].getType() == Token::Type::IDENTIFIER, "Expected an identifier in function definition", lineNumber);
		const std::string functionName = tokens[offset].as<IdentifierToken>().mIdentifier;

		++offset;
		CHECK_ERROR(offset < tokens.size() && isOperator(tokens[offset], Operator::PARENTHESIS_LEFT), "Expected opening parentheses in function definition", lineNumber);

		++offset;
		CHECK_ERROR(offset < tokens.size(), "Unexpected end of function definition", lineNumber);

		std::vector<Function::Parameter> parameters;
		if (tokens[offset].getType() == Token::Type::OPERATOR)
		{
			CHECK_ERROR(tokens[offset].as<OperatorToken>().mOperator == Operator::PARENTHESIS_RIGHT, "Expected closing parentheses or parameter definition", lineNumber);
		}
		else
		{
			// Here come the parameters
			while (true)
			{
				// Must be a type and identifier as next tokens, then a comma or closing parentheses
				CHECK_ERROR(offset + 2 < tokens.size(), "Expected function parameter definition", lineNumber);
				parameters.emplace_back();

				CHECK_ERROR(tokens[offset].getType() == Token::Type::VARTYPE, "Expected type in function parameter definition", lineNumber);
				parameters.back().mType = tokens[offset].as<VarTypeToken>().mDataType;

				++offset;
				CHECK_ERROR(tokens[offset].getType() == Token::Type::IDENTIFIER, "Expected identifier in function parameter definition", lineNumber);
				parameters.back().mIdentifier = tokens[offset].as<IdentifierToken>().mIdentifier;

				++offset;
				CHECK_ERROR(tokens[offset].getType() == Token::Type::OPERATOR, "Expected comma or closing parentheses after function parameter definition", lineNumber);
				const Operator op = tokens[offset].as<OperatorToken>().mOperator;
				if (op == Operator::PARENTHESIS_RIGHT)
				{
					// We're done!
					break;
				}

				CHECK_ERROR(op == Operator::COMMA_SEPARATOR, "Expected comma or closing parentheses after function parameter definition", lineNumber);
				++offset;
			}
		}

		// Create function in program
		ScriptFunction& function = mModule.addScriptFunction(functionName, returnType, &parameters);
		mGlobalsLookup.registerFunction(function);

		// Create new variables for parameters
		for (const Function::Parameter& parameter : function.getParameters())
		{
			CHECK_ERROR(nullptr == function.getLocalVariableByIdentifier(parameter.mIdentifier), "Parameter name already used", lineNumber);
			function.addLocalVariable(parameter.mIdentifier, parameter.mType, lineNumber);
		}

		// Set source metadata
		const auto& translated = mLineNumberTranslation.translateLineNumber(lineNumber);
		function.mSourceFilename = translated.mFilename;
		function.mSourceBaseLineOffset = lineNumber - translated.mLineNumber;

		return function;
	}

	void Compiler::processSingleFunction(FunctionNode& functionNode)
	{
		BlockNode& content = *functionNode.mContent;
		ScriptFunction& function = *functionNode.mFunction;
		function.mStartLineNumber = functionNode.getLineNumber();

		// Build scope context for processing
		ScopeContext scopeContext;
		for (LocalVariable* localVariable : function.mLocalVariablesById)
		{
			// All local variables so far have to be parameters; add each to the scope
			scopeContext.mLocalVariables.push_back(localVariable);
		}

		// Processing
		processUndefinedNodesInBlock(content, function, scopeContext);

		// Build opcodes out of nodes inside the function's block
		FunctionCompiler functionCompiler(function, mGlobalCompilerConfig);
		functionCompiler.processParameters();
		functionCompiler.buildOpcodesForFunction(content);
	}

	void Compiler::processUndefinedNodesInBlock(BlockNode& blockNode, ScriptFunction& function, ScopeContext& scopeContext)
	{
		// Block start: Create new scope
		scopeContext.beginScope();

		for (NodesIterator nodesIterator(blockNode); nodesIterator.valid(); ++nodesIterator)
		{
			Node& node = *nodesIterator;
			const size_t nodeIndex = nodesIterator.mCurrentIndex;
			switch (node.getType())
			{
				case Node::Type::BLOCK:
				{
					processUndefinedNodesInBlock(node.as<BlockNode>(), function, scopeContext);
					break;
				}

				case Node::Type::UNDEFINED:
				{
					UndefinedNode& un = node.as<UndefinedNode>();
					Node* newNode = processUndefinedNode(un, function, scopeContext, nodesIterator);
					if (newNode != nullptr)
					{
						newNode->setLineNumber(node.getLineNumber());

						// Replace undefined node
						blockNode.mNodes.replace(*newNode, nodeIndex);
					}
					break;
				}

				default:
					break;
			}
		}

		// Block end: Close scope
		scopeContext.endScope();
	}

	Node* Compiler::processUndefinedNode(UndefinedNode& undefinedNode, ScriptFunction& function, ScopeContext& scopeContext, NodesIterator& nodesIterator)
	{
		const uint32 lineNumber = undefinedNode.getLineNumber();
		TokenList& tokens = undefinedNode.mTokenList;

		if (tokens[0].getType() == Token::Type::KEYWORD)
		{
			// Check for keywords
			const Keyword keyword = tokens[0].as<KeywordToken>().mKeyword;
			switch (keyword)
			{
				case Keyword::RETURN:
				{
					// Process tokens
					const TokenProcessing::Context context(mGlobalsLookup, scopeContext.mLocalVariables, &function);
					TokenProcessing tokenProcessing(context, mGlobalCompilerConfig);
					tokenProcessing.processTokens(tokens, lineNumber);

					if (tokens.size() > 1)
					{
						CHECK_ERROR(tokens.size() <= 2, "Return can have up to one statement", lineNumber);
						CHECK_ERROR(tokens[1].isStatement(), "Token after 'return' must be a statement", lineNumber);
					}

					// Note that return type is not known here yet
					ReturnNode& node = NodeFactory::create<ReturnNode>();
					node.setLineNumber(lineNumber);

					if (tokens.size() > 1)
					{
						node.mStatementToken = tokens[1].as<StatementToken>();
						tokens.erase(1);
					}
					return &node;
				}

				case Keyword::CALL:
				case Keyword::JUMP:
				{
					// Process tokens
					processTokens(tokens, function, scopeContext, lineNumber, mCompileOptions.mExternalAddressType);

					CHECK_ERROR(tokens.size() == 2, "'call' and 'jump' need an additional token after them", lineNumber);
					if (tokens[1].isStatement())
					{
						// Note that argument type is not known here yet
						ExternalNode& node = NodeFactory::create<ExternalNode>();
						node.mStatementToken = tokens[1].as<StatementToken>();
						node.mSubType = (keyword == Keyword::CALL) ? ExternalNode::SubType::EXTERNAL_CALL : ExternalNode::SubType::EXTERNAL_JUMP;
						node.setLineNumber(lineNumber);
						tokens.erase(1);
						return &node;
					}
					else if (tokens[1].getType() == Token::Type::LABEL)
					{
						CHECK_ERROR(keyword == Keyword::JUMP, "Label is not allowed after 'call' keyword", lineNumber);

						JumpNode& node = NodeFactory::create<JumpNode>();
						node.mLabelToken = tokens[1].as<LabelToken>();
						node.setLineNumber(lineNumber);
						tokens.erase(1);
						return &node;
					}

					CHECK_ERROR(false, "Token after 'call' and 'jump' must be a statement or a label", lineNumber);
					return nullptr;
				}

				case Keyword::BREAK:
				{
					CHECK_ERROR(tokens.size() == 1, "There must be no token after 'break' keyword", lineNumber);
					BreakNode& newNode = NodeFactory::create<BreakNode>();
					newNode.setLineNumber(lineNumber);
					return &newNode;
				}

				case Keyword::CONTINUE:
				{
					CHECK_ERROR(tokens.size() == 1, "There must be no token after 'continue' keyword", lineNumber);
					ContinueNode& newNode = NodeFactory::create<ContinueNode>();
					newNode.setLineNumber(lineNumber);
					return &newNode;
				}

				case Keyword::IF:
				{
					if (mGlobalCompilerConfig.mScriptFeatureLevel >= 2)
						CHECK_ERROR(tokens.size() >= 2 && isOperator(tokens[1], Operator::PARENTHESIS_LEFT), "Expected parentheses after 'if' keyword", lineNumber);

					// Process tokens
					processTokens(tokens, function, scopeContext, lineNumber);

					CHECK_ERROR(tokens.size() == 2, "Expected single statement after 'if' keyword", lineNumber);
					CHECK_ERROR(tokens[1].isStatement(), "Expected statement after 'if' keyword", lineNumber);

					IfStatementNode& node = NodeFactory::create<IfStatementNode>();
					node.mConditionToken = tokens[1].as<StatementToken>();
					node.setLineNumber(lineNumber);
					tokens.erase(1);

					// Go on with the next node, which must be either a block or a statement
					{
						Node* newNode = gatherNextStatement(nodesIterator, function, scopeContext);
						CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'if' line", node.getLineNumber());
						newNode->setLineNumber(node.getLineNumber());
						node.mContentIf = newNode;
						nodesIterator.eraseCurrent();
					}

					// Also check for 'else'
					{
						Node* nextNode = nodesIterator.peek();
						if (nullptr != nextNode && nextNode->getType() == Node::Type::UNDEFINED)
						{
							UndefinedNode& nextNodeUndefined = nextNode->as<UndefinedNode>();
							if (nextNodeUndefined.mTokenList.size() >= 1 && nextNodeUndefined.mTokenList[0].getType() == Token::Type::KEYWORD && nextNodeUndefined.mTokenList[0].as<KeywordToken>().mKeyword == Keyword::ELSE)
							{
								// Special case: 'else if' (or anything where there's a statement directly after the 'else')
								if (nextNodeUndefined.mTokenList.size() >= 2)
								{
									// Remove the 'else' here and treat the rest as a normal statement, as if it was on the next line
									nextNodeUndefined.mTokenList.erase(0);
								}
								else
								{
									// Normal handling for 'else'
									++nodesIterator;
									nodesIterator.eraseCurrent();
								}

								Node* newNode = gatherNextStatement(nodesIterator, function, scopeContext);
								CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'else' line", node.getLineNumber());
								newNode->setLineNumber(node.getLineNumber());
								node.mContentElse = newNode;
								nodesIterator.eraseCurrent();
							}
						}
					}

					return &node;
				}

				case Keyword::ELSE:
				{
					CHECK_ERROR(false, "Found 'else' without a corresponding 'if'", lineNumber);
					break;
				}

				case Keyword::WHILE:
				{
					// Process tokens
					processTokens(tokens, function, scopeContext, lineNumber);

					CHECK_ERROR(tokens.size() == 2, "Expected single statement after 'while' keyword", lineNumber);
					CHECK_ERROR(tokens[1].isStatement(), "Expected statement after 'while' keyword", lineNumber);

					WhileStatementNode& node = NodeFactory::create<WhileStatementNode>();
					node.mConditionToken = tokens[1].as<StatementToken>();
					node.setLineNumber(lineNumber);
					tokens.erase(1);

					// Go on with the next node, which must be either a block or a statement
					{
						Node* newNode = gatherNextStatement(nodesIterator, function, scopeContext);
						CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'while' line", node.getLineNumber());
						newNode->setLineNumber(node.getLineNumber());
						node.mContent = newNode;
						nodesIterator.eraseCurrent();
					}
					return &node;
				}

				case Keyword::FOR:
				{
					// Check for braces
					CHECK_ERROR(tokens.size() >= 3, "Not enough tokens found after 'for' keyword", lineNumber);
					CHECK_ERROR(isOperator(tokens[1], Operator::PARENTHESIS_LEFT), "Expected opening parenthesis after 'for' keyword", lineNumber);
					CHECK_ERROR(isOperator(tokens.back(), Operator::PARENTHESIS_RIGHT), "Expected closing parenthesis as last token after 'for' keyword", lineNumber);

					// Split by semicolons
					const size_t firstIndex = 2;
					const size_t endIndex = tokens.size() - 1;
					size_t numSemicolons = 0;
					size_t splitPosition[4] = { 1, 0, 0, endIndex };
					for (size_t i = firstIndex; i < endIndex; ++i)
					{
						if (isOperator(tokens[i], Operator::SEMICOLON_SEPARATOR))
						{
							++numSemicolons;
							if (numSemicolons <= 2)
							{
								splitPosition[numSemicolons] = i;
							}
						}
					}
					CHECK_ERROR(numSemicolons == 2, "Expected exactly two semicolons in 'for' loop header", lineNumber);

					// Create new scope, should end after the next node (counting both this and the next one)
					scopeContext.beginScope();

					TokenPtr<StatementToken> statements[3];
					for (int i = 0; i < 3; ++i)
					{
						const size_t numTokens = splitPosition[i+1] - splitPosition[i] - 1;
						if (numTokens > 0)
						{
							TokenList innerTokenList;
							for (size_t k = splitPosition[i] + 1; k < splitPosition[i+1]; ++k)
							{
								innerTokenList.add(tokens[k]);
							}

							// Process tokens
							processTokens(innerTokenList, function, scopeContext, lineNumber);

							CHECK_ERROR(innerTokenList.size() == 1, "Tokens in 'for' loop header do not evaluate to a single statement", lineNumber);
							CHECK_ERROR(innerTokenList[0].isStatement(), "Tokens in 'for' loop header do not evaluate to a statement", lineNumber);

							statements[i] = innerTokenList[0].as<StatementToken>();
						}
					}

					ForStatementNode& node = NodeFactory::create<ForStatementNode>();
					node.mInitialToken   = statements[0];
					node.mConditionToken = statements[1];
					node.mIterationToken = statements[2];
					node.setLineNumber(lineNumber);

					// Go on with the next node, which must be either a block or a statement
					{
						Node* newNode = gatherNextStatement(nodesIterator, function, scopeContext);
						CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'for' line", node.getLineNumber());
						newNode->setLineNumber(node.getLineNumber());
						node.mContent = newNode;
						nodesIterator.eraseCurrent();
					}

					scopeContext.endScope();
					return &node;
				}

				default:
					break;
			}
		}
		else if (tokens[0].getType() == Token::Type::LABEL)
		{
			// Process label definition
			CHECK_ERROR(tokens.size() == 2, "Expected only colon after label", lineNumber);
			CHECK_ERROR(isOperator(tokens[1], Operator::COLON), "Expected a colon operator after label", lineNumber);

			LabelNode& node = NodeFactory::create<LabelNode>();
			node.mLabel = tokens[0].as<LabelToken>().mName;
			node.setLineNumber(lineNumber);
			return &node;
		}
		else
		{
			// Process tokens
			processTokens(tokens, function, scopeContext, lineNumber);

			// Evaluate processed token tree
			CHECK_ERROR(tokens.size() == 1, "Statement contains more than a single token tree root", lineNumber);
			CHECK_ERROR(tokens[0].isStatement(), "Statement is no statement?", lineNumber);

			StatementNode& node = NodeFactory::create<StatementNode>();
			node.mStatementToken = tokens[0].as<StatementToken>();
			node.setLineNumber(lineNumber);
			tokens.erase(0);
			return &node;
		}

		return nullptr;
	}

	Node* Compiler::gatherNextStatement(NodesIterator& nodesIterator, ScriptFunction& function, ScopeContext& scopeContext)
	{
		++nodesIterator;
		if (nodesIterator.valid())
		{
			Node& nextNode = *nodesIterator;
			switch (nextNode.getType())
			{
				case Node::Type::BLOCK:
				{
					processUndefinedNodesInBlock(nextNode.as<BlockNode>(), function, scopeContext);
					return &nextNode;
				}

				case Node::Type::UNDEFINED:
				{
					UndefinedNode& un = nextNode.as<UndefinedNode>();
					Node* newNode = processUndefinedNode(un, function, scopeContext, nodesIterator);
					newNode->setLineNumber(un.getLineNumber());
					return newNode;
				}

				default:
					break;
			}
		}
		return nullptr;
	}

	void Compiler::processTokens(TokenList& tokens, ScriptFunction& function, ScopeContext& scopeContext, uint32 lineNumber, const DataTypeDefinition* resultType)
	{
		const TokenProcessing::Context context(mGlobalsLookup, scopeContext.mLocalVariables, &function);
		TokenProcessing tokenProcessing(context, mGlobalCompilerConfig);
		tokenProcessing.processTokens(tokens, lineNumber, resultType);
	}

	bool Compiler::processGlobalPragma(const std::string& content)
	{
		static const std::string PREFIX_FEATURE_LEVEL = "script-feature-level(";
		String str(content);
		str.trimWhitespace();
		if (str.startsWith(PREFIX_FEATURE_LEVEL.c_str()) && str.endsWith(")"))
		{
			str.remove(0, (int)PREFIX_FEATURE_LEVEL.size());
			str.remove(str.length() - 1, 1);
			const uint64 value = rmx::parseInteger(str);
			if (value > 0)
			{
				// Don't allow a higher script feature level than actually supported
				const constexpr uint32 MAX_SCRIPT_FEATURE_LEVEL = 2;
				if (value > MAX_SCRIPT_FEATURE_LEVEL)
				{
					REPORT_ERROR_CODE(CompilerError::Code::SCRIPT_FEATURE_LEVEL_TOO_HIGH, value, MAX_SCRIPT_FEATURE_LEVEL, "Script uses feature level " << value << ", but the highest supported level is " << MAX_SCRIPT_FEATURE_LEVEL);
				}
				mGlobalCompilerConfig.mScriptFeatureLevel = (uint32)value;
			}
			return true;
		}
		return false;
	}

}
