/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/menu/settings/submenus/AudioMenu.h"
#include "oxygen/application/menu/settings/SimpleSelection.h"
#include "oxygen/application/menu/SharedFonts.h"


void AudioMenu::init()
{
	const Vec2i buttonSize(300, 16);
	loui::FontWrapper& font = SharedFonts::oxyFontSmallShadow;

	const auto setupVolumeSelection = [&](loui::SimpleSelection& selection, const char* text)
	{
		for (int percent = 0; percent <= 100; percent += 5)
		{
			selection.addOption(String(0, "%d %%", percent), percent);
		}
		selection.init(text, font, buttonSize);
		addChildWidget(selection);
	};

	setupVolumeSelection(mMasterVolumeSelection, "Main Volume");
	setupVolumeSelection(mMusicVolumeSelection, "Music");
	setupVolumeSelection(mSoundVolumeSelection, "Sound Effects");

	const Configuration& config = Configuration::instance();

	mMasterVolumeSelection.setCurrentOptionByValue(roundToInt(config.mAudio.mMasterVolume * 100.0f));
	mMusicVolumeSelection.setCurrentOptionByValue(roundToInt(config.mAudio.mMusicVolume * 100.0f));
	mSoundVolumeSelection.setCurrentOptionByValue(roundToInt(config.mAudio.mSoundVolume * 100.0f));
}

void AudioMenu::update(loui::UpdateInfo& updateInfo)
{
	loui::VerticalLayout::update(updateInfo);

	Configuration& config = Configuration::instance();

	if (mMasterVolumeSelection.wasChanged())
	{
		config.mAudio.mMasterVolume = (float)mMasterVolumeSelection.getCurrentOptionValue() / 100.0f;
	}

	if (mMusicVolumeSelection.wasChanged())
	{
		config.mAudio.mMusicVolume = (float)mMusicVolumeSelection.getCurrentOptionValue() / 100.0f;
	}

	if (mSoundVolumeSelection.wasChanged())
	{
		config.mAudio.mSoundVolume = (float)mSoundVolumeSelection.getCurrentOptionValue() / 100.0f;
	}
}
