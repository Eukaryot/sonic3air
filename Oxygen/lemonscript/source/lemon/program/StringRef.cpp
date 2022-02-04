/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/StringRef.h"


namespace lemon
{
	void StringLookup::addFromList(const std::vector<FlyweightString>& list)
	{
		for (FlyweightString str : list)
		{
			addString(str);
		}
	}
}
