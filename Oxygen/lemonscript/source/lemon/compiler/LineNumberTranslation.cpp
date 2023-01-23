/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/compiler/LineNumberTranslation.h"
#include "lemon/compiler/Utility.h"


namespace lemon
{
	LineNumberTranslation::TranslationResult LineNumberTranslation::translateLineNumber(uint32 lineNumber) const
	{
		TranslationResult result;
		if (mIntervals.empty())
		{
			CHECK_ERROR_NOLINE(false, "Error resolving line number");
			return result;
		}

		// TODO: Possible optimization with binary search
		size_t index = 0;
		while (index+1 < mIntervals.size() && mIntervals[index+1].mStartLineNumber < lineNumber)
			++index;

		const Interval& interval = mIntervals[index];
		result.mSourceFileInfo = interval.mSourceFileInfo;
		result.mLineNumber = lineNumber - interval.mStartLineNumber + interval.mLineOffsetInFile;
		return result;
	}

	void LineNumberTranslation::push(uint32 currentLineNumber, const SourceFileInfo& sourceFileInfo, uint32 lineOffsetInFile)
	{
		const bool updateLastEntry = (!mIntervals.empty() && mIntervals.back().mStartLineNumber == currentLineNumber);
		Interval& interval = updateLastEntry ? mIntervals.back() : vectorAdd(mIntervals);
		interval.mStartLineNumber = currentLineNumber;
		interval.mSourceFileInfo = &sourceFileInfo;
		interval.mLineOffsetInFile = lineOffsetInFile;
	}
}
