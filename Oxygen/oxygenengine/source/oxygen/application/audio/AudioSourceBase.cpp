/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/audio/AudioSourceBase.h"


AudioBuffer* AudioSourceBase::startup(float precacheTime)
{
	if (mState == State::INACTIVE || isDynamic())
	{
		// Initial startup or reset from scratch
		mState = startupInternal();
		mReadTime = 0.0f;
		progress(precacheTime);
	}
	return &mAudioBuffer;
}

void AudioSourceBase::progress(float precacheTime)
{
	if (mState == State::STREAMING)
	{
		progressInternal(precacheTime);
	}
}
