/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/RuntimeOpcode.h"


namespace lemon
{
	class OptimizedOpcodeProvider final : public RuntimeOpcodeProvider
	{
	public:
		static bool buildRuntimeOpcodeStatic(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime);

	public:
		bool buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime) override;
	};
}
