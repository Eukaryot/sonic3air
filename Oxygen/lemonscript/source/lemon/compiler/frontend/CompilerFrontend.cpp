/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/frontend/CompilerFrontend.h"
#include "lemon/compiler/LineNumberTranslation.h"
#include "lemon/compiler/Node.h"
#include "lemon/compiler/TokenHelper.h"
#include "lemon/compiler/TokenTypes.h"
#include "lemon/compiler/Utility.h"
#include "lemon/compiler/parser/Parser.h"
#include "lemon/compiler/parser/ParserTokens.h"
#include "lemon/program/GlobalsLookup.h"
#include "lemon/program/Module.h"


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
	}


	struct CompilerFrontend::NodesIterator
	{
		BlockNode& mBlockNode;
		size_t mCurrentIndex = 0;
		std::vector<size_t> mIndicesToErase;

		inline NodesIterator(BlockNode& blockNode) :
			mBlockNode(blockNode)
		{}

		inline ~NodesIterator()
		{
			mBlockNode.mNodes.erase(mIndicesToErase);
		}

		inline void operator++()		{ ++mCurrentIndex; }
		inline Node& operator*() const	{ return mBlockNode.mNodes[mCurrentIndex]; }
		inline Node* operator->() const	{ return &mBlockNode.mNodes[mCurrentIndex]; }
		inline bool valid() const		{ return (mCurrentIndex < mBlockNode.mNodes.size()); }
		inline Node* get() const		{ return (mCurrentIndex < mBlockNode.mNodes.size()) ? &mBlockNode.mNodes[mCurrentIndex] : nullptr; }
		inline Node* peek() const		{ return (mCurrentIndex + 1 < mBlockNode.mNodes.size()) ? &mBlockNode.mNodes[mCurrentIndex + 1] : nullptr; }

		template<typename T> inline T* getSpecific() const   { Node* node = get();  return (nullptr != node && node->isA<T>()) ? static_cast<T*>(node) : nullptr; }
		template<typename T> inline T* peekSpecific() const  { Node* node = peek(); return (nullptr != node && node->isA<T>()) ? static_cast<T*>(node) : nullptr; }

		inline void eraseCurrent()
		{
			// For performance reasons, don't erase it here already, but all of these in one go later on
			if (mIndicesToErase.empty())
				mIndicesToErase.reserve(mBlockNode.mNodes.size() / 2);
			mIndicesToErase.push_back(mCurrentIndex);
		}
	};


	CompilerFrontend::CompilerFrontend(Module& module, GlobalsLookup& globalsLookup, CompileOptions& compileOptions, const LineNumberTranslation& lineNumberTranslation, std::vector<FunctionNode*>& functionNodes) :
		mModule(module),
		mGlobalsLookup(globalsLookup),
		mCompileOptions(compileOptions),
		mLineNumberTranslation(lineNumberTranslation),
		mTokenProcessing(globalsLookup, compileOptions),
		mFunctionNodes(functionNodes)
	{
	}

	void CompilerFrontend::runCompilerFrontend(BlockNode& outRootNode, const std::vector<std::string_view>& lines)
	{
		// Parse all lines and make nodes out of them that form a node hierarchy
		buildNodesFromCodeLines(outRootNode, lines);

		// Identify all globals definitions (functions, global variables, constants, defines)
		processGlobalDefinitions(outRootNode);

		// Process function contents
		for (FunctionNode* node : mFunctionNodes)
		{
			processSingleFunction(*node);
		}
	}

	void CompilerFrontend::buildNodesFromCodeLines(BlockNode& rootNode, const std::vector<std::string_view>& lines)
	{
		// Parse text lines and build blocks hierarchy
		Parser parser;
		ParserTokenList parserTokens;

		std::vector<BlockNode*> blockStack = { &rootNode };
		uint32 lineNumber = 0;

		for (const std::string_view line : lines)
		{
			++lineNumber;	// First line will have number 1

			// Parse text line
			parserTokens.clear();
			parser.splitLineIntoTokens(line, lineNumber, parserTokens);
			if (parserTokens.empty())
				continue;

			// Check for block begin and end
			bool isUndefined = true;
			if (parserTokens[0].isA<KeywordParserToken>())
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
			if (parserTokens[0].isA<PragmaParserToken>())
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
							node.mTokenList.createBack<LabelToken>().mName = parserToken.as<LabelParserToken>().mName;
							break;
						}

						case ParserToken::Type::PRAGMA:
						{
							// Just ignore this one
							break;
						}

						case ParserToken::Type::CONSTANT:
						{
							const ConstantParserToken& input = parserToken.as<ConstantParserToken>();
							ConstantToken& constantToken = node.mTokenList.createBack<ConstantToken>();
							constantToken.mValue = input.mValue;
							constantToken.mDataType = (input.mBaseType == BaseType::FLOAT)  ? static_cast<const DataTypeDefinition*>(&PredefinedDataTypes::FLOAT) :
													  (input.mBaseType == BaseType::DOUBLE) ? static_cast<const DataTypeDefinition*>(&PredefinedDataTypes::DOUBLE) : static_cast<const DataTypeDefinition*>(&PredefinedDataTypes::CONST_INT);
							break;
						}

						case ParserToken::Type::STRING_LITERAL:
						{
							const FlyweightString str = parserToken.as<StringLiteralParserToken>().mString;
							const FlyweightString* existingString = mGlobalsLookup.getStringLiteralByHash(str.getHash());
							if (nullptr == existingString)
							{
								// Add as a new string literal to the module
								mModule.addStringLiteral(str);
							}
							ConstantToken& constantToken = node.mTokenList.createBack<ConstantToken>();
							constantToken.mValue.set(str.getHash());
							constantToken.mDataType = &PredefinedDataTypes::STRING;
							break;
						}

						case ParserToken::Type::IDENTIFIER:
						{
							IdentifierToken& token = node.mTokenList.createBack<IdentifierToken>();
							token.mName = parserToken.as<IdentifierParserToken>().mName;
							break;
						}
					}
				}
			}
		}

		CHECK_ERROR(blockStack.size() == 1, "More blocks opened than closed", lineNumber);
	}

	void CompilerFrontend::processGlobalDefinitions(BlockNode& rootNode)
	{
		NodeList& nodes = rootNode.mNodes;
		std::vector<PragmaNode*> currentPragmas;

		// Cycle through all top-level nodes to find global definitions (functions, global variables, defines)
		for (NodesIterator nodesIterator(rootNode); nodesIterator.valid(); ++nodesIterator)
		{
			Node& node = *nodesIterator;
			if (node.isA<PragmaNode>())
			{
				currentPragmas.push_back(&node.as<PragmaNode>());
			}
			else if (node.isA<UndefinedNode>())
			{
				TokenList& tokens = node.as<UndefinedNode>().mTokenList;

				// Check for keywords
				if (tokens[0].isA<KeywordToken>())
				{
					switch (tokens[0].as<KeywordToken>().mKeyword)
					{
						case Keyword::FUNCTION:
						{
							// Next node must be a block node
							const size_t nodeIndex = nodesIterator.mCurrentIndex;
							CHECK_ERROR(nodeIndex+1 < nodes.size(), "Function definition as last node is not allowed", node.getLineNumber());
							CHECK_ERROR(nodes[nodeIndex+1].isA<BlockNode>(), "Expected block node after function header", node.getLineNumber());

							// Process tokens
							ScriptFunction& function = processFunctionHeader(node, tokens);

							// Create function node, replacing the undefined node
							FunctionNode& newNode = nodes.createReplaceAt<FunctionNode>(nodeIndex);
							newNode.mFunction = &function;
							newNode.mContent = nodes[nodeIndex+1].as<BlockNode>();
							newNode.setLineNumber(node.getLineNumber());

							mFunctionNodes.push_back(&newNode);

							// Erase block node pointer
							++nodesIterator;
							nodesIterator.eraseCurrent();

							// Add all pragmas associated with this function, i.e. all pragma nodes in front
							for (PragmaNode* pragmaNode : currentPragmas)
							{
								function.addOrProcessPragma(pragmaNode->mContent, mCompileOptions.mConsumeProcessedPragmas);
							}

							// Now register the function
							mGlobalsLookup.registerFunction(function);
							break;
						}

						case Keyword::GLOBAL:
						{
							size_t offset = 1;
							CHECK_ERROR(offset < tokens.size() && tokens[offset].isA<VarTypeToken>(), "Expected a typename after 'global' keyword", node.getLineNumber());
							const DataTypeDefinition* dataType = tokens[offset].as<VarTypeToken>().mDataType;
							++offset;

							CHECK_ERROR(offset < tokens.size() && tokens[offset].isA<IdentifierToken>(), "Expected an identifier in global variable definition", node.getLineNumber());
							const FlyweightString identifier = tokens[offset].as<IdentifierToken>().mName;
							++offset;

							// Create global variable
							GlobalVariable& variable = mModule.addGlobalVariable(identifier, dataType);
							mGlobalsLookup.registerGlobalVariable(variable);

							if (offset+2 <= tokens.size() && isOperator(tokens[offset], Operator::ASSIGN))
							{
								CHECK_ERROR(offset+2 == tokens.size() && tokens[offset+1].isA<ConstantToken>(), "Expected a constant value for initializing the global variable", node.getLineNumber());
								variable.mInitialValue = tokens[offset+1].as<ConstantToken>().mValue.get<int64>();
							}
							break;
						}

						case Keyword::CONSTANT:
						{
							processConstantDefinition(tokens, nodesIterator, nullptr);
							break;
						}

						case Keyword::DEFINE:
						{
							size_t offset = 1;
							const DataTypeDefinition* dataType = nullptr;	// Not specified

							// Typename is optional
							if (offset < tokens.size() && tokens[offset].isA<VarTypeToken>())
							{
								dataType = tokens[offset].as<VarTypeToken>().mDataType;
								++offset;
							}

							CHECK_ERROR(offset < tokens.size() && tokens[offset].isA<IdentifierToken>(), "Expected an identifier for define", node.getLineNumber());
							const FlyweightString identifier = tokens[offset].as<IdentifierToken>().mName;
							++offset;

							CHECK_ERROR(offset < tokens.size() && isOperator(tokens[offset], Operator::ASSIGN), "Expected '=' in define", node.getLineNumber());
							++offset;

							// Rest is define content
							CHECK_ERROR(offset < tokens.size(), "Missing define content", node.getLineNumber());

							// Find out the data type if not specified yet
							if (nullptr == dataType)
							{
								if (tokens[offset].isA<VarTypeToken>())
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

		// Do some post-processing on the defines, to resolve situations where one define uses another one
		for (Define* define : mModule.getDefines())
		{
			for (int iterationDepth = 0; ; ++iterationDepth)
			{
				if (!mTokenProcessing.resolveIdentifiers(define->mContent))
					break;
				CHECK_ERROR(iterationDepth < 10, "Too deep recursion in evaluating define '" << define->getName() << "'", 0);
			}
		}
	}

	ScriptFunction& CompilerFrontend::processFunctionHeader(Node& node, const TokenList& tokens)
	{
		const uint32 lineNumber = node.getLineNumber();

		size_t offset = 1;
		CHECK_ERROR(offset < tokens.size() && tokens[offset].isA<VarTypeToken>(), "Expected a typename after 'function' keyword", lineNumber);
		const DataTypeDefinition* returnType = tokens[offset].as<VarTypeToken>().mDataType;

		++offset;
		CHECK_ERROR(offset < tokens.size() && tokens[offset].isA<IdentifierToken>(), "Expected an identifier in function definition", lineNumber);
		const FlyweightString functionName = tokens[offset].as<IdentifierToken>().mName;

		++offset;
		CHECK_ERROR(offset < tokens.size() && isOperator(tokens[offset], Operator::PARENTHESIS_LEFT), "Expected opening parentheses in function definition", lineNumber);

		++offset;
		CHECK_ERROR(offset < tokens.size(), "Unexpected end of function definition", lineNumber);

		std::vector<Function::Parameter> parameters;
		if (tokens[offset].isA<OperatorToken>())
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

				CHECK_ERROR(tokens[offset].isA<VarTypeToken>(), "Expected type in function parameter definition", lineNumber);
				parameters.back().mDataType = tokens[offset].as<VarTypeToken>().mDataType;

				++offset;
				CHECK_ERROR(tokens[offset].isA<IdentifierToken>(), "Expected identifier in function parameter definition", lineNumber);
				parameters.back().mName = tokens[offset].as<IdentifierToken>().mName;

				++offset;
				CHECK_ERROR(tokens[offset].isA<OperatorToken>(), "Expected comma or closing parentheses after function parameter definition", lineNumber);
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

		// Create function in module
		ScriptFunction& function = mModule.addScriptFunction(functionName, returnType, parameters);

		// Create new variables for parameters
		for (const Function::Parameter& parameter : function.getParameters())
		{
			CHECK_ERROR(nullptr == function.getLocalVariableByIdentifier(parameter.mName.getHash()), "Parameter name already used", lineNumber);
			function.addLocalVariable(parameter.mName, parameter.mDataType, lineNumber);
		}

		// Set source metadata
		const auto& translated = mLineNumberTranslation.translateLineNumber(lineNumber);
		function.mSourceFileInfo = translated.mSourceFileInfo;
		function.mSourceBaseLineOffset = lineNumber - translated.mLineNumber;

		return function;
	}

	void CompilerFrontend::processSingleFunction(FunctionNode& functionNode)
	{
		BlockNode& content = *functionNode.mContent;
		ScriptFunction& function = *functionNode.mFunction;
		function.mStartLineNumber = functionNode.getLineNumber();

		// Build scope context for processing
		ScopeContext scopeContext;
		for (LocalVariable* localVariable : function.mLocalVariablesByID)
		{
			// All local variables so far have to be parameters; add each to the scope
			scopeContext.mLocalVariables.push_back(localVariable);
		}

		// Processing
		processUndefinedNodesInBlock(content, function, scopeContext);
	}

	void CompilerFrontend::processUndefinedNodesInBlock(BlockNode& blockNode, ScriptFunction& function, ScopeContext& scopeContext)
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

	Node* CompilerFrontend::processUndefinedNode(UndefinedNode& undefinedNode, ScriptFunction& function, ScopeContext& scopeContext, NodesIterator& nodesIterator)
	{
		const uint32 lineNumber = undefinedNode.getLineNumber();
		TokenList& tokens = undefinedNode.mTokenList;

		if (tokens[0].isA<KeywordToken>())
		{
			// Check for keywords
			const Keyword keyword = tokens[0].as<KeywordToken>().mKeyword;
			switch (keyword)
			{
				case Keyword::RETURN:
				{
					// Process tokens
					processTokens(tokens, function, scopeContext, lineNumber);

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

					// Special case: Indirect jump
					if (keyword == Keyword::JUMP && tokens.size() == 1 && tokens[0].isA<CommaSeparatedListToken>())
					{
						const CommaSeparatedListToken& cslt = tokens[0].as<CommaSeparatedListToken>();
						if (cslt.mContent.size() >= 2 && cslt.mContent[0].size() == 2 && cslt.mContent[0][1].isStatement())
						{
							JumpIndirectNode& node = NodeFactory::create<JumpIndirectNode>();
							node.mIndexToken = cslt.mContent[0][1].as<StatementToken>();

							size_t index = 1;
							while (index < cslt.mContent.size())
							{
								CHECK_ERROR(cslt.mContent[index].size() == 1, "Invalid syntax for indirect 'jump'", lineNumber);
								CHECK_ERROR(cslt.mContent[index][0].isA<LabelToken>(), "Invalid syntax for indirect 'jump'", lineNumber);
								vectorAdd(node.mLabelTokens) = cslt.mContent[index][0].as<LabelToken>();
								++index;
							}
							node.setLineNumber(lineNumber);
							return &node;
						}
					}

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
					else if (tokens[1].isA<LabelToken>())
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
					return processIfBlock(tokens, function, scopeContext, nodesIterator, lineNumber);
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

				case Keyword::CONSTANT:
				{
					processConstantDefinition(tokens, nodesIterator, &scopeContext);
					break;
				}

				default:
					break;
			}
		}
		else if (tokens[0].isA<LabelToken>())
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

			if (!tokens.empty())	// An empty tokens list can occur when a base call was completely removed
			{
				// Evaluate processed token tree
				CHECK_ERROR(tokens.size() == 1, "Statement contains more than a single token tree root", lineNumber);
				CHECK_ERROR(tokens[0].isStatement(), "Statement is no statement?", lineNumber);

				StatementNode& node = NodeFactory::create<StatementNode>();
				node.mStatementToken = tokens[0].as<StatementToken>();
				node.setLineNumber(lineNumber);
				tokens.erase(0);
				return &node;
			}
			else
			{
				// Just return an empty block - this is relevant if the removed code was e.g. the content of an if-clause
				return &NodeFactory::create<BlockNode>();
			}
		}

		return nullptr;
	}

	Node* CompilerFrontend::gatherNextStatement(NodesIterator& nodesIterator, ScriptFunction& function, ScopeContext& scopeContext)
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
					if (newNode != nullptr)
					{
						newNode->setLineNumber(un.getLineNumber());
					}
					return newNode;
				}

				default:
					break;
			}
		}
		return nullptr;
	}

	void CompilerFrontend::processTokens(TokenList& tokens, ScriptFunction& function, ScopeContext& scopeContext, uint32 lineNumber, const DataTypeDefinition* resultType)
	{
		mTokenProcessing.mContext.mFunction = &function;
		mTokenProcessing.mContext.mLocalVariables = &scopeContext.mLocalVariables;
		mTokenProcessing.mContext.mLocalConstants = &scopeContext.mLocalConstants;
		mTokenProcessing.mContext.mLocalConstantArrays = &scopeContext.mLocalConstantArrays;
		mTokenProcessing.processTokens(tokens, lineNumber, resultType);
	}

	void CompilerFrontend::processConstantDefinition(TokenList& tokens, NodesIterator& nodesIterator, ScopeContext* scopeContext)
	{
		const uint32 lineNumber = nodesIterator->getLineNumber();
		const bool isGlobalDefinition = (nullptr == scopeContext);
		CHECK_ERROR(tokens.size() >= 5, "Syntax error in constant definition", lineNumber);
		static const uint64 ARRAY_NAME_HASH = rmx::getMurmur2_64(std::string_view("array"));

		// Check for "constant array"
		if (isIdentifier(tokens[1], ARRAY_NAME_HASH))
		{
			CHECK_ERROR(tokens.size() >= 7, "Syntax error in constant array definition", lineNumber);
			CHECK_ERROR(isOperator(tokens[2], Operator::COMPARE_LESS), "Expected a type in <> in constant array definition", lineNumber);
			CHECK_ERROR(tokens[3].isA<VarTypeToken>(), "Expected a type in <> in constant array definition", lineNumber);
			CHECK_ERROR(isOperator(tokens[4], Operator::COMPARE_GREATER), "Expected a type in <> in constant array definition", lineNumber);
			CHECK_ERROR(tokens[5].isA<IdentifierToken>(), "Expected identifier in constant array definition", lineNumber);
			CHECK_ERROR(isOperator(tokens[6], Operator::ASSIGN), "Expected assignment at the end of constant array definition", lineNumber);

			static std::vector<uint64> values;
			values.clear();
			values.reserve(0x20);

			bool removeNextNode = false;
			if (tokens.size() >= 8)
			{
				CHECK_ERROR(tokens.size() >= 9, "Syntax error in constant array definition", lineNumber);
				CHECK_ERROR(isKeyword(tokens[7], Keyword::BLOCK_BEGIN), "Expected { or a line break after = in constant array definition", lineNumber);
				CHECK_ERROR(isKeyword(tokens.back(), Keyword::BLOCK_END), "Expected } at the end of constant array definition", lineNumber);

				// Handle one-liner definition of constant array
				bool expectingComma = false;
				for (size_t i = 8; i < tokens.size() - 1; ++i)
				{
					Token& token = tokens[i];
					if (expectingComma)
					{
						CHECK_ERROR(isOperator(token, Operator::COMMA_SEPARATOR), "Expected a comma-separated list of constants inside constant array list of values", lineNumber);
						expectingComma = false;
					}
					else
					{
						CHECK_ERROR(token.isA<ConstantToken>(), "Expected a comma-separated list of constants inside constant array list of values", lineNumber);
						values.push_back(token.as<ConstantToken>().mValue.get<uint64>());
						expectingComma = true;
					}
				}
			}
			else
			{
				Node* nextNode = nodesIterator.peek();
				CHECK_ERROR(nullptr != nextNode, "Constant array definition as last node is not allowed", lineNumber);
				CHECK_ERROR(nextNode->isA<BlockNode>(), "Expected block node after constant array header", lineNumber);

				// Go through the block node and collect the values
				BlockNode& content = nextNode->as<BlockNode>();
				bool expectingComma = false;
				for (size_t n = 0; n < content.mNodes.size(); ++n)
				{
					CHECK_ERROR(content.mNodes[n].isA<UndefinedNode>(), "Syntax error inside constant array list of values", content.mNodes[n].getLineNumber());
					UndefinedNode& listNode = content.mNodes[n].as<UndefinedNode>();
					for (size_t i = 0; i < listNode.mTokenList.size(); ++i)
					{
						Token& token = listNode.mTokenList[i];
						if (expectingComma)
						{
							CHECK_ERROR(isOperator(token, Operator::COMMA_SEPARATOR), "Expected a comma-separated list of constants inside constant array list of values", listNode.getLineNumber());
							expectingComma = false;
						}
						else
						{
							CHECK_ERROR(token.isA<ConstantToken>(), "Expected a comma-separated list of constants inside constant array list of values", listNode.getLineNumber());
							values.push_back(token.as<ConstantToken>().mValue.get<uint64>());
							expectingComma = true;
						}
					}
				}

				removeNextNode = true;
			}

			ConstantArray& constantArray = mModule.addConstantArray(tokens[5].as<IdentifierToken>().mName.getString(), tokens[3].as<VarTypeToken>().mDataType, &values[0], values.size(), isGlobalDefinition);
			if (isGlobalDefinition)
			{
				mGlobalsLookup.registerConstantArray(constantArray);
			}
			else
			{
				scopeContext->mLocalConstantArrays.push_back(&constantArray);
			}

			if (removeNextNode)
			{
				// Erase block node pointer
				++nodesIterator;
				nodesIterator.eraseCurrent();
			}
		}
		else
		{
			CHECK_ERROR(tokens[1].isA<VarTypeToken>(), "Expected a type in constant definition", lineNumber);
			CHECK_ERROR(tokens[2].isA<IdentifierToken>(), "Expected an identifier for constant definition", lineNumber);
			CHECK_ERROR(isOperator(tokens[3], Operator::ASSIGN), "Missing assignment in constant definition", lineNumber);

			// TODO: Support all statements that result in a compile-time constant
			CHECK_ERROR(tokens[4].isA<ConstantToken>(), "", lineNumber);

			const DataTypeDefinition* dataType = tokens[1].as<VarTypeToken>().mDataType;
			const FlyweightString identifier = tokens[2].as<IdentifierToken>().mName;
			const AnyBaseValue constantValue = tokens[4].as<ConstantToken>().mValue;

			if (isGlobalDefinition)
			{
				Constant& constant = mModule.addConstant(identifier, dataType, constantValue);
				mGlobalsLookup.registerConstant(constant);
			}
			else
			{
				Constant& constant = vectorAdd(scopeContext->mLocalConstants);
				constant.mName = identifier;
				constant.mDataType = dataType;
				constant.mValue = constantValue;
			}
		}
	}

	Node* CompilerFrontend::processIfBlock(TokenList& tokens_, ScriptFunction& function, ScopeContext& scopeContext, NodesIterator& nodesIterator, uint32 lineNumber)
	{
		IfStatementNode* firstIfStatementNode = nullptr;
		IfStatementNode* previousIfStatementNode = nullptr;
		TokenList* currentTokens = &tokens_;

		// Loop used in case we have multiple if-else-if, to avoid too deep recursion
		while (true)
		{
			// Assuming here that the first token was already checked and is an 'if' keyword
			TokenList& tokens = *currentTokens;
			if (mCompileOptions.mScriptFeatureLevel >= 2)
				CHECK_ERROR(tokens.size() >= 2 && isOperator(tokens[1], Operator::PARENTHESIS_LEFT), "Expected parentheses after 'if' keyword", lineNumber);

			// Process tokens
			processTokens(tokens, function, scopeContext, lineNumber);

			CHECK_ERROR(tokens.size() == 2, "Expected single statement after 'if' keyword", lineNumber);
			CHECK_ERROR(tokens[1].isStatement(), "Expected statement after 'if' keyword", lineNumber);

			// Create node and move the condition statement into it
			IfStatementNode& isn = NodeFactory::create<IfStatementNode>();
			isn.mConditionToken = tokens[1].as<StatementToken>();
			isn.setLineNumber(lineNumber);
			tokens.erase(1);

			// Is this the first loop iteration or already somewhere further down?
			if (nullptr == previousIfStatementNode)
			{
				// Set the return value
				firstIfStatementNode = &isn;
			}
			else
			{
				// Register in the previous node as 'else' content
				previousIfStatementNode->mContentElse = &isn;
			}

			// Go on with the next node, which must be either a block or a statement
			{
				Node* newNode = gatherNextStatement(nodesIterator, function, scopeContext);
				CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'if' line", lineNumber);
				newNode->setLineNumber(lineNumber);
				isn.mContentIf = newNode;
				nodesIterator.eraseCurrent();
			}

			// Also check for 'else'
			UndefinedNode* peekedNode = nodesIterator.peekSpecific<UndefinedNode>();
			if (nullptr != peekedNode && !peekedNode->mTokenList.empty() && isKeyword(peekedNode->mTokenList[0], Keyword::ELSE))
			{
				++nodesIterator;

				// Special case: 'else if' (or anything where there's a statement directly after the 'else')
				if (peekedNode->mTokenList.size() >= 2)
				{
					// Remove the 'else' here and treat the rest as a normal statement, as if it was on the next line
					peekedNode->mTokenList.erase(0);
				}
				else
				{
					// Remove the 'else', it does not contain anything other than the 'else ' itself
					nodesIterator.eraseCurrent();
					++nodesIterator;
				}

				Node* newNode = nullptr;
				Node* nextNode = nodesIterator.get();
				if (nullptr != nextNode)
				{
					if (nextNode->isA<BlockNode>())
					{
						// Handle block
						newNode = nextNode;
						processUndefinedNodesInBlock(nextNode->as<BlockNode>(), function, scopeContext);
					}
					else if (nextNode->isA<UndefinedNode>())
					{
						UndefinedNode& nextNodeUndefined = nextNode->as<UndefinedNode>();
						if (isKeyword(nextNodeUndefined.mTokenList[0], Keyword::IF))
						{
							nodesIterator.eraseCurrent();	// This seems to make no difference?

							// Loop once more
							currentTokens = &nextNodeUndefined.mTokenList;
							lineNumber = nextNodeUndefined.getLineNumber();
							previousIfStatementNode = &isn;
							continue;
						}
						else
						{
							newNode = processUndefinedNode(nextNodeUndefined, function, scopeContext, nodesIterator);
							newNode->setLineNumber(nextNodeUndefined.getLineNumber());
						}
					}
				}

				CHECK_ERROR(nullptr != newNode, "Expected a block or statement after 'else' line", peekedNode->getLineNumber());
				newNode->setLineNumber(peekedNode->getLineNumber());
				isn.mContentElse = newNode;
				nodesIterator.eraseCurrent();
			}

			// Done, break the loop
			break;
		}

		return firstIfStatementNode;
	}

	bool CompilerFrontend::processGlobalPragma(const std::string& content)
	{
		static const std::string PREFIX_FEATURE_LEVEL = "script-feature-level(";
		String str(content);
		str.trimWhitespace();
		if (str.startsWith(PREFIX_FEATURE_LEVEL) && str.endsWith(")"))
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
				mCompileOptions.mScriptFeatureLevel = (uint32)value;
			}
			return true;
		}
		return false;
	}

}
