/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Module.h"
#include "lemon/program/GlobalsLookup.h"

#include <iomanip>


namespace lemon
{
	namespace
	{
		static const std::vector<Function*> EMPTY_FUNCTIONS;
		static const SourceFileInfo EMPTY_SOURCE_FILE_INFO;

		static BaseType DEFAULT_OPCODE_BASETYPES[(size_t)Opcode::Type::_NUM_TYPES] =
		{
			BaseType::VOID,			// NOP
			BaseType::VOID,			// MOVE_STACK
			BaseType::VOID,			// MOVE_VAR_STACK
			BaseType::INT_CONST,	// PUSH_CONSTANT
			BaseType::VOID,			// DUPLICATE
			BaseType::VOID,			// EXCHANGE
			BaseType::UINT_32,		// GET_VARIABLE_VALUE
			BaseType::UINT_32,		// SET_VARIABLE_VALUE
			BaseType::UINT_8,		// READ_MEMORY
			BaseType::UINT_8,		// WRITE_MEMORY
			BaseType::VOID,			// CAST_VALUE
			BaseType::VOID,			// MAKE_BOOL
			BaseType::UINT_32,		// ARITHM_ADD
			BaseType::UINT_32,		// ARITHM_SUB
			BaseType::UINT_32,		// ARITHM_MUL
			BaseType::UINT_32,		// ARITHM_DIV
			BaseType::UINT_32,		// ARITHM_MOD
			BaseType::UINT_8,		// ARITHM_AND
			BaseType::UINT_8,		// ARITHM_OR
			BaseType::UINT_8,		// ARITHM_XOR
			BaseType::UINT_32,		// ARITHM_SHL
			BaseType::UINT_32,		// ARITHM_SHR
			BaseType::INT_CONST,	// ARITHM_NEG
			BaseType::UINT_8,		// ARITHM_NOT
			BaseType::UINT_8,		// ARITHM_BITNOT
			BaseType::UINT_8,		// COMPARE_EQ
			BaseType::UINT_8,		// COMPARE_NEQ
			BaseType::UINT_8,		// COMPARE_LT
			BaseType::UINT_8,		// COMPARE_LE
			BaseType::UINT_8,		// COMPARE_GT
			BaseType::UINT_8,		// COMPARE_GE
			BaseType::UINT_32,		// JUMP
			BaseType::UINT_32,		// JUMP_CONDITIONAL
			BaseType::VOID,			// CALL
			BaseType::VOID,			// RETURN
			BaseType::VOID,			// EXTERNAL_CALL
			BaseType::VOID,			// EXTERNAL_JUMP
		};
	}


	Module::Module(const std::string& name) :
		mModuleName(name),
		mModuleId(rmx::getMurmur2_64(name) & 0xffffffffffff0000ull)
	{
	}

	Module::~Module()
	{
		clear();
	}

	void Module::clear()
	{
		// Preprocessor definitions
		mPreprocessorDefinitions.clear();

		// Functions
		for (Function* func : mFunctions)
		{
			if (func->getType() == Function::Type::NATIVE)
				mNativeFunctionPool.destroyObject(*static_cast<NativeFunction*>(func));
			else
				mScriptFunctionPool.destroyObject(*static_cast<ScriptFunction*>(func));
		}
		mFunctions.clear();
		mScriptFunctions.clear();
		mNativeFunctionPool.clear();
		mScriptFunctionPool.clear();

		// Variables
		mGlobalVariables.clear();

		// Constants
		mConstants.clear();

		// Constant arrays
		mConstantArrays.clear();

		// Defines
		for (Define* define : mDefines)
		{
			mDefinePool.destroyObject(*define);
		}
		mDefines.clear();
		mDefinePool.clear();

		// String literals
		mStringLiterals.clear();

		// Clear source file infos
		mSourceFileInfoPool.clear();
		mAllSourceFiles.clear();
	}

	void Module::startCompiling(const GlobalsLookup& globalsLookup)
	{
		if (mFunctions.empty())
		{
			// It's the same here as for variables, see below
			mFirstFunctionID = globalsLookup.mNextFunctionID;
		}

		if (mGlobalVariables.empty())
		{
			// This here only makes sense if no global variables got set previously
			//  -> That's because the existing global variables are likely part of the globals lookup already
			//  -> Unfortunately, this is only safe to assume for the first module -- TODO: How to handle other cases?
			mFirstVariableID = globalsLookup.mNextVariableID;
		}

		if (mConstantArrays.empty())
		{
			mFirstConstantArrayID = globalsLookup.mNextConstantArrayID;
		}
	}

