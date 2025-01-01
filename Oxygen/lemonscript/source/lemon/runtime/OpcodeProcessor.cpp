/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/runtime/OpcodeProcessor.h"
#include "lemon/program/Program.h"


namespace lemon
{

	void OpcodeProcessor::buildOpcodeData(std::vector<OpcodeData>& opcodeData, const ScriptFunction& function)
	{
		// Reset
		const std::vector<Opcode>& opcodes = function.mOpcodes;
		const size_t numOpcodes = opcodes.size();
		opcodeData.resize(numOpcodes);

		// Count remaining sequence lengths
		uint8 sequenceLength = 1;
		for (int i = (int)numOpcodes-1; i >= 0; --i)
		{
			if (opcodes[i].mFlags.isSet(Opcode::Flag::CTRLFLOW))
			{
				sequenceLength = 0;
			}
			else if (opcodes[i].mFlags.isSet(Opcode::Flag::SEQ_BREAK))
			{
				sequenceLength = 1;
			}
			else
			{
				if (sequenceLength < 0xff)
					++sequenceLength;
			}
			opcodeData[i].mRemainingSequenceLength = sequenceLength;
		}
	}

}
