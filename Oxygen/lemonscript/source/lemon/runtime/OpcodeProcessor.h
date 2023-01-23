/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	class ScriptFunction;

	class OpcodeProcessor
	{
	public:
		struct OpcodeData
		{
			uint8 mRemainingSequenceLength = 1;
		};

	public:
		static void buildOpcodeData(std::vector<OpcodeData>& opcodeData, const ScriptFunction& function);
	};

}
