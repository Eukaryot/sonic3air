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
	class PragmaSplitter
	{
	public:
		struct Entry
		{
			std::string_view mArgument;
			std::string_view mValue;
		};
		std::vector<Entry> mEntries;

	public:
		explicit PragmaSplitter(std::string_view input);
	};
}
