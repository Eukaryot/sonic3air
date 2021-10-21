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

void AudioOut::update(float secondsPassed)
{
}

void AudioOut::realtimeUpdate(float secondsPassed)
{
	// Call base implementation
	AudioOutBase::realtimeUpdate(secondsPassed);
}
