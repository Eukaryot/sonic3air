/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/devmode/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/audio/AudioCollection.h"
#include "oxygen/devmode/DevModeWindowBase.h"


class AudioBrowserWindow : public DevModeWindowBase
{
public:
	AudioBrowserWindow();

	virtual void buildContent() override;

private:
	std::vector<const AudioCollection::AudioDefinition*> mAudioDefinitions;
	uint32 mLastAudioCollectionChangeCounter = 0;

	AudioReference mPlayingAudio;
	const AudioCollection::AudioDefinition* mPlayingDefinition = nullptr;
};

#endif