	void Module::dumpDefinitionsToScriptFile(const std::wstring& filename, bool append)
	{
		String content;
		if (!append)
		{
			content << "// This file was auto-generated";
		}
		content << "\r\n\r\n\r\n";
		content << "// === Module '" << getModuleName() << "' ===";

		// Constants
		if (!mConstants.empty())
		{
			content << "\r\n\r\n";
			content << "Constants";
			content << "\r\n\r\n";

			for (const Constant* constant : mConstants)
			{
				content << "declare constant " << constant->getDataType()->getName().getString() << " " << constant->getName().getString() << " = ";
				switch (constant->getDataType()->getClass())
				{
					case DataTypeDefinition::Class::INTEGER:
					{
						content << rmx::hexString(constant->getValue().get<uint64>());
						break;
					}

					case DataTypeDefinition::Class::FLOAT:
					{
						std::stringstream str;
						if (constant->getDataType()->getBytes() == 4)
						{
							str << std::setprecision(std::numeric_limits<float>::digits10) << constant->getValue().get<float>() << "f";
						}
						else
						{
							str << std::setprecision(std::numeric_limits<double>::digits10) << constant->getValue().get<double>();
						}
						content << str.str();
						break;
					}
				}
				content << "\r\n";
			}
		}

		// Functions
		std::vector<const Function*> currentFunctions;
		for (int pass = 0; pass < 2; ++pass)
		{
			// First pass: Regular functions -- Second pass: Methods
			const bool outputMethods = (pass == 1);

			currentFunctions.clear();
			for (const Function* function : mFunctions)
			{
				const bool isMethod = !function->getContext().isEmpty();
				if (isMethod == outputMethods)
				{
					if (function->getName().getString()[0] != '#')	// Exclude hidden built-ins (which can't be accessed by scripts directly anyways)
					{
						currentFunctions.push_back(function);
					}
				}
			}
			if (currentFunctions.empty())
				continue;

			content << "\r\n\r\n";
			content << (outputMethods ? "// Methods (to be called on variables directly)" : "// Regular functions");
			content << "\r\n";

			std::string lastPrefix = ".";		// Start with an invalid prefix so that first function will add a line break
			for (const Function* function : currentFunctions)
			{
				// Separate functiosn with different prefixes
				const size_t dot = function->getName().getString().find_first_of('.');
				std::string_view prefix = (dot == std::string_view::npos) ? std::string_view() : function->getName().getString().substr(0, dot);
				if (prefix != lastPrefix)
				{
					lastPrefix = prefix;
					content << "\r\n";
				}

				// Output signature declaration
				content << "declare function " << function->getReturnType()->getName().getString() << " ";

				size_t firstParam = 0;
				if (outputMethods)
				{
					content << function->getContext().getString() << ":" << function->getName().getString() << "(";
					firstParam = 1;
				}
				else
				{
					content << function->getName().getString() << "(";
				}

				for (size_t i = firstParam; i < function->getParameters().size(); ++i)
				{
					if (i > firstParam)
						content << ", ";
					const Function::Parameter& parameter = function->getParameters()[i];
					content << parameter.mDataType->getName().getString();
					if (parameter.mName.isValid())
						content << " " << parameter.mName.getString();
				}
				content << ")\r\n";
			}
		}

		FileHandle fileHandle;
		fileHandle.open(filename, append ? FILE_ACCESS_APPEND : FILE_ACCESS_WRITE);
		fileHandle.write(&content[0], content.length());
	}

	const SourceFileInfo& Module::addSourceFileInfo(const std::wstring& basepath, const std::wstring& filename)
	{
		SourceFileInfo& sourceFileInfo = mSourceFileInfoPool.createObject();
		sourceFileInfo.mFilename = filename;
		sourceFileInfo.mFullPath = basepath + filename;
		sourceFileInfo.mIndex = mAllSourceFiles.size();
		mAllSourceFiles.push_back(&sourceFileInfo);
		return sourceFileInfo;
	}

