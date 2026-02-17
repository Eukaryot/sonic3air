/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/function/Function.h"
#include "lemon/program/Opcode.h"
#include "lemon/program/SourceFileInfo.h"
#include "lemon/program/Variable.h"


namespace lemon
{
	class Module;


	class ScriptFunction : public Function
	{
	public:
		static const Type TYPE = Type::SCRIPT;

	public:
		struct AddressHook
		{
			uint32 mAddress = 0;
			bool mDisabled = false;
		};

		struct Label
		{
			FlyweightString mName;
			uint32 mOffset = 0;
			std::vector<AddressHook> mLabelAddressHooks;
		};

	public:
		inline ScriptFunction() : Function(Type::SCRIPT) {}
		~ScriptFunction();

		inline const Module& getModule() const	{ return *mModule; }
		inline void setModule(Module& module)	{ mModule = &module; }

		LocalVariable* getLocalVariableByIdentifier(uint64 nameHash) const;
		LocalVariable& getLocalVariableByID(uint32 id) const;
		LocalVariable& addLocalVariable(FlyweightString name, const DataTypeDefinition* dataType, uint32 lineNumber);

		const std::vector<Label>& getLabels() const  { return mLabels; }
		const Label* findLabelByName(FlyweightString labelName) const;
		const Label* findLabelByOffset(size_t offset) const;
		void addLabel(FlyweightString labelName, size_t offset, const std::vector<AddressHook>& addressHooks);

		const std::vector<AddressHook>& getAddressHooks() const  { return mAddressHooks; }

		void addOrProcessPragma(std::string_view pragmaString, bool consumeIfProcessed);
		inline const std::vector<std::string>& getPragmas() const  { return mPragmas; }

		uint64 addToCompiledHash(uint64 hash) const;

	public:
		// Variables
		std::map<uint64, LocalVariable*> mLocalVariablesByIdentifier;
		std::vector<LocalVariable*> mLocalVariablesByID;
		size_t mLocalVariablesMemorySize = 0;

		// Code
		std::vector<Opcode> mOpcodes;

		// Labels
		std::vector<Label> mLabels;

		// Address hooks
		std::vector<AddressHook> mAddressHooks;

		// Pragmas
		std::vector<std::string> mPragmas;

		// Source
		const SourceFileInfo* mSourceFileInfo = nullptr;
		uint32 mStartLineNumber = 0;
		uint32 mSourceBaseLineOffset = 0;	// Offset translating from the full line number (when all includes are fully resolved) to line number inside the original script file

	private:
		Module* mModule = nullptr;
	};

}
