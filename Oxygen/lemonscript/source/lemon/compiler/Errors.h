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
	struct CompilerError
	{
		enum class Code
		{
			UNDEFINED,						// This is actually used for most errors
			SCRIPT_FEATURE_LEVEL_TOO_HIGH,	// Script requested a higher script feature level than the compiler offers; mData1 is the requested level, mData2 is the maximum available level
		};

		Code mCode = Code::UNDEFINED;
		uint64 mData1 = 0;
		uint64 mData2 = 0;
		uint32 mLineNumber = 0;
	};
}
