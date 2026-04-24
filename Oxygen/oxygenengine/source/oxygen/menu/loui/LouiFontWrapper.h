/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


namespace loui
{
	class FontWrapper
	{
	public:
		FontWrapper(const std::string_view fontKey);

		void setFontKey(const std::string_view fontKey);

		Font* getFont();
		Font& getFontSafe();

	private:
		WeakPtr<Font> mCachedFont;
		std::string mFontKey;
	};
}
