/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Define.h"
#include "lemon/compiler/TokenTypes.h"


namespace lemon
{
	void Define::invalidateResolvedIdentifiers()
	{
		for (size_t k = 0; k < mContent.size(); ++k)
		{
			if (mContent[k].isA<IdentifierToken>())
			{
				mContent[k].as<IdentifierToken>().mResolved = nullptr;
			}
		}
	}
}
