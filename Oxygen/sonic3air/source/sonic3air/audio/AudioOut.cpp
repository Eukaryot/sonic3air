/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/audio/AudioOut.h"
#include "sonic3air/audio/CustomAudioMixer.h"
#include "sonic3air/data/SharedDatabase.h"
#include "sonic3air/ConfigurationImpl.h"

#include "oxygen/application/audio/AudioPlayer.h"
#include "oxygen/application/Application.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/simulation/EmulatorInterface.h"


namespace
{
	static const constexpr float FASTMUSIC_SPEED_FACTOR = 1.25f;
}


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

	// Switch soundtrack selection if remastered soundtrack is not available
	if (!mLoadedRemasteredSoundtrack)
	{
		ConfigurationImpl::instance().mActiveSoundtrack = 0;
	}

	// Replace the ingame master audio mixer
	mIngameAudioMixer = &FTX::Audio->createAudioMixer<CustomAudioMixer>((int)AudioMixerId::INGAME_MASTER, (int)AudioMixerId::ROOT);
}

void AudioOut::shutdown()
{
	reset();

	// Call base implementation
	AudioOutBase::shutdown();
}

void AudioOut::reset()
{
	mAudioPlayer.stopAllSounds();
	resetGame();
}

void AudioOut::resetGame()
{
	mAudioPlayer.resetChannelOverrides();
	mAudioPlayer.resetAudioModifiers();
}

void AudioOut::playAudioDirect(uint64 sfxId, SoundRegType type, int contextBase, AudioReference* outAudioReference)
{
	SfxHandling handling;
	handling.mSoundReg = mAudioCollection.getSourceRegistration(sfxId);
	handling.mType = type;
	handling.mClearMusic = false;
	if (nullptr != handling.mSoundReg)
	{
		playAudioInternal(handling, sfxId, contextBase, outAudioReference);
	}
}

void AudioOut::setMenuMusic(uint64 sfxId)
{
	if (!isPlayingSfxId(sfxId))
	{
		playAudioDirect(sfxId, SoundRegType::MUSIC, CONTEXT_MENU + CONTEXT_MUSIC);
		mMenuMusicId = sfxId;
	}
}

void AudioOut::restartMenuMusic()
{
	stopSoundContext(CONTEXT_MENU + CONTEXT_MUSIC);
	playAudioDirect(mMenuMusicId, SoundRegType::MUSIC, CONTEXT_MENU + CONTEXT_MUSIC);
}

void AudioOut::moveMenuMusicToIngame()
{
	AudioReference audioRef;
	if (mAudioPlayer.getAudioRefByContext(CONTEXT_MENU + CONTEXT_MUSIC, audioRef))
	{
		mAudioPlayer.changeSoundContext(audioRef, CONTEXT_INGAME + CONTEXT_MUSIC);
	}
}

void AudioOut::moveIngameMusicToMenu()
{
	AudioReference audioRef;
	if (mAudioPlayer.getAudioRefByContext(CONTEXT_INGAME + CONTEXT_MUSIC, audioRef))
	{
		mAudioPlayer.changeSoundContext(audioRef, CONTEXT_MENU + CONTEXT_MUSIC);
	}
}

void AudioOut::pauseSoundContext(int contextId)
{
	mAudioPlayer.pauseAllSoundsByContext(contextId);
}

void AudioOut::resumeSoundContext(int contextId)
{
	mAudioPlayer.resumeAllSoundsByContext(contextId);
}

void AudioOut::stopSoundContext(int contextId)
{
	mAudioPlayer.stopAllSoundsByContext(contextId);
}

void AudioOut::onSoundtrackPreferencesChanged()
{
	determineActiveSourceRegistrations();
}

void AudioOut::enableUnderwaterEffect(float value)
{
	if (nullptr != mIngameAudioMixer)
	{
		if (value > 0.0f)
		{
			const int effect = roundToInt(value * 32.0f);
			const float volume = interpolate(1.0f, 2.0f, value);
			mIngameAudioMixer->setUnderwaterEffect(effect, volume);
		}
		else
		{
			mIngameAudioMixer->setUnderwaterEffect(0, 1.0f);
		}
	}
}

void AudioOut::determineActiveSourceRegistrations()
{
	const bool preferOriginal = (ConfigurationImpl::instance().mActiveSoundtrack != 1);
	mAudioCollection.determineActiveSourceRegistrations(preferOriginal);
}

void AudioOut::playAudioInternal(const SfxHandling& handling, uint64 sfxId, int contextBase, AudioReference* outAudioReference)
{
	int contextId = contextBase & 0xf0;
	if (handling.mType == SoundRegType::MUSIC || handling.mType == SoundRegType::JINGLE)
	{
		contextId |= CONTEXT_MUSIC;
	}
	else
	{
		contextId |= CONTEXT_SOUND;
	}
	mAudioPlayer.playAudio(sfxId, contextId);
}
