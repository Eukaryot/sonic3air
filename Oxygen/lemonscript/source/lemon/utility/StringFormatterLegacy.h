/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	struct AnyTypeWrapper;
	namespace detail
	{
		class FastStringStream;
	}


	struct StringFormatterLegacy
	{
		static void buildFormattedString(detail::FastStringStream& output, std::string_view formatString, size_t numArguments, const AnyTypeWrapper* args);
	};
}
