/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Constant.h"
#include "lemon/program/ConstantArray.h"
#include "lemon/program/Define.h"
#include "lemon/program/Function.h"
#include "lemon/program/StringRef.h"
#include "lemon/compiler/PreprocessorDefinition.h"


namespace lemon
{
	class Module;

	class GlobalsLookup
	{
	friend class Module;

	public:
		// Preprocessor definitions
		PreprocessorDefinitionMap mPreprocessorDefinitions;

	public:
		struct Identifier
		{
		public:
			enum class Type
			{
				UNDEFINED,
				VARIABLE,
				CONSTANT,
				CONSTANT_ARRAY,
				DEFINE,
				DATA_TYPE
			};

			inline Type getType() const  { return mType; }
			inline bool isValid() const  { return mType != Type::UNDEFINED; }

			inline void set(Variable* variable)					{ mType = Type::VARIABLE;		mPointer = variable; }
			inline void set(Constant* constant)					{ mType = Type::CONSTANT;		mPointer = constant; }
			inline void set(ConstantArray* constantArray)		{ mType = Type::CONSTANT_ARRAY;	mPointer = constantArray; }
			inline void set(Define* define)						{ mType = Type::DEFINE;			mPointer = define; }
			inline void set(const DataTypeDefinition* dataType)	{ mType = Type::DATA_TYPE;		mPointer = dataType; }

			template<typename T> const T& as() const  { return *static_cast<const T*>(mPointer); }

		private:
			Type mType = Type::UNDEFINED;
			const void* mPointer = nullptr;
		};

		struct FunctionReference
		{
			Function* mFunction = nullptr;
			bool mIsDeprecated = false;
		};

	public:
		GlobalsLookup();

		void clear();
		void addDefinitionsFromModule(const Module& module);

		// All identifiers
		const Identifier* resolveIdentifierByHash(uint64 nameHash) const;

		// Functions
		const std::vector<FunctionReference>& getFunctionsByName(uint64 nameHash) const;
		const FunctionReference* getFunctionByNameAndSignature(uint64 nameHash, uint32 signatureHash, bool* outAnyFound = nullptr) const;
		const std::vector<FunctionReference>& getMethodsByName(uint64 contextNameHash) const;
		void registerFunction(Function& function);

		// Global variables
		void registerGlobalVariable(Variable& variable);

		// Constants
		void registerConstant(Constant& constant);

		// Constant arrays
		void registerConstantArray(ConstantArray& constantArray);

		// Defines
		void registerDefine(Define& define);

		// String literals
		const FlyweightString* getStringLiteralByHash(uint64 hash) const;

		// Data types
		inline const std::vector<const DataTypeDefinition*>& getDataTypes() const  { return mDataTypes; }
		void registerDataType(const CustomDataType* dataTypeDefinition);
		const DataTypeDefinition* readDataType(VectorBinarySerializer& serializer) const;
		void serializeDataType(VectorBinarySerializer& serializer, const DataTypeDefinition*& dataTypeDefinition) const;

	private:
		// All identifiers
		std::unordered_map<uint64, Identifier> mAllIdentifiers;

		// Functions
		std::unordered_map<uint64, std::vector<FunctionReference>> mFunctionsByName;	// Key is the hashed function name
		std::unordered_map<uint64, std::vector<FunctionReference>> mMethodsByName;		// Key is the sum of hashed context name + hashed function name
		uint32 mNextFunctionID = 0;

		// Global variables
		uint32 mNextVariableID = 0;

		// Constant arrays
		uint32 mNextConstantArrayID = 0;

		// String literals
		StringLookup mStringLiterals;

		// Data types
		std::vector<const DataTypeDefinition*> mDataTypes;
	};

}
