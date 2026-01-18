/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	struct Opcode;
	class ScriptFunction;

	class OpcodeOptimization
	{
	public:
		OpcodeOptimization(ScriptFunction& function, std::vector<Opcode>& opcodes) : mFunction(function), mOpcodes(opcodes) {}

		void optimizeOpcodes();

	private:
		void cleanupNOPs();

	private:
		ScriptFunction& mFunction;
		std::vector<Opcode>& mOpcodes;
	};

}
