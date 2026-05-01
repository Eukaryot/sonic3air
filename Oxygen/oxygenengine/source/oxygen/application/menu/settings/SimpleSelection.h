/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/loui/LouiButton.h"
#include "oxygen/menu/loui/LouiLabel.h"


namespace loui
{
	class SimpleSelection : public Widget
	{
	public:
		SimpleSelection& init(const std::string_view text, FontWrapper& font, Vec2i size);
		SimpleSelection& addOption(std::string_view displayText, int32 value);

		void setValue(int newValue);

		inline bool wasChanged() const  { return mWasChanged; }
		int32 getCurrentOptionValue() const;

		virtual void update(UpdateInfo& updateInfo) override;
		virtual void render(RenderInfo& renderInfo) override;

	protected:
		virtual void applyLayouting() override;

	protected:
		struct Option
		{
			std::string mDisplayText;
			int32 mValue = 0;
		};

	protected:
		Label mTitleLabel;
		Label mValueLabel;
		Button mButtonLeft;
		Button mButtonRight;

		bool mIsHovered = false;
		int mOptionIndex = -1;
		bool mWasChanged = false;

		std::vector<Option> mOptions;
	};
}
