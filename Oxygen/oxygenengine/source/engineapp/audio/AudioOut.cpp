/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "engineapp/pch.h"
#include "engineapp/audio/AudioOut.h"


AudioOut::AudioOut()
{
}

AudioOut::~AudioOut()
{
}

void AudioOut::startup()
{
	// Call base implementation
	AudioOutBase::startup();
}

void AudioOut::shutdown()
{
	// Call base implementation
	AudioOutBase::shutdown();
}

void AudioOut::reset()
{
}

void AudioOut::resetGame()
{
}

void AudioOut::realtimeUpdate(float secondsPassed)
{
	// Call base implementation
	AudioOutBase::realtimeUpdate(secondsPassed);
}
