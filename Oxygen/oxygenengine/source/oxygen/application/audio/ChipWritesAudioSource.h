/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/audio/AudioSourceBase.h"
#include "oxygen/simulation/sound/SoundEmulation.h"


class ChipWritesAudioSource : public AudioSourceBase
{
public:
	explicit ChipWritesAudioSource(bool useCaching);
	~ChipWritesAudioSource();

	bool load(std::wstring_view filename);

protected:
	virtual void resetInternal() override;
	virtual State startupInternal() override;
	virtual void progressInternal(float targetTime) override;

protected:
	virtual bool jobFunc() override;

private:
	std::vector<std::vector<SoundChipWrite>> mSoundChipWritesByFrame;

	SoundEmulation mSoundEmulation;
	size_t mCurrentFrame = 0;

	float mPrecacheTime = 0.0f;
};