	void Module::registerNewPreprocessorDefinitions(PreprocessorDefinitionMap& preprocessorDefinitions)
	{
		for (uint64 hash : preprocessorDefinitions.getNewDefinitions())
		{
			const PreprocessorDefinition* definition = preprocessorDefinitions.getDefinition(hash);
			RMX_ASSERT(nullptr != definition, "Invalid entry in PreprocessorDefinitionMap's new definitions set");
			Constant& constant = addPreprocessorDefinition(definition->mIdentifier, definition->mValue);
			mPreprocessorDefinitions.emplace_back(&constant);
		}
		preprocessorDefinitions.clearNewDefinitions();
	}

	Constant& Module::addPreprocessorDefinition(FlyweightString name, int64 value)
	{
		Constant& constant = mConstantPool.createObject();
		constant.mName = name;
		constant.mDataType = &PredefinedDataTypes::INT_64;
		constant.mValue.set(value);
		mPreprocessorDefinitions.emplace_back(&constant);
		return constant;
	}

	const Function* Module::getFunctionByUniqueId(uint64 uniqueId) const
	{
		RMX_ASSERT(mModuleId == (uniqueId & 0xffffffffffff0000ull), "Function unique ID is not valid for this module");
		return mFunctions[uniqueId & 0xffff];
	}

	ScriptFunction& Module::addScriptFunction(FlyweightString name, const DataTypeDefinition* returnType, const Function::ParameterList& parameters, std::vector<FlyweightString>* aliasNames)
	{
		ScriptFunction& func = mScriptFunctionPool.createObject();
		func.setModule(*this);
		func.mName = name;
		func.mReturnType = returnType;
		func.mParameters = parameters;
		if (nullptr != aliasNames)
			func.mAliasNames = *aliasNames;

		addFunctionInternal(func);
		mScriptFunctions.push_back(&func);
		return func;
	}

	NativeFunction& Module::addNativeFunction(FlyweightString name, const NativeFunction::FunctionWrapper& functionWrapper, BitFlagSet<Function::Flag> flags)
	{
		NativeFunction& func = mNativeFunctionPool.createObject();
		func.mName = name;
		func.setFunction(functionWrapper);
		func.mFlags = flags;

		addFunctionInternal(func);
		return func;
	}

	NativeFunction& Module::addNativeMethod(FlyweightString context, FlyweightString name, const NativeFunction::FunctionWrapper& functionWrapper, BitFlagSet<Function::Flag> flags)
	{
		NativeFunction& func = mNativeFunctionPool.createObject();
		func.mContext = context;
		func.mName = name;
		func.setFunction(functionWrapper);
		func.mFlags = flags;

		addFunctionInternal(func);
		return func;
	}

	void Module::addFunctionInternal(Function& func)
	{
		RMX_ASSERT(mFunctions.size() < 0x10000, "Too many functions in module");
		func.mID = mFirstFunctionID + (uint32)mFunctions.size();
		if (func.mContext.isEmpty())
			func.mNameAndSignatureHash = func.mName.getHash() + func.getSignatureHash();
		else
			func.mNameAndSignatureHash = func.mContext.getHash() + func.mName.getHash() + func.getSignatureHash();
		mFunctions.push_back(&func);
	}

	GlobalVariable& Module::addGlobalVariable(FlyweightString name, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		GlobalVariable& variable = *new GlobalVariable();
		addGlobalVariable(variable, name, dataType);
		return variable;
	}

	UserDefinedVariable& Module::addUserDefinedVariable(FlyweightString name, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		UserDefinedVariable& variable = *new UserDefinedVariable();
		addGlobalVariable(variable, name, dataType);
		return variable;
	}

	ExternalVariable& Module::addExternalVariable(FlyweightString name, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		ExternalVariable& variable = *new ExternalVariable();
		addGlobalVariable(variable, name, dataType);
		return variable;
	}

	void Module::addGlobalVariable(Variable& variable, FlyweightString name, const DataTypeDefinition* dataType)
	{
		variable.mName = name;
		variable.mDataType = dataType;
		variable.mID = mFirstVariableID + (uint32)mGlobalVariables.size() + ((uint32)variable.mType << 28);
		mGlobalVariables.emplace_back(&variable);
	}

	LocalVariable& Module::createLocalVariable()
	{
		return mLocalVariablesPool.createObject();
	}

	void Module::destroyLocalVariable(LocalVariable& variable)
	{
		mLocalVariablesPool.destroyObject(variable);
	}

