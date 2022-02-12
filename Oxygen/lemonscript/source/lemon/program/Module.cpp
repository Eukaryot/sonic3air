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


namespace lemon
{
	namespace
	{
		static const std::vector<Function*> EMPTY_FUNCTIONS;
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
			if (func->getType() == Function::Type::USER)
				mUserDefinedFunctionPool.destroyObject(*static_cast<UserDefinedFunction*>(func));
			else
				mScriptFunctionPool.destroyObject(*static_cast<ScriptFunction*>(func));
		}
		mFunctions.clear();
		mScriptFunctions.clear();
		mUserDefinedFunctionPool.clear();
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

	void Module::dumpDefinitionsToScriptFile(const std::wstring& filename)
	{
		String content;
		content << "// This file was auto-generated from the definitions in lemon script module '" << getModuleName() << "'.\r\n";
		content << "\r\n";
		
		for (const Function* function : mFunctions)
		{
			if (function->getName().getString()[0] == '#')	// Exclude hidden built-ins (which can't be accessed by scripts directly anyways)
				continue;

			content << "\r\n";
			content << "declare function " << function->getReturnType()->toString() << " " << function->getName().getString() << "(";
			for (size_t i = 0; i < function->getParameters().size(); ++i)
			{
				if (i != 0)
					content << ", ";
				const Function::Parameter& parameter = function->getParameters()[i];
				content << parameter.mType->toString();
				if (parameter.mName.isValid())
					content << " " << parameter.mName.getString();
			}
			content << ")\r\n";
		}

		content.saveFile(filename);
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
		constant.mValue = value;
		mPreprocessorDefinitions.emplace_back(&constant);
		return constant;
	}

	const Function* Module::getFunctionByUniqueId(uint64 uniqueId) const
	{
		RMX_ASSERT(mModuleId == (uniqueId & 0xffffffffffff0000ull), "Function unique ID is not valid for this module");
		return mFunctions[uniqueId & 0xffff];
	}

	ScriptFunction& Module::addScriptFunction(FlyweightString name, const DataTypeDefinition* returnType, const Function::ParameterList* parameters)
	{
		ScriptFunction& func = mScriptFunctionPool.createObject();
		func.setModule(*this);
		func.mName = name;
		func.mReturnType = returnType;
		if (nullptr != parameters)
			func.mParameters = *parameters;

		addFunctionInternal(func);
		mScriptFunctions.push_back(&func);
		return func;
	}

