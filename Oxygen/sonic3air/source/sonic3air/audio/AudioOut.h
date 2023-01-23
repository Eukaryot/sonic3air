/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>
#include "oxygen/application/audio/AudioCollection.h"
#include "oxygen/application/audio/AudioOutBase.h"


class CustomAudioMixer;

class AudioOut final : public AudioOutBase, public SingleInstance<AudioOut>
{
public:
	enum class SoundRegType
	{
		MUSIC   = 0,
		JINGLE  = 1,
		SOUND   = 2
	};

public:
	AudioOut();
	~AudioOut();

	void startup() override;
	void shutdown() override;
	void reset() override;
	void resetGame() override;

	void update(float secondsPassed) override;
	void realtimeUpdate(float secondsPassed) override;

	void playAudioDirect(uint64 sfxId, SoundRegType type, int contextBase = CONTEXT_INGAME, AudioReference* outAudioReference = nullptr);

	void setMenuMusic(uint64 sfxId);
	void restartMenuMusic();
	void moveMenuMusicToIngame();
	void moveIngameMusicToMenu();

	void pauseSoundContext(int contextId);
	void resumeSoundContext(int contextId);
	void stopSoundContext(int contextId);

	void onSoundtrackPreferencesChanged();
	void enableUnderwaterEffect(float value);

protected:
	void determineActiveSourceRegistrations() override;

private:
	struct SfxHandling
	{
		AudioCollection::SourceRegistration* mSoundReg = nullptr;
		SoundRegType mType = SoundRegType::MUSIC;
		bool mClearMusic = false;
	};

private:
	void playAudioInternal(const SfxHandling& handling, uint64 sfxId, int contextBase, AudioReference* outAudioReference);

private:
	float			  mMusicVolume = 1.0f;
	float			  mSoundVolume = 1.0f;

	std::set<uint32>  mPausedContexts;
	uint64			  mMenuMusicId = -1;
	CustomAudioMixer* mIngameAudioMixer = nullptr;
};
