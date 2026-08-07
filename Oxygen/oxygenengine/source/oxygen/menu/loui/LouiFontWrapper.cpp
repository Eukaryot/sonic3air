/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiFontWrapper.h"
#include "oxygen/resources/FontCollection.h"


namespace loui
{
	FontWrapper::FontWrapper(const std::string_view fontKey)
	{
		setFontKey(fontKey);
	}

	void FontWrapper::setFontKey(const std::string_view fontKey)
	{
		mFontKey = fontKey;
		mCachedFont.clear();
	}

	Font* FontWrapper::getFont()
	{
		if (!mCachedFont.isValid())
		{
			mCachedFont = FontCollection::instance().createFontByKey(mFontKey);
		}
		return mCachedFont.get();
	}

	Font& FontWrapper::getFontSafe()
	{
		static Font FALLBACK;
		Font* font = getFont();
		return (nullptr != font) ? *font : FALLBACK;
	}
}
