/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Errors.h"
#include "lemon/compiler/PreprocessorDefinition.h"


namespace lemon
{
	struct SourceFileInfo;

	struct LineNumberTranslation
	{
		struct Interval
		{
			uint32 mStartLineNumber = 0;	// End line number is the start of the next item minus one
			const SourceFileInfo* mSourceFileInfo = nullptr;
			uint32 mLineOffsetInFile = 0;
		};
		std::vector<Interval> mIntervals;

		struct TranslationResult
		{
			const SourceFileInfo* mSourceFileInfo = nullptr;
			uint32 mLineNumber = 0;
		};

		TranslationResult translateLineNumber(uint32 lineNumber) const;
		void push(uint32 currentLineNumber, const SourceFileInfo& sourceFileInfo, uint32 lineOffsetInFile);
	};
}
