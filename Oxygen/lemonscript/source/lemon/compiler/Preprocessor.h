/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/PreprocessorDefinition.h"


namespace lemon
{
	class Parser;
	class StatementToken;

	class Preprocessor
	{
	public:
		Preprocessor();

		void processLines(std::vector<std::string_view>& lines);

	public:
		const PreprocessorDefinitionMap* mPreprocessorDefinitions = nullptr;
		ObjectPool<std::string, 64> mModifiedLines;		// Buffer needed to store modified lines

	private:
		void eraseFromLine(std::string_view& line, std::string*& modifiedLine, size_t offset, size_t count);
		bool evaluateConditionString(const char* characters, size_t len, Parser& parser);
		int64 evaluateConditionToken(const StatementToken& token) const;

	private:
		std::string mBufferString;
		uint32 mLineNumber;
	};

}