	UserDefinedFunction& Module::addUserDefinedFunction(FlyweightString name, const UserDefinedFunction::FunctionWrapper& functionWrapper, uint8 flags)
	{
		UserDefinedFunction& func = mUserDefinedFunctionPool.createObject();
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
		func.mNameAndSignatureHash = func.mName.getHash() + func.getSignatureHash();
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

	Constant& Module::addConstant(FlyweightString name, const DataTypeDefinition* dataType, uint64 value)
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

	bool Module::serialize(VectorBinarySerializer& outerSerializer)
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

		// Signature and version number
		const uint32 SIGNATURE = *(uint32*)"LMD|";
		uint16 version = 0x08;
		if (outerSerializer.isReading())
		{
			const uint32 signature = *(const uint32*)outerSerializer.peek();
			if (signature != SIGNATURE)
				return false;

			outerSerializer.skip(4);
			version = outerSerializer.read<uint16>();
			if (version < 0x08)
				return false;	// Loading older versions is not supported
		}
		else
		{
			outerSerializer.write(SIGNATURE);
			outerSerializer.write(version);
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
					serializer.write(constant->mValue);
				}
			}
		}

		// Serialize functions
		{
			uint32 numberOfFunctions = (uint32)mFunctions.size();
			serializer & numberOfFunctions;

			for (uint32 i = 0; i < numberOfFunctions; ++i)
			{
				if (serializer.isReading())
				{
					const Function::Type type = (Function::Type)serializer.read<uint8>();
					FlyweightString name;
					name.serialize(serializer);

					const DataTypeDefinition* returnType = DataTypeHelper::readDataType(serializer);
					Function::ParameterList parameters;
					const uint8 parameterCount = serializer.read<uint8>();
					parameters.resize((size_t)parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						parameters[k].mName.serialize(serializer);
						parameters[k].mType = DataTypeHelper::readDataType(serializer);
					}

					if (type == Function::Type::USER)
					{
						// TODO: Check if it's there already and uses the same ID
					}
					else
					{
						// Create new script function
						ScriptFunction& scriptFunc = addScriptFunction(name, returnType, &parameters);
						uint32 lastLineNumber = 0;

						// Source information
						serializer.serialize(scriptFunc.mSourceFilename);
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
							const bool hasOpcodeFlags  = (typeAndFlags & 0x400) != 0;
							const uint8 lineNumberBits = (uint8)(typeAndFlags >> 11) & 0x03;

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

							opcode.mDataType = hasDataType ? (BaseType)serializer.read<uint8>() : BaseType::VOID;
							opcode.mFlags = hasOpcodeFlags ? serializer.read<uint8>() : 0;
							opcode.mLineNumber = (lineNumberBits == 3) ? serializer.read<uint32>() : (lastLineNumber + lineNumberBits);
							lastLineNumber = opcode.mLineNumber;
						}

						// Local variables
						count = (size_t)serializer.read<uint32>();
						for (size_t k = 0; k < count; ++k)
						{
							FlyweightString name;
							name.serialize(serializer);
							const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
							scriptFunc.addLocalVariable(name, dataType, 0);
						}

						// Labels
						count = (size_t)serializer.read<uint32>();
						for (size_t k = 0; k < count; ++k)
						{
							FlyweightString name;
							name.serialize(serializer);
							const uint32 offset = serializer.read<uint32>();
							scriptFunc.addLabel(name, (size_t)offset);
						}

						// Pragmas
						count = (size_t)serializer.read<uint32>();
						for (size_t k = 0; k < count; ++k)
						{
							scriptFunc.mPragmas.emplace_back(serializer.read<std::string>());
						}
					}
				}
				else
				{
					const Function& function = *mFunctions[i];

					serializer.write((uint8)function.getType());
					function.mName.write(serializer);

					DataTypeHelper::writeDataType(serializer, function.mReturnType);
					const uint8 parameterCount = (uint8)function.mParameters.size();
					serializer.write(parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						function.mParameters[k].mName.write(serializer);
						DataTypeHelper::writeDataType(serializer, function.mParameters[k].mType);
					}

					if (function.getType() == Function::Type::SCRIPT)
					{
						// Load script function
						const ScriptFunction& scriptFunc = static_cast<const ScriptFunction&>(function);
						uint32 lastLineNumber = 0;

						// Source information
						serializer.write(scriptFunc.mSourceFilename);
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
							const bool hasDataType    = (opcode.mDataType != BaseType::VOID);
							const bool hasOpcodeFlags = (opcode.mFlags != 0);
							const uint8 lineNumberBits = (opcode.mLineNumber >= lastLineNumber && opcode.mLineNumber <= lastLineNumber + 2) ? (opcode.mLineNumber - lastLineNumber) : 3;

							const uint16 typeAndFlags = (uint16)opcode.mType | ((uint16)parameterBits << 6) | ((uint16)hasDataType * 0x200) | ((uint16)hasOpcodeFlags * 0x400) | ((uint16)lineNumberBits << 11);
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
							if (hasOpcodeFlags)
								serializer.write(opcode.mFlags);
							if (lineNumberBits == 3)
								serializer.write(opcode.mLineNumber);
							lastLineNumber = opcode.mLineNumber;
						}

						// Local variables
						serializer.writeAs<uint32>(scriptFunc.mLocalVariablesByID.size());
						for (const LocalVariable* var : scriptFunc.mLocalVariablesByID)
						{
							var->getName().serialize(serializer);
							DataTypeHelper::writeDataType(serializer, var->getDataType());
						}

						// Labels
						serializer.writeAs<uint32>(scriptFunc.mLabels.size());
						for (const ScriptFunction::Label& label : scriptFunc.mLabels)
						{
							label.mName.write(serializer);
							serializer.write(label.mOffset);
						}

						// Pragmas
						serializer.writeAs<uint32>(scriptFunc.mPragmas.size());
						for (const auto& pragma : scriptFunc.mPragmas)
						{
							serializer.write(pragma);
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
				const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
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
				DataTypeHelper::writeDataType(serializer, variable.getDataType());
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
					const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
					const uint64 value = serializer.read<uint64>();
					addConstant(name, dataType, value);
				}
			}
			else
			{
				for (Constant* constant : mConstants)
				{
					constant->getName().write(serializer);
					DataTypeHelper::writeDataType(serializer, constant->getDataType());
					serializer.write(constant->mValue);
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
					const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
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
					DataTypeHelper::writeDataType(serializer, constantArray->getElementDataType());
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
					const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);

					Define& define = addDefine(name, dataType);
					TokenSerializer::serializeTokenList(serializer, define.mContent);
				}
			}
			else
			{
				for (Define* define : mDefines)
				{
					define->getName().serialize(serializer);
					DataTypeHelper::writeDataType(serializer, define->getDataType());

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
