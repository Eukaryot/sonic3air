/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Module.h"
#include "lemon/program/ModuleSerializer.h"
#include "lemon/program/GlobalsLookup.h"

#include <iomanip>


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
		static_assert((size_t)Opcode::Type::_NUM_TYPES == 36);	// Otherwise DEFAULT_OPCODE_BASETYPES needs to get updated
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

		// Data types
		for (const CustomDataType* customDataType : mDataTypes)
		{
			delete customDataType;
		}
		mDataTypes.clear();

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

		if (mDataTypes.empty())
		{
			mFirstDataTypeID = (uint16)globalsLookup.mDataTypes.size();
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
			content << "// Constants";
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

					default:
						break;
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
				// Separate functions with different prefixes
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

	ExternalVariable& Module::addExternalVariable(FlyweightString name, const DataTypeDefinition* dataType, std::function<int64*()>&& accessor)
	{
		// TODO: Add an object pool for this
		ExternalVariable& variable = *new ExternalVariable();
		variable.mAccessor = std::move(accessor);
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

	const CustomDataType* Module::addDataType(const char* name, BaseType baseType)
	{
		const uint16 id = mFirstDataTypeID + (uint16)mDataTypes.size();
		CustomDataType* customDataType = new CustomDataType(name, id, baseType);
		mDataTypes.push_back(customDataType);
		return customDataType;
	}

	uint32 Module::buildDependencyHash() const
	{
		// Just a very simple "hash" that changes when a new definition gets added
		return (uint32)(mFunctions.size() + mGlobalVariables.size() + mConstants.size() + mConstantArrays.size() + mDefines.size() + mStringLiterals.size());
	}

	bool Module::serialize(VectorBinarySerializer& outerSerializer, const GlobalsLookup& globalsLookup, uint32 dependencyHash, uint32 appVersion)
	{
		return ModuleSerializer::serialize(*this, outerSerializer, globalsLookup, dependencyHash, appVersion);
	}

}