	Constant& Module::addConstant(FlyweightString name, const DataTypeDefinition* dataType, AnyBaseValue value)
	{
		Constant& constant = mConstantPool.createObject();
		constant.mName = name;
		constant.mDataType = dataType;
		constant.mValue = value;
		mConstants.emplace_back(&constant);
		return constant;
	}

	ConstantArray& Module::addConstantArray(FlyweightString name, const DataTypeDefinition* elementDataType, const uint64* values, size_t size, bool isGlobalDefinition)
	{
		ConstantArray& constantArray = mConstantArrayPool.createObject();
		constantArray.mName = name;
		constantArray.mElementDataType = elementDataType;
		constantArray.mID = mFirstConstantArrayID + (uint32)mConstantArrays.size();
		if (nullptr != values)
			constantArray.setContent(values, size);
		else if (size > 0)
			constantArray.setSize(size);
		mConstantArrays.emplace_back(&constantArray);

		if (isGlobalDefinition)
		{
			RMX_ASSERT(mNumGlobalConstantArrays + 1 == mConstantArrays.size(), "Gap in global constant arrays list");
			mNumGlobalConstantArrays = mConstantArrays.size();
		}
		return constantArray;
	}

	Define& Module::addDefine(FlyweightString name, const DataTypeDefinition* dataType)
	{
		Define& define = mDefinePool.createObject();
		define.mName = name;
		define.mDataType = dataType;
		mDefines.emplace_back(&define);
		return define;
	}

	void Module::addStringLiteral(FlyweightString str)
	{
		if (mStringLiterals.empty())
			mStringLiterals.reserve(0x100);
		mStringLiterals.push_back(str);
	}

	uint32 Module::buildDependencyHash() const
	{
		// Just a very simple "hash" that changes when a new definition gets added
		return (uint32)(mFunctions.size() + mGlobalVariables.size() + mConstants.size() + mConstantArrays.size() + mDefines.size() + mStringLiterals.size());
	}

