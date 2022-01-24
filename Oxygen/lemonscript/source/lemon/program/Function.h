/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/program/Variable.h"


namespace lemon
{
	class Module;
	class ControlFlow;

	class API_EXPORT Function
	{
	friend class Module;

	public:
		enum class Type : uint8
		{
			SCRIPT,
			USER
		};
		struct Parameter
		{
			const DataTypeDefinition* mType = nullptr;
			std::string mIdentifier;
		};
		typedef std::vector<Parameter> ParameterList;

	public:
		static uint32 getVoidSignatureHash();

	public:
		inline Type getType() const  { return mType; }
		inline uint32 getId() const  { return mId; }

		inline const std::string& getName() const { return mName; }
		inline uint64 getNameHash() const { return mNameHash; }
		inline uint64 getNameAndSignatureHash() const { return mNameAndSignatureHash; }

		const DataTypeDefinition* getReturnType() const  { return mReturnType; }
		const ParameterList& getParameters() const  { return mParameters; }

		uint32 getSignatureHash() const;

	protected:
		inline Function(Type type) : mType(type) {}
		inline virtual ~Function() {}

		void setParametersByTypes(const std::vector<const DataTypeDefinition*>& parameterTypes);

	protected:
		Type mType;
		uint32 mId = 0;

		// Metadata
		std::string mName;
		uint64 mNameHash = 0;
		uint64 mNameAndSignatureHash = 0;

		// Signature
		const DataTypeDefinition* mReturnType = &PredefinedDataTypes::VOID;
		ParameterList mParameters;
		mutable uint32 mSignatureHash = 0;
	};


	class ScriptFunction : public Function
	{
	public:
		inline ScriptFunction() : Function(Type::SCRIPT) {}
		~ScriptFunction();

		inline const Module& getModule() const	{ return *mModule; }
		inline void setModule(Module& module)	{ mModule = &module; }

		LocalVariable* getLocalVariableByIdentifier(const std::string& identifier) const;
		LocalVariable& getLocalVariableById(uint32 id) const;
		LocalVariable& addLocalVariable(const std::string& identifier, const DataTypeDefinition* dataType, uint32 lineNumber);

		bool getLabel(const std::string& labelName, size_t& outOffset) const;
		void addLabel(const std::string& labelName, size_t offset);
		const std::string* findLabelByOffset(size_t offset) const;

		inline const std::vector<std::string>& getPragmas() const  { return mPragmas; }

	public:
		// Variables
		std::map<std::string, LocalVariable*> mLocalVariablesByIdentifier;
		std::vector<LocalVariable*> mLocalVariablesById;

		// Code
		std::vector<Opcode> mOpcodes;

		// Labels
		std::map<std::string, uint32> mLabels;

		// Pragmas
		std::vector<std::string> mPragmas;

		// Source
		std::wstring mSourceFilename;
		uint32 mStartLineNumber = 0;
		uint32 mSourceBaseLineOffset = 0;	// Offset translating from the full line number (when all includes are fully resolved) to line number inside the original script file

	private:
		Module* mModule = nullptr;
	};


	class UserDefinedFunction : public Function
	{
	public:
		struct Context
		{
			inline explicit Context(ControlFlow& controlFlow) : mControlFlow(controlFlow) {}
			ControlFlow& mControlFlow;
		};

		class FunctionWrapper
		{
		public:
			virtual ~FunctionWrapper() {}
			virtual void execute(const Context context) const = 0;
			virtual const DataTypeDefinition* getReturnType() const = 0;
			virtual std::vector<const DataTypeDefinition*> getParameterTypes() const = 0;
		};

		enum Flags
		{
			FLAG_ALLOW_INLINE_EXECUTION = 0x01
		};

	public:
		inline UserDefinedFunction() : Function(Type::USER) {}
		inline virtual ~UserDefinedFunction()  { delete mFunctionWrapper; }

		void setFunction(const FunctionWrapper& functionWrapper);
		UserDefinedFunction& setParameterInfo(size_t index, const std::string& identifier);

		void execute(const Context context) const;

	public:
		const FunctionWrapper* mFunctionWrapper = nullptr;
		uint8 mFlags = 0;
	};


}
