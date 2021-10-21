#pragma once

#include "oxygen/application/audio/AudioOutBase.h"


class AudioOut final : public AudioOutBase, public SingleInstance<AudioOut>
{
public:
	AudioOut();
	~AudioOut();

	void startup() override;
	void shutdown() override;
	void reset() override;
	void resetGame() override;

	void update(float secondsPassed) override;
	void realtimeUpdate(float secondsPassed) override;
};