	bool Module::serialize(VectorBinarySerializer& outerSerializer, uint32 dependencyHash, uint32 appVersion)
	{
		// Format version history:
		//  - 0x00 = First version, no signature yet
		//  - 0x01 = Added signature and version number + serialize global variable initial values
		//  - 0x02 = Variable size of opcode parameter serialization + dumping backwards compatibility with older versions, as it's not really needed
		//  - 0x03 = Added opcode flags and deflate compression
		//  - 0x04 = Added source information to function serialization
		//  - 0x05 = Added serialization of constants
		//  - 0x06 = Support for string data type in serialization - this breaks compatibility with older versions
		//  - 0x07 = Several compatibility breaking changes and extensions (e.g. constant arrays)
		//  - 0x08 = Added preprocessor definitions
		//  - 0x09 = Added source file infos + more compact way of line number serialization
		//  - 0x0a = Added dependency hash
		//  - 0x0b = Added app version
		//  - 0x0c = Added address hook serialization
		//  - 0x0d = Support for function alias names + added function flags as a small optimization
		//  - 0x0e = Change in serialization of std::wstring in rmx

		// Signature and version number
		const uint32 SIGNATURE = *(uint32*)"LMD|";	// "Lemonscript Module"
		const uint16 MINIMUM_VERSION = 0x0e;
		uint16 version = 0x0e;

		if (outerSerializer.isReading())
		{
			const uint32 signature = *(const uint32*)outerSerializer.peek();
			if (signature != SIGNATURE)
				return false;

			outerSerializer.skip(4);
			version = outerSerializer.read<uint16>();
			if (version < MINIMUM_VERSION)
				return false;	// Loading older versions is not supported

			const uint32 readDependencyHash = outerSerializer.read<uint32>();
			if (readDependencyHash != dependencyHash)
				return false;

			const uint32 readAppVersion = outerSerializer.read<uint32>();
			if (readAppVersion != appVersion)
				return false;
		}
		else
		{
			outerSerializer.write(SIGNATURE);
			outerSerializer.write(version);
			outerSerializer.write(dependencyHash);
			outerSerializer.write(appVersion);
		}

		// Setup buffer and serializer for the uncompressed data
		std::vector<uint8> uncompressed;
		if (outerSerializer.isReading())
		{
			if (!ZlibDeflate::decode(uncompressed, outerSerializer.peek(), outerSerializer.getRemaining()))
				return false;
			outerSerializer.skip(outerSerializer.getRemaining());
		}
		VectorBinarySerializer serializer(outerSerializer.isReading(), uncompressed);

		// Serialize module
		serializer & mFirstFunctionID;
		serializer & mFirstVariableID;

		// Serialize source file info
		{
			size_t numberOfSourceFiles = mAllSourceFiles.size();
			serializer.serializeAs<uint16>(numberOfSourceFiles);

			if (serializer.isReading())
			{
				std::wstring filename;
				for (size_t i = 0; i < numberOfSourceFiles; ++i)
				{
					serializer.serialize(filename, 1024);
					addSourceFileInfo(L"", filename);
				}
			}
			else
			{
				for (const SourceFileInfo* sourceFileInfo : mAllSourceFiles)
				{
					serializer.write(sourceFileInfo->mFilename, 1024);
				}
			}
		}

		// Serialize preprocessor definitions
		{
			size_t numberOfConstants = mPreprocessorDefinitions.size();
			serializer.serializeAs<uint16>(numberOfConstants);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfConstants; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const uint64 value = serializer.read<uint64>();
					addPreprocessorDefinition(name, value);
				}
			}
			else
			{
				for (Constant* constant : mPreprocessorDefinitions)
				{
					constant->getName().write(serializer);
					serializer.write(constant->mValue.get<uint64>());
				}
			}
		}

		// Serialize functions
		{
			uint32 numberOfFunctions = (uint32)mFunctions.size();
			serializer & numberOfFunctions;

			enum FunctionSerializationFlags
			{
				FLAG_NATIVE_FUNCTION	= 0x01,
				FLAG_HAS_ALIAS_NAMES	= 0x02,
				FLAG_HAS_RETURN_TYPE	= 0x04,
				FLAG_HAS_PARAMETERS		= 0x08,
				FLAG_HAS_LABELS			= 0x10,
				FLAG_HAS_ADDRESS_HOOKS	= 0x20,
				FLAG_HAS_PRAGMAS		= 0x40,
			};

			uint32 lastLineNumber = 0;
			std::vector<FlyweightString> aliasNames;
			Function::ParameterList parameters;
			for (uint32 i = 0; i < numberOfFunctions; ++i)
			{
				if (serializer.isReading())
				{
					const uint8 flags = serializer.read<uint8>();
					const Function::Type type = (flags & FLAG_NATIVE_FUNCTION) ? Function::Type::NATIVE : Function::Type::SCRIPT;
					
					FlyweightString name;
					name.serialize(serializer);

					aliasNames.clear();
					if (flags & FLAG_HAS_ALIAS_NAMES)
					{
						aliasNames.resize((size_t)serializer.read<uint8>());
						for (FlyweightString& aliasName : aliasNames)
							aliasName.serialize(serializer);
					}

					const DataTypeDefinition* returnType = (flags & FLAG_HAS_RETURN_TYPE) ? DataTypeSerializer::readDataType(serializer) : &PredefinedDataTypes::VOID;

					parameters.clear();
					if (flags & FLAG_HAS_PARAMETERS)
					{
						const uint8 parameterCount = serializer.read<uint8>();
						parameters.resize((size_t)parameterCount);
						for (uint8 k = 0; k < parameterCount; ++k)
						{
							parameters[k].mName.serialize(serializer);
							parameters[k].mDataType = DataTypeSerializer::readDataType(serializer);
						}
					}

					if (type == Function::Type::NATIVE)
					{
						// TODO: Check if it's there already and uses the same ID
					}
					else
					{
						// Create new script function
						ScriptFunction& scriptFunc = addScriptFunction(name, returnType, parameters, &aliasNames);

						// Source information
						const size_t index = (size_t)serializer.read<uint16>();
						scriptFunc.mSourceFileInfo = (index < mAllSourceFiles.size()) ? mAllSourceFiles[index] : &EMPTY_SOURCE_FILE_INFO;
						serializer.serialize(scriptFunc.mSourceBaseLineOffset);

						// Opcodes
						size_t count = (size_t)serializer.read<uint32>();
						scriptFunc.mOpcodes.resize(count);
						for (size_t k = 0; k < count; ++k)
						{
							Opcode& opcode = scriptFunc.mOpcodes[k];

							const uint16 typeAndFlags = serializer.read<uint16>();
							opcode.mType = (Opcode::Type)(typeAndFlags & 0x3f);

							const uint8 parameterBits  = (uint8)(typeAndFlags >> 6) & 0x07;
							const bool hasDataType     = (typeAndFlags & 0x200) != 0;
							const bool isSequenceBreak = (typeAndFlags & 0x400) != 0;
							const uint8 lineNumberBits = (uint8)(typeAndFlags >> 11) & 0x1f;

							switch (parameterBits)
							{
								default:
								case 0:  opcode.mParameter = 0;  break;
								case 1:  opcode.mParameter = 1;  break;
								case 2:  opcode.mParameter = -1; break;
								case 3:  opcode.mParameter = serializer.read<int8>();	break;
								case 4:  opcode.mParameter = serializer.read<int16>();	break;
								case 5:  opcode.mParameter = serializer.read<int32>();	break;
								case 6:  opcode.mParameter = serializer.read<int64>();	break;
							}

							opcode.mDataType = hasDataType ? (BaseType)serializer.read<uint8>() : DEFAULT_OPCODE_BASETYPES[(size_t)opcode.mType];

							// Load / rebuild the two opcode flags that are actually needed during run-time (see "OpcodeProcessor::buildOpcodeData")
							opcode.mFlags = isSequenceBreak ? Opcode::Flag::SEQ_BREAK : 0;
							switch (opcode.mType)
							{
								case Opcode::Type::JUMP:
								case Opcode::Type::JUMP_CONDITIONAL:
								case Opcode::Type::CALL:
								case Opcode::Type::RETURN:
								case Opcode::Type::EXTERNAL_CALL:
								case Opcode::Type::EXTERNAL_JUMP:
									opcode.mFlags |= Opcode::Flag::CTRLFLOW;
									break;
								default:
									break;
							}

							opcode.mLineNumber = (lineNumberBits == 31) ? serializer.read<uint32>() : (lastLineNumber + lineNumberBits);
							lastLineNumber = opcode.mLineNumber;
						}

						// Local variables
						count = (size_t)serializer.read<uint32>();
						for (size_t k = 0; k < count; ++k)
						{
							FlyweightString name;
							name.serialize(serializer);
							const DataTypeDefinition* dataType = DataTypeSerializer::readDataType(serializer);
							scriptFunc.addLocalVariable(name, dataType, 0);
						}

						// Labels
						if (flags & FLAG_HAS_LABELS)
						{
							count = (size_t)serializer.read<uint32>();
							for (size_t k = 0; k < count; ++k)
							{
								FlyweightString name;
								name.serialize(serializer);
								const uint32 offset = serializer.read<uint32>();
								scriptFunc.addLabel(name, (size_t)offset);
							}
						}

						// Address hooks
						if (flags & FLAG_HAS_ADDRESS_HOOKS)
						{
							count = (size_t)serializer.read<uint32>();
							for (size_t k = 0; k < count; ++k)
							{
								scriptFunc.mAddressHooks.emplace_back(serializer.read<uint32>());
							}
						}

						// Pragmas
						if (flags & FLAG_HAS_PRAGMAS)
						{
							count = (size_t)serializer.read<uint32>();
							for (size_t k = 0; k < count; ++k)
							{
								scriptFunc.mPragmas.emplace_back(serializer.read<std::string>());
							}
						}
					}
				}
				else
				{
					const Function& function = *mFunctions[i];

					uint8 flags = 0;
					flags |= FLAG_NATIVE_FUNCTION * (function.getType() == Function::Type::NATIVE);
					flags |= FLAG_HAS_ALIAS_NAMES * (!function.mAliasNames.empty());
					flags |= FLAG_HAS_RETURN_TYPE * (function.mReturnType != &PredefinedDataTypes::VOID);
					flags |= FLAG_HAS_PARAMETERS  * (!function.mParameters.empty());
					if (function.getType() == Function::Type::SCRIPT)
					{
						const ScriptFunction& scriptFunc = static_cast<const ScriptFunction&>(function);
						flags |= FLAG_HAS_LABELS		* (!scriptFunc.mLabels.empty());
						flags |= FLAG_HAS_ADDRESS_HOOKS * (!scriptFunc.mAddressHooks.empty());
						flags |= FLAG_HAS_PRAGMAS		* (!scriptFunc.mPragmas.empty());
					}
					serializer.write(flags);

					function.mName.write(serializer);

					if (flags & FLAG_HAS_ALIAS_NAMES)
					{
						serializer.writeAs<uint8>(function.mAliasNames.size());
						for (const FlyweightString& aliasName : function.mAliasNames)
							aliasName.write(serializer);
					}

					if (flags & FLAG_HAS_RETURN_TYPE)
					{
						DataTypeSerializer::writeDataType(serializer, function.mReturnType);
					}

					if (flags & FLAG_HAS_PARAMETERS)
					{
						const uint8 parameterCount = (uint8)function.mParameters.size();
						serializer.write(parameterCount);
						for (uint8 k = 0; k < parameterCount; ++k)
						{
							function.mParameters[k].mName.write(serializer);
							DataTypeSerializer::writeDataType(serializer, function.mParameters[k].mDataType);
						}
					}

					if (function.getType() == Function::Type::SCRIPT)
					{
						// Load script function
						const ScriptFunction& scriptFunc = static_cast<const ScriptFunction&>(function);

						// Source information
						serializer.writeAs<uint16>(scriptFunc.mSourceFileInfo->mIndex);
						serializer.write(scriptFunc.mSourceBaseLineOffset);

						// Opcodes
						serializer.writeAs<uint32>(scriptFunc.mOpcodes.size());
						for (const Opcode& opcode : scriptFunc.mOpcodes)
						{
							static_assert((size_t)Opcode::Type::_NUM_TYPES <= 64);

							const uint8 parameterBits = (opcode.mParameter == 0)  ? 0 :
														(opcode.mParameter == 1)  ? 1 :
														(opcode.mParameter == -1) ? 2 :
														(opcode.mParameter == (int64)(int8)opcode.mParameter)  ? 3 :
														(opcode.mParameter == (int64)(int16)opcode.mParameter) ? 4 :
														(opcode.mParameter == (int64)(int32)opcode.mParameter) ? 5 : 6;
							const bool hasDataType     = (opcode.mDataType != DEFAULT_OPCODE_BASETYPES[(size_t)opcode.mType]);
							const bool isSequenceBreak = (opcode.mFlags & Opcode::Flag::SEQ_BREAK) != 0;
							const uint8 lineNumberBits = (opcode.mLineNumber >= lastLineNumber && opcode.mLineNumber < lastLineNumber + 31) ? (opcode.mLineNumber - lastLineNumber) : 31;

							const uint16 typeAndFlags = (uint16)opcode.mType | ((uint16)parameterBits << 6) | ((uint16)hasDataType * 0x200) | ((uint16)isSequenceBreak * 0x400) | ((uint16)lineNumberBits << 11);
							serializer.write(typeAndFlags);

							switch (parameterBits)
							{
								case 3:  serializer.writeAs<int8>(opcode.mParameter);	break;
								case 4:  serializer.writeAs<int16>(opcode.mParameter);	break;
								case 5:  serializer.writeAs<int32>(opcode.mParameter);	break;
								case 6:  serializer.write(opcode.mParameter);			break;
								default: break;
							}

							if (hasDataType)
								serializer.writeAs<uint8>(opcode.mDataType);
							if (lineNumberBits == 31)
								serializer.write(opcode.mLineNumber);
							lastLineNumber = opcode.mLineNumber;
						}

						// Local variables
						serializer.writeAs<uint32>(scriptFunc.mLocalVariablesByID.size());
						for (const LocalVariable* var : scriptFunc.mLocalVariablesByID)
						{
							var->getName().serialize(serializer);
							DataTypeSerializer::writeDataType(serializer, var->getDataType());
						}

						// Labels
						if (flags & FLAG_HAS_LABELS)
						{
							serializer.writeAs<uint32>(scriptFunc.mLabels.size());
							for (const ScriptFunction::Label& label : scriptFunc.mLabels)
							{
								label.mName.write(serializer);
								serializer.write(label.mOffset);
							}
						}

						// Address hooks
						if (flags & FLAG_HAS_ADDRESS_HOOKS)
						{
							serializer.writeAs<uint32>(scriptFunc.mAddressHooks.size());
							for (uint32 addressHook : scriptFunc.mAddressHooks)
							{
								serializer.write(addressHook);
							}
						}

						// Pragmas
						if (flags & FLAG_HAS_PRAGMAS)
						{
							serializer.writeAs<uint32>(scriptFunc.mPragmas.size());
							for (const std::string& pragma : scriptFunc.mPragmas)
							{
								serializer.write(pragma);
							}
						}
					}
				}
			}
		}

		// Serialize global variables
		if (serializer.isReading())
		{
			const uint32 numberOfUserDefined = serializer.read<uint32>();
			RMX_CHECK(mGlobalVariables.size() == (size_t)numberOfUserDefined, "Other number of user-defined variables", return false);

			const uint32 numberOfGlobals = serializer.read<uint32>();
			for (uint32 i = 0; i < numberOfGlobals; ++i)
			{
				FlyweightString name;
				name.serialize(serializer);
				const DataTypeDefinition* dataType = DataTypeSerializer::readDataType(serializer);
				const int64 initialValue = serializer.read<int64>();
				GlobalVariable& globalVariable = addGlobalVariable(name, dataType);
				globalVariable.mInitialValue = initialValue;
			}
		}
		else
		{
			// Ignore all user-defined variables, which must be first inside "mGlobalVariables"
			size_t i = 0;
			for (; i < mGlobalVariables.size(); ++i)
			{
				if (mGlobalVariables[i]->getType() == Variable::Type::GLOBAL)
					break;
			}
			serializer.writeAs<uint32>(i);		// Number of user-defined variables

			uint32 numberOfGlobals = (uint32)(mGlobalVariables.size() - i);
			serializer & numberOfGlobals;

			for (; i < mGlobalVariables.size(); ++i)
			{
				const Variable& variable = *mGlobalVariables[i];
				RMX_CHECK(variable.getType() == Variable::Type::GLOBAL, "Mix of global variables and others", return false);
				const GlobalVariable& globalVariable = static_cast<const GlobalVariable&>(variable);

				variable.getName().serialize(serializer);
				DataTypeSerializer::writeDataType(serializer, variable.getDataType());
				serializer.writeAs<int64>(globalVariable.mInitialValue);
			}
		}

		// Serialize constants
		{
			size_t numberOfConstants = mConstants.size();
			serializer.serializeAs<uint16>(numberOfConstants);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfConstants; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = DataTypeSerializer::readDataType(serializer);
					const uint64 value = serializer.read<uint64>();
					addConstant(name, dataType, AnyBaseValue(value));
				}
			}
			else
			{
				for (Constant* constant : mConstants)
				{
					constant->getName().write(serializer);
					DataTypeSerializer::writeDataType(serializer, constant->getDataType());
					serializer.write(constant->mValue.get<uint64>());
				}
			}
		}

		// Serialize constant arrays
		{
			size_t numberOfConstantArrays = mConstantArrays.size();
			serializer.serializeAs<uint16>(numberOfConstantArrays);

			if (serializer.isReading())
			{
				const size_t numGlobalConstantArrays = (size_t)serializer.read<uint16>();
				for (size_t i = 0; i < numberOfConstantArrays; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = DataTypeSerializer::readDataType(serializer);
					ConstantArray& constantArray = addConstantArray(name, dataType, nullptr, 0, i < numGlobalConstantArrays);
					constantArray.serializeData(serializer);
				}
			}
			else
			{
				serializer.serializeAs<uint16>(mNumGlobalConstantArrays);
				for (ConstantArray* constantArray : mConstantArrays)
				{
					constantArray->getName().write(serializer);
					DataTypeSerializer::writeDataType(serializer, constantArray->getElementDataType());
					constantArray->serializeData(serializer);
				}
			}
		}

		// Serialize defines
		{
			size_t numberOfDefines = mDefines.size();
			serializer.serializeAs<uint16>(numberOfDefines);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfDefines; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = DataTypeSerializer::readDataType(serializer);

					Define& define = addDefine(name, dataType);
					TokenSerializer::serializeTokenList(serializer, define.mContent);
				}
			}
			else
			{
				for (Define* define : mDefines)
				{
					define->getName().serialize(serializer);
					DataTypeSerializer::writeDataType(serializer, define->getDataType());

					TokenSerializer::serializeTokenList(serializer, define->mContent);
				}
			}
		}

		// Serialize string literals
		{
			serializer.serializeArraySize(mStringLiterals);
			for (FlyweightString& str : mStringLiterals)
			{
				str.serialize(serializer);
			}
		}

		if (!outerSerializer.isReading())
		{
			std::vector<uint8> compressed;
			if (!ZlibDeflate::encode(compressed, &uncompressed[0], uncompressed.size(), 5))
				return false;
			outerSerializer.write(&compressed[0], compressed.size());
		}

		return true;
	}

}
