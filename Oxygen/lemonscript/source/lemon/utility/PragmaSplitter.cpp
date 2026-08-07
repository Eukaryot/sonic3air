/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/utility/PragmaSplitter.h"


namespace lemon
{
	PragmaSplitter::PragmaSplitter(std::string_view input)
	{
		size_t pos = 0;
		while (pos < input.length())
		{
			// Skip all whitespace
			while (pos < input.length() && (input[pos] == ' ' || input[pos] == '\t'))
				++pos;

			if (pos < input.length())
			{
				const size_t startPos = pos;
				while (pos < input.length() && (input[pos] != ' ' && input[pos] != '\t'))
					++pos;

				mEntries.emplace_back();

				// Split part string into argument and value
				const std::string_view part = input.substr(startPos, pos - startPos);
				const size_t left = part.find_first_of('(');
				if (left != std::string::npos)
				{
					RMX_CHECK(part.back() == ')', "No matching parentheses in pragma found", continue);
					mEntries.back().mArgument = part.substr(0, left);
					mEntries.back().mValue = part.substr(left + 1, part.length() - left - 2);
				}
				else
				{
					mEntries.back().mArgument = part;
				}
			}
		}
	}
}
