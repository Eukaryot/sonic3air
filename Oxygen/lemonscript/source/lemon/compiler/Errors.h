/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	struct SourceFileInfo;

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

	struct CompilerWarning
	{
		enum class Code
		{
			UNDEFINED					= 0,		// Should not be used at all
			DEPRECATED_FUNCTION			= 0x0100,
			DEPRECATED_FUNCTION_ALIAS	= 0x0101,
		};

		struct Occurrence
		{
			const SourceFileInfo* mSourceFileInfo = nullptr;
			uint32 mLineNumber = 0;
		};

		Code mCode = Code::UNDEFINED;
		std::string mMessage;
		uint64 mMessageHash = 0;
		std::vector<Occurrence> mOccurrences;
	};
}
