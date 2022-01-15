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
			mFirstFunctionId = globalsLookup.mNextFunctionId;
		}

		if (mGlobalVariables.empty())
		{
			// This here only makes sense if no global variables got set previously
			//  -> That's because the existing global variables are likely part of the globals lookup already
			//  -> Unfortunately, this is only safe to assume for the first module -- TODO: How to handle other cases?
			mFirstVariableId = globalsLookup.mNextVariableId;
		}
	}

	void Module::dumpDefinitionsToScriptFile(const std::wstring& filename)
	{
		String content;
		content << "// This file was auto-generated from the definitions in lemon script module '" << getModuleName() << "'.\r\n";
		content << "\r\n";
		
		for (const Function* function : mFunctions)
		{
			content << "\r\n";
			content << "declare function " << function->getReturnType()->toString() << " " << function->getName() << "(";
			for (size_t i = 0; i < function->getParameters().size(); ++i)
			{
				if (i != 0)
					content << ", ";
				const Function::Parameter& parameter = function->getParameters()[i];
				content << parameter.mType->toString();
				if (!parameter.mIdentifier.empty())
					content << " " << parameter.mIdentifier;
			}
			content << ")\r\n";
		}

		content.saveFile(filename);
	}

	const Function* Module::getFunctionByUniqueId(uint64 uniqueId) const
	{
		RMX_ASSERT(mModuleId == (uniqueId & 0xffffffffffff0000ull), "Function unique ID is not valid for this module");
		return mFunctions[uniqueId & 0xffff];
	}

	ScriptFunction& Module::addScriptFunction(const std::string& name, const DataTypeDefinition* returnType, const Function::ParameterList* parameters)
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

	UserDefinedFunction& Module::addUserDefinedFunction(const std::string& name, const UserDefinedFunction::FunctionWrapper& functionWrapper, uint8 flags)
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
		func.mId = mFirstFunctionId + (uint32)mFunctions.size();
		func.mNameHash = rmx::getMurmur2_64(func.mName);
		func.mNameAndSignatureHash = func.mNameHash + func.getSignatureHash();
		mFunctions.push_back(&func);
	}

	GlobalVariable& Module::addGlobalVariable(const std::string& identifier, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		GlobalVariable& variable = *new GlobalVariable();
		addGlobalVariable(variable, identifier, dataType);
		return variable;
	}

	UserDefinedVariable& Module::addUserDefinedVariable(const std::string& identifier, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		UserDefinedVariable& variable = *new UserDefinedVariable();
		addGlobalVariable(variable, identifier, dataType);
		return variable;
	}

	ExternalVariable& Module::addExternalVariable(const std::string& identifier, const DataTypeDefinition* dataType)
	{
		// TODO: Add an object pool for this
		ExternalVariable& variable = *new ExternalVariable();
		addGlobalVariable(variable, identifier, dataType);
		return variable;
	}

	void Module::addGlobalVariable(Variable& variable, const std::string& name, const DataTypeDefinition* dataType)
	{
		variable.mName = name;
		variable.mNameHash = rmx::getMurmur2_64(name);
		variable.mDataType = dataType;
		variable.mId = mFirstVariableId + (uint32)mGlobalVariables.size() + ((uint32)variable.mType << 28);
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

	Constant& Module::addConstant(const std::string& name, const DataTypeDefinition* dataType, uint64 value)
	{
		Constant& constant = mConstantPool.createObject();
		constant.mName = name;
		constant.mDataType = dataType;
		constant.mValue = value;
		mConstants.emplace_back(&constant);
		return constant;
	}

	Define& Module::addDefine(const std::string& name, const DataTypeDefinition* dataType)
	{
		Define& define = mDefinePool.createObject();
		define.mName = name;
		define.mDataType = dataType;
		mDefines.emplace_back(&define);
		return define;
	}

	const StoredString* Module::addStringLiteral(const std::string& str)
	{
		return &mStringLiterals.getOrAddString(str);
	}

	const StoredString* Module::addStringLiteral(const std::string& str, uint64 hash)
	{
		return &mStringLiterals.getOrAddString(str, hash);
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

		// Signature and version number
		const uint32 SIGNATURE = *(uint32*)"LMD|";
		uint16 version = 0x06;
		if (outerSerializer.isReading())
		{
			const uint32 signature = *(const uint32*)outerSerializer.peek();
			if (signature != SIGNATURE)
				return false;

			outerSerializer.skip(4);
			version = outerSerializer.read<uint16>();
			if (version < 0x06)
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
		serializer & mFirstFunctionId;
		serializer & mFirstVariableId;

		// Serialize functions
		{
			uint32 numberOfFunctions = (uint32)mFunctions.size();
			serializer & numberOfFunctions;

			for (uint32 i = 0; i < numberOfFunctions; ++i)
			{
				if (serializer.isReading())
				{
					const Function::Type type = (Function::Type)serializer.read<uint8>();
					const std::string name = serializer.read<std::string>();

					const DataTypeDefinition* returnType = DataTypeHelper::readDataType(serializer);
					Function::ParameterList parameters;
					const uint8 parameterCount = serializer.read<uint8>();
					parameters.resize((size_t)parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						serializer.serialize(parameters[k].mIdentifier);
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
							const std::string identifier = serializer.read<std::string>();
							const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
							scriptFunc.addLocalVariable(identifier, dataType, 0);
						}

						// Labels
						count = (size_t)serializer.read<uint32>();
						for (size_t k = 0; k < count; ++k)
						{
							const std::string name = serializer.read<std::string>();
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
					serializer.write(function.mName);

					DataTypeHelper::writeDataType(serializer, function.mReturnType);
					const uint8 parameterCount = (uint8)function.mParameters.size();
					serializer.write(parameterCount);
					for (uint8 k = 0; k < parameterCount; ++k)
					{
						serializer.write(function.mParameters[k].mIdentifier);
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

							const uint16 typeAndFlags = (uint16)opcode.mType | ((uint16)parameterBits << 6) | (hasDataType ? 0x200 : 0) | (hasOpcodeFlags ? 0x400 : 0) | ((uint16)lineNumberBits << 11);
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
						serializer.writeAs<uint32>(scriptFunc.mLocalVariablesById.size());
						for (const LocalVariable* var : scriptFunc.mLocalVariablesById)
						{
							serializer.write(var->getName());
							DataTypeHelper::writeDataType(serializer, var->getDataType());
						}

						// Labels
						serializer.writeAs<uint32>(scriptFunc.mLabels.size());
						for (const auto& pair : scriptFunc.mLabels)
						{
							serializer.write(pair.first);
							serializer.write(pair.second);
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
				const std::string name = serializer.read<std::string>();
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

				serializer.write(variable.getName());
				DataTypeHelper::writeDataType(serializer, variable.getDataType());
				serializer.writeAs<int64>(globalVariable.mInitialValue);
			}
		}

		// Serialize constants
		{
			uint32 numberOfConstants = (uint32)mConstants.size();
			serializer & numberOfConstants;

			if (serializer.isReading())
			{
				for (uint32 i = 0; i < numberOfConstants; ++i)
				{
					const std::string name = serializer.read<std::string>();
					const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);
					const uint64 value = serializer.read<uint64>();
					Constant& constant = addConstant(name, dataType, value);
				}
			}
			else
			{
				for (Constant* constant : mConstants)
				{
					serializer.write(constant->getName());
					DataTypeHelper::writeDataType(serializer, constant->getDataType());
					serializer.write(constant->mValue);
				}
			}
		}

		// Serialize defines
		{
			uint32 numberOfDefines = (uint32)mDefines.size();
			serializer & numberOfDefines;

			if (serializer.isReading())
			{
				for (uint32 i = 0; i < numberOfDefines; ++i)
				{
					const std::string name = serializer.read<std::string>();
					const DataTypeDefinition* dataType = DataTypeHelper::readDataType(serializer);

					Define& define = addDefine(name, dataType);
					TokenSerializer::serializeTokenList(serializer, define.mContent);
				}
			}
			else
			{
				for (Define* define : mDefines)
				{
					serializer.write(define->getName());
					DataTypeHelper::writeDataType(serializer, define->getDataType());

					TokenSerializer::serializeTokenList(serializer, define->mContent);
				}
			}
		}

		// Serialize string literals
		mStringLiterals.serialize(serializer);

		if (!outerSerializer.isReading())
		{
			std::vector<uint8> compressed;
			if (!ZlibDeflate::encode(compressed, &uncompressed[0], uncompressed.size()))
				return false;
			outerSerializer.write(&compressed[0], compressed.size());
		}

		return true;
	}

}
