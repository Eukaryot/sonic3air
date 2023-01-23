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

	class ParserTokenList;

	class Parser
	{
	public:
		void splitLineIntoTokens(std::string_view input, uint32 lineNumber, ParserTokenList& outTokens);

	private:
		std::string mBufferString;
	};

}
