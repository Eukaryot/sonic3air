/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/ModuleSerializer.h"
#include "lemon/program/Module.h"
#include "lemon/program/GlobalsLookup.h"


namespace lemon
{
	namespace
	{
		static const SourceFileInfo EMPTY_SOURCE_FILE_INFO;

		static const BaseType DEFAULT_OPCODE_BASETYPES[(size_t)Opcode::Type::_NUM_TYPES] =
		{
			BaseType::VOID,			// NOP
			BaseType::VOID,			// MOVE_STACK
			BaseType::VOID,			// MOVE_VAR_STACK
			BaseType::INT_CONST,	// PUSH_CONSTANT
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
			BaseType::VOID,			// JUMP
			BaseType::VOID,			// JUMP_CONDITIONAL
			BaseType::VOID,			// JUMP_SWITCH
			BaseType::VOID,			// CALL
			BaseType::VOID,			// RETURN
			BaseType::VOID,			// EXTERNAL_CALL
			BaseType::VOID,			// EXTERNAL_JUMP
		};
	}


	bool ModuleSerializer::serialize(Module& module, VectorBinarySerializer& outerSerializer, const GlobalsLookup& globalsLookup, uint32 dependencyHash, uint32 appVersion)
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
		//  - 0x0f = Smaller optimizations in serialization
		//  - 0x10 = Opcode JUMP_SWITCH added

		// Signature and version number
		const uint32 SIGNATURE = *(uint32*)"LMD|";	// "Lemonscript Module"
		const uint16 MINIMUM_VERSION = 0x10;
		uint16 version = 0x10;

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
		serializer & module.mFirstFunctionID;
		serializer & module.mFirstVariableID;

		// Serialize source file info
		{
			size_t numberOfSourceFiles = module.mAllSourceFiles.size();
			serializer.serializeAs<uint16>(numberOfSourceFiles);

			if (serializer.isReading())
			{
				std::wstring filename;
				for (size_t i = 0; i < numberOfSourceFiles; ++i)
				{
					serializer.serialize(filename, 1024);
					module.addSourceFileInfo(L"", filename);
				}
			}
			else
			{
				for (const SourceFileInfo* sourceFileInfo : module.mAllSourceFiles)
				{
					serializer.write(sourceFileInfo->mFilename, 1024);
				}
			}
		}

