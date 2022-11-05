/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/PreprocessorDefinition.h"


namespace lemon
{
	class Parser;
	class ParserTokenList;
	class StatementToken;
	class TokenProcessing;
	struct CompileOptions;

	class Preprocessor
	{
	public:
		Preprocessor(const CompileOptions& compileOptions, TokenProcessing& tokenProcessing);

		void processLines(std::vector<std::string_view>& lines);

	public:
		PreprocessorDefinitionMap* mPreprocessorDefinitions = nullptr;
		ObjectPool<std::string, 64> mModifiedLines;		// Buffer needed to store modified lines

	private:
		void eraseFromLine(std::string_view& line, std::string*& modifiedLine, size_t offset, size_t count);
		bool evaluateConditionString(const char* characters, size_t len, Parser& parser);
		void processDefinition(const char* characters, size_t len, Parser& parser);
		int64 evaluateConstantExpression(const ParserTokenList& parserTokens) const;
		int64 evaluateConstantToken(const StatementToken& token) const;

	private:
		const CompileOptions& mCompileOptions;
		TokenProcessing& mTokenProcessing;
		std::string mBufferString;
		uint32 mLineNumber;
	};

}
