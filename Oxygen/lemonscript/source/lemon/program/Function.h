/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/program/SourceFileInfo.h"
#include "lemon/program/Variable.h"
#include "lemon/utility/FlyweightString.h"


namespace lemon
{
	class Module;
	class ControlFlow;

	class API_EXPORT Function
	{
	friend class Module;
	friend class ModuleSerializer;

	public:
		enum class Type : uint8
		{
			SCRIPT,
			NATIVE
		};

		struct Parameter
		{
			const DataTypeDefinition* mDataType = nullptr;
			FlyweightString mName;
		};
		typedef std::vector<Parameter> ParameterList;

		struct SignatureBuilder
		{
			void clear(const DataTypeDefinition& returnType);
			void addParameterType(const DataTypeDefinition& dataType);
			uint32 getSignatureHash();

			std::vector<uint32> mData;
		};

		enum class Flag
		{
			ALLOW_INLINE_EXECUTION = 0x01,	// Native only: Function can be called directly inside the opcode run loop and does not interfere with control flow
			COMPILE_TIME_CONSTANT  = 0x02	// Native only: Function only does calculation on the parameters, does not read from run-time sources and has no side-effects 
		};

	public:
		static uint32 getVoidSignatureHash();

	public:
		inline Type getType() const  { return mType; }
		inline uint32 getID() const  { return mID; }

		inline BitFlagSet<Flag> getFlags() const  { return mFlags; }
		inline bool hasFlag(Flag flag) const	  { return mFlags.isSet(flag); }

		inline FlyweightString getContext() const { return mContext; }
		inline FlyweightString getName() const    { return mName; }
		inline uint64 getNameAndSignatureHash() const { return mNameAndSignatureHash; }
		inline const std::vector<FlyweightString>& getAliasNames() const { return mAliasNames; }

		const DataTypeDefinition* getReturnType() const  { return mReturnType; }
		const ParameterList& getParameters() const  { return mParameters; }

		uint32 getSignatureHash() const;

	protected:
		inline Function(Type type) : mType(type) {}
		inline virtual ~Function() {}

		void setParametersByTypes(const std::vector<const DataTypeDefinition*>& parameterTypes);

	protected:
		Type mType;
		uint32 mID = 0;
		BitFlagSet<Flag> mFlags;

		// Metadata
		FlyweightString mContext;		// Name of the type if this is a method-like function
		FlyweightString mName;
		uint64 mNameAndSignatureHash = 0;
		std::vector<FlyweightString> mAliasNames;

		// Signature
		const DataTypeDefinition* mReturnType = &PredefinedDataTypes::VOID;
		ParameterList mParameters;
		mutable uint32 mSignatureHash = 0;
	};


	class ScriptFunction : public Function
	{
	public:
		struct Label
		{
			FlyweightString mName;
			uint32 mOffset = 0;
		};

	public:
		inline ScriptFunction() : Function(Type::SCRIPT) {}
		~ScriptFunction();

		inline const Module& getModule() const	{ return *mModule; }
		inline void setModule(Module& module)	{ mModule = &module; }

		LocalVariable* getLocalVariableByIdentifier(uint64 nameHash) const;
		LocalVariable& getLocalVariableByID(uint32 id) const;
		LocalVariable& addLocalVariable(FlyweightString name, const DataTypeDefinition* dataType, uint32 lineNumber);

		bool getLabel(FlyweightString labelName, size_t& outOffset) const;
		void addLabel(FlyweightString labelName, size_t offset);
		const Label* findLabelByOffset(size_t offset) const;

		const std::vector<uint32>& getAddressHooks() const  { return mAddressHooks; }

		void addOrProcessPragma(std::string_view pragmaString, bool consumeIfProcessed);
		inline const std::vector<std::string>& getPragmas() const  { return mPragmas; }

		uint64 addToCompiledHash(uint64 hash) const;

	public:
		// Variables
		std::map<uint64, LocalVariable*> mLocalVariablesByIdentifier;
		std::vector<LocalVariable*> mLocalVariablesByID;

		// Code
		std::vector<Opcode> mOpcodes;

		// Labels
		std::vector<Label> mLabels;

		// Address hooks
		std::vector<uint32> mAddressHooks;

		// Pragmas
		std::vector<std::string> mPragmas;

		// Source
		const SourceFileInfo* mSourceFileInfo = nullptr;
		uint32 mStartLineNumber = 0;
		uint32 mSourceBaseLineOffset = 0;	// Offset translating from the full line number (when all includes are fully resolved) to line number inside the original script file

	private:
		Module* mModule = nullptr;
	};


	class NativeFunction : public Function
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

	public:
		inline NativeFunction() : Function(Type::NATIVE) {}
		inline virtual ~NativeFunction()  { delete mFunctionWrapper; }

		void setFunction(const FunctionWrapper& functionWrapper);
		NativeFunction& setParameterInfo(size_t index, const std::string& identifier);

		void execute(const Context context) const;

	public:
		const FunctionWrapper* mFunctionWrapper = nullptr;
	};


}