		// Serialize preprocessor definitions
		{
			size_t numberOfConstants = module.mPreprocessorDefinitions.size();
			serializer.serializeAs<uint16>(numberOfConstants);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfConstants; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const uint64 value = serializer.read<uint64>();
					module.addPreprocessorDefinition(name, value);
				}
			}
			else
			{
				for (Constant* constant : module.mPreprocessorDefinitions)
				{
					constant->getName().write(serializer);
					serializer.write(constant->mValue.get<uint64>());
				}
			}
		}

		// Serialize functions
		serializeFunctions(module, serializer, globalsLookup);

		// Serialize global variables
		if (serializer.isReading())
		{
			const uint32 numberOfUserDefined = serializer.read<uint32>();
			RMX_CHECK(module.mGlobalVariables.size() == (size_t)numberOfUserDefined, "Other number of user-defined variables", return false);

			const uint32 numberOfGlobals = serializer.read<uint32>();
			for (uint32 i = 0; i < numberOfGlobals; ++i)
			{
				FlyweightString name;
				name.serialize(serializer);
				const DataTypeDefinition* dataType = globalsLookup.readDataType(serializer);
				const int64 initialValue = serializer.read<int64>();
				GlobalVariable& globalVariable = module.addGlobalVariable(name, dataType);
				globalVariable.mInitialValue = initialValue;
			}
		}
		else
		{
			// Ignore all user-defined variables, which must be first inside "mGlobalVariables"
			size_t i = 0;
			for (; i < module.mGlobalVariables.size(); ++i)
			{
				if (module.mGlobalVariables[i]->getType() == Variable::Type::GLOBAL)
					break;
			}
			serializer.writeAs<uint32>(i);		// Number of user-defined variables

			uint32 numberOfGlobals = (uint32)(module.mGlobalVariables.size() - i);
			serializer & numberOfGlobals;

			for (; i < module.mGlobalVariables.size(); ++i)
			{
				const Variable& variable = *module.mGlobalVariables[i];
				RMX_CHECK(variable.getType() == Variable::Type::GLOBAL, "Mix of global variables and others", return false);
				const GlobalVariable& globalVariable = static_cast<const GlobalVariable&>(variable);

				variable.getName().serialize(serializer);
				serializer.write(variable.getDataType()->getID());
				serializer.writeAs<int64>(globalVariable.mInitialValue);
			}
		}

		// Serialize constants
		{
			size_t numberOfConstants = module.mConstants.size();
			serializer.serializeAs<uint16>(numberOfConstants);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfConstants; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = globalsLookup.readDataType(serializer);
					const uint64 value = serializer.read<uint64>();
					module.addConstant(name, dataType, AnyBaseValue(value));
				}
			}
			else
			{
				for (Constant* constant : module.mConstants)
				{
					constant->getName().write(serializer);
					serializer.write(constant->getDataType()->getID());
					serializer.write(constant->mValue.get<uint64>());
				}
			}
		}

		// Serialize constant arrays
		{
			size_t numberOfConstantArrays = module.mConstantArrays.size();
			serializer.serializeAs<uint16>(numberOfConstantArrays);

			if (serializer.isReading())
			{
				const size_t numGlobalConstantArrays = (size_t)serializer.read<uint16>();
				for (size_t i = 0; i < numberOfConstantArrays; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = globalsLookup.readDataType(serializer);
					ConstantArray& constantArray = module.addConstantArray(name, dataType, nullptr, 0, i < numGlobalConstantArrays);
					constantArray.serializeData(serializer);
				}
			}
			else
			{
				serializer.serializeAs<uint16>(module.mNumGlobalConstantArrays);
				for (ConstantArray* constantArray : module.mConstantArrays)
				{
					constantArray->getName().write(serializer);
					serializer.write(constantArray->getElementDataType()->getID());
					constantArray->serializeData(serializer);
				}
			}
		}

		// Serialize defines
		{
			size_t numberOfDefines = module.mDefines.size();
			serializer.serializeAs<uint16>(numberOfDefines);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfDefines; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const DataTypeDefinition* dataType = globalsLookup.readDataType(serializer);

					Define& define = module.addDefine(name, dataType);
					TokenSerializer::serializeTokenList(serializer, define.mContent, globalsLookup);
				}
			}
			else
			{
				for (Define* define : module.mDefines)
				{
					define->getName().serialize(serializer);
					serializer.write(define->getDataType()->getID());
					TokenSerializer::serializeTokenList(serializer, define->mContent, globalsLookup);
				}
			}
		}

		// Serialize string literals
		{
			serializer.serializeArraySize(module.mStringLiterals);
			for (FlyweightString& str : module.mStringLiterals)
			{
				str.serialize(serializer);
			}
		}

		// Serialize data types
		{
			size_t numberOfDataTypes = module.mDataTypes.size();
			serializer.serializeAs<uint16>(numberOfDataTypes);

			if (serializer.isReading())
			{
				for (size_t i = 0; i < numberOfDataTypes; ++i)
				{
					FlyweightString name;
					name.serialize(serializer);
					const BaseType baseType = (BaseType)serializer.read<uint8>();
					module.addDataType(name.getString().data(), baseType);
				}
			}
			else
			{
				for (const CustomDataType* dataType : module.mDataTypes)
				{
					dataType->getName().serialize(serializer);
					serializer.writeAs<uint8>(dataType->getBaseType());
				}
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

	void ModuleSerializer::serializeFunctions(Module& module, VectorBinarySerializer& serializer, const GlobalsLookup& globalsLookup)
	{
		uint32 numberOfFunctions = (uint32)module.mFunctions.size();
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

				FlyweightString functionName;
				functionName.serialize(serializer);

				aliasNames.clear();
				if (flags & FLAG_HAS_ALIAS_NAMES)
				{
					aliasNames.resize((size_t)serializer.read<uint8>());
					for (FlyweightString& aliasName : aliasNames)
						aliasName.serialize(serializer);
				}

				const DataTypeDefinition* returnType = (flags & FLAG_HAS_RETURN_TYPE) ? globalsLookup.readDataType(serializer) : &PredefinedDataTypes::VOID;

				parameters.clear();
				if (flags & FLAG_HAS_PARAMETERS)
				{
					const uint8 parameterCount = serializer.read<uint8>();
					parameters.resize((size_t)parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						parameters[k].mName.serialize(serializer);
						parameters[k].mDataType = globalsLookup.readDataType(serializer);
					}
				}

				if (type == Function::Type::NATIVE)
				{
					// TODO: Check if it's there already and uses the same ID
				}
				else
				{
					// Create new script function
					ScriptFunction& scriptFunc = module.addScriptFunction(functionName, returnType, parameters, &aliasNames);

					// Source information
					const size_t index = (size_t)serializer.read<uint16>();
					scriptFunc.mSourceFileInfo = (index < module.mAllSourceFiles.size()) ? module.mAllSourceFiles[index] : &EMPTY_SOURCE_FILE_INFO;
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
							case 6:  opcode.mParameter = serializer.read<uint32>();	break;
							case 7:  opcode.mParameter = serializer.read<int64>();	break;
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
						const DataTypeDefinition* dataType = globalsLookup.readDataType(serializer);
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
				const Function& function = *module.mFunctions[i];

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
					serializer.write(function.mReturnType->getID());
				}

				if (flags & FLAG_HAS_PARAMETERS)
				{
					const uint8 parameterCount = (uint8)function.mParameters.size();
					serializer.write(parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						function.mParameters[k].mName.write(serializer);
						serializer.write(function.mParameters[k].mDataType->getID());
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
							(opcode.mParameter == (int64)(int32)opcode.mParameter) ? 5 :
							(opcode.mParameter == (int64)(uint32)opcode.mParameter) ? 6 : 7;
						const bool hasDataType     = (opcode.mDataType != DEFAULT_OPCODE_BASETYPES[(size_t)opcode.mType]);
						const bool isSequenceBreak = (opcode.mFlags & Opcode::Flag::SEQ_BREAK) != 0;
						const uint8 lineNumberBits = (opcode.mLineNumber >= lastLineNumber && opcode.mLineNumber < lastLineNumber + 31) ? (uint8)(opcode.mLineNumber - lastLineNumber) : 31;

						const uint16 typeAndFlags = (uint16)opcode.mType | ((uint16)parameterBits << 6) | ((uint16)hasDataType * 0x200) | ((uint16)isSequenceBreak * 0x400) | ((uint16)lineNumberBits << 11);
						serializer.write(typeAndFlags);

						switch (parameterBits)
						{
							case 3:  serializer.writeAs<int8>(opcode.mParameter);	break;
							case 4:  serializer.writeAs<int16>(opcode.mParameter);	break;
							case 5:  serializer.writeAs<int32>(opcode.mParameter);	break;
							case 6:  serializer.writeAs<uint32>(opcode.mParameter);	break;
							case 7:  serializer.write(opcode.mParameter);			break;
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
						serializer.write(var->getDataType()->getID());
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

}
