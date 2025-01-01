/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/options/OptionsEntry.h"


class OptionsConfig
{
public:
	struct Setting
	{
		struct Option
		{
			std::string mName;
			uint32 mValue = 0;
			inline Option(std::string_view name, uint32 value) : mName(name), mValue(value) {}
		};

		option::Option mKey;
		std::string mName;
		std::vector<Option> mOptions;
	};

	struct OptionsCategory
	{
		std::string mName;
		std::vector<Setting> mSettings;
	};

	struct OptionsTab
	{
		std::vector<OptionsCategory> mCategories;
	};

public:
	OptionsTab mSystemOptions;
	OptionsTab mDisplayOptions;
	OptionsTab mAudioOptions;
	OptionsTab mVisualsOptions;
	OptionsTab mGameplayOptions;
	OptionsTab mControlsOptions;
	OptionsTab mTweaksOptions;

public:
	void build();

private:
	void buildSystem();
	void buildDisplay();
	void buildAudio();
	void buildVisuals();
	void buildGameplay();
	void buildControls();
	void buildTweaks();
};
