/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/translator/Nativizer.h"


namespace lemon
{
	class API_EXPORT NativizedOpcodeProvider : public RuntimeOpcodeProvider
	{
	public:
		typedef void (*BuildFunction)(Nativizer::LookupDictionary& dict);

	public:
		inline NativizedOpcodeProvider() {}
		inline NativizedOpcodeProvider(BuildFunction buildFunction) { buildLookup(buildFunction); }

		inline bool isValid() const  { return !mLookupDictionary.mEntries.empty(); }
		void buildLookup(BuildFunction buildFunction);

		bool buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime) override;

	protected:
		Nativizer::LookupDictionary mLookupDictionary;	// This needs to be filled by either a sub-class implementation or a call to the buildLookup method
	};
}
