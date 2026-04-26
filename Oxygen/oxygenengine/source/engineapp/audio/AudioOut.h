/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

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
	void realtimeUpdate(float secondsPassed) override;
};
