/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/bindings/AudioBindings.h"
#include "oxygen/simulation/bindings/LemonScriptBindings.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"

#include <lemon/program/ModuleBindingsBuilder.h>


namespace
{
	uint8 Audio_getAudioKeyType(uint64 audioKey)
	{
		return (uint8)EngineMain::instance().getAudioOut().getAudioKeyType(audioKey);
	}

	bool Audio_isPlayingAudio(uint64 audioKey)
	{
		return EngineMain::instance().getAudioOut().isPlayingAudioKey(audioKey);
	}

	void Audio_playAudio1(uint64 audioKey, uint8 contextId)
	{
		const bool success = EngineMain::instance().getAudioOut().playAudioBase(audioKey, contextId);
		if (!success)
		{
			// Audio collections expect lowercase IDs, so we might need to do the conversion here first
			lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
			if (nullptr != runtime)
			{
				const lemon::FlyweightString* str = runtime->resolveStringByKey(audioKey);
				if (nullptr != str)
				{
					const std::string_view textString = str->getString();

					// Does the string contain any uppercase letters?
					if (containsByPredicate(textString, [](char ch) { return (ch >= 'A' && ch <= 'Z'); } ))
					{
						// Convert to lowercase and try again
						String tempStr = textString;
						tempStr.lowerCase();
						audioKey = rmx::getMurmur2_64(tempStr);
						EngineMain::instance().getAudioOut().playAudioBase(audioKey, contextId);
					}
				}
			}
		}
	}

	void Audio_playAudio2(uint64 audioKey)
	{
		Audio_playAudio1(audioKey, 0x01);	// In-game sound effect context
	}

	void Audio_pauseChannel(uint8 channel)
	{
		AudioPlayer::instance().pauseAllSoundsByChannel(channel);
	}

	void Audio_resumeChannel(uint8 channel)
	{
		AudioPlayer::instance().resumeAllSoundsByChannel(channel);
	}

	void Audio_stopChannel(uint8 channel)
	{
		AudioPlayer::instance().stopAllSoundsByChannel(channel);
	}

	void Audio_pauseContext(uint8 contextId)
	{
		AudioPlayer::instance().pauseAllSoundsByContext(contextId);
	}

	void Audio_resumeContext(uint8 contextId)
	{
		AudioPlayer::instance().resumeAllSoundsByContext(contextId);
	}

	void Audio_stopContext(uint8 contextId)
	{
		AudioPlayer::instance().stopAllSoundsByContext(contextId);
	}

	void Audio_fadeInChannel(uint8 channel, float seconds)
	{
		EngineMain::instance().getAudioOut().fadeInChannel(channel, seconds);
	}

	void Audio_fadeInChannel2(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeInChannel(channel, (float)length / 256.0f);
	}

	void Audio_fadeOutChannel(uint8 channel, float seconds)
	{
		EngineMain::instance().getAudioOut().fadeOutChannel(channel, seconds);
	}

	void Audio_fadeOutChannel2(uint8 channel, uint16 length)
	{
		EngineMain::instance().getAudioOut().fadeOutChannel(channel, (float)length / 256.0f);
	}

	void Audio_playOverride(uint64 audioKey, uint8 contextId, uint8 channelId, uint8 overriddenChannelId)
	{
		EngineMain::instance().getAudioOut().playOverride(audioKey, contextId, channelId, overriddenChannelId);
	}

	void Audio_enableAudioModifier(uint8 channel, uint8 contextId, lemon::StringRef postfix, float relativeSpeed)
	{
		if (postfix.isValid())
		{
			EngineMain::instance().getAudioOut().enableAudioModifier(channel, contextId, postfix.getString(), relativeSpeed);
		}
	}

	void Audio_enableAudioModifier2(uint8 channel, uint8 contextId, lemon::StringRef postfix, uint32 relativeSpeed)
	{
		if (postfix.isValid())
		{
			EngineMain::instance().getAudioOut().enableAudioModifier(channel, contextId, postfix.getString(), (float)relativeSpeed / 65536.0f);
		}
	}

	void Audio_disableAudioModifier(uint8 channel, uint8 contextId)
	{
		EngineMain::instance().getAudioOut().disableAudioModifier(channel, contextId);
	}


	struct AudioInstanceWrapper
	{
		uint32 mUniqueId = 0;
		static inline const lemon::CustomDataType* mDataType = nullptr;
	};

	AudioInstanceWrapper Audio_getAudioInstanceByAudioKey_u64(uint64 audioKey)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		const AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByAudioKey(audioKey);
		return AudioInstanceWrapper { ref.mUniqueId };
	}

	AudioInstanceWrapper Audio_getAudioInstanceByAudioKey(lemon::StringRef audioKey)
	{
		// Support keys like "2C", which should result in a "hash" of that uint8 hex number (here 0x2c)
		const int64 numericKey = AudioCollection::checkForNumericKey(audioKey.getString());
		if (numericKey >= 0)
			return Audio_getAudioInstanceByAudioKey_u64(numericKey);
		else
			return Audio_getAudioInstanceByAudioKey_u64(audioKey.getHash());
	}

	AudioInstanceWrapper Audio_getAudioInstanceByChannel(uint8 channelId)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		const AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByChannel(channelId);
		return AudioInstanceWrapper { ref.mUniqueId };
	}

	AudioInstanceWrapper Audio_getAudioInstanceByInternalId(uint32 internalId)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		const AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(internalId);
		return AudioInstanceWrapper { ref.mUniqueId };
	}

	AudioInstanceWrapper Audio_getAudioInstanceByIndex(uint16 index)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		const AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByIndex((size_t)index);
		return AudioInstanceWrapper { ref.mUniqueId };
	}

	uint16 Audio_getNumAudioInstances()
	{
		return (uint16)AudioPlayer::instance().getNumPlayingSounds();
	}

	bool AudioInstance_isValid(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.isValidPlayingSound(ref);
	}

	uint32 AudioInstance_getInternalId(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return ref.mUniqueId;
	}

	lemon::StringRef AudioInstance_getAudioKey(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return lemon::StringRef(audioPlayer.getPlayingSoundAudioKey(ref));
	}

	void AudioInstance_pause(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.pausePlayingSound(ref);
	}

	void AudioInstance_resume(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.resumePlayingSound(ref);
	}

	void AudioInstance_stop(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.stopPlayingSound(ref);
	}

	void AudioInstance_stop2(AudioInstanceWrapper audioInstance, float cutOffTime)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.stopPlayingSound(ref, clamp(cutOffTime, 0.001f, 10.0f));
	}

	float AudioInstance_getVolume(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundVolume(ref);
	}

	void AudioInstance_setVolume(AudioInstanceWrapper audioInstance, float volume)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.setPlayingSoundVolume(ref, volume);
	}

	void AudioInstance_fadeToVolume(AudioInstanceWrapper audioInstance, float volume, float seconds)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.fadePlayingSoundVolume(ref, volume, seconds);
	}

	float AudioInstance_getPlaybackPosition(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundPosition(ref);
	}

	uint8 AudioInstance_getChannel(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundChannel(ref);
	}

	uint8 AudioInstance_getContext(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundContext(ref);
	}

	float AudioInstance_getPlaybackSpeed(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundSpeed(ref);
	}

	void AudioInstance_setPlaybackSpeed(AudioInstanceWrapper audioInstance, float speed)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.setPlayingSoundSpeed(ref, speed);
	}

	float AudioInstance_getPanning(AudioInstanceWrapper audioInstance)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		return audioPlayer.getPlayingSoundPanning(ref);
	}

	void AudioInstance_setPanning(AudioInstanceWrapper audioInstance, float panning)
	{
		AudioPlayer& audioPlayer = AudioPlayer::instance();
		AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByUniqueId(audioInstance.mUniqueId);
		audioPlayer.setPlayingSoundPanning(ref, clamp(panning, -1.0f, 1.0f));
	}
}


namespace lemon
{
	namespace traits
	{
		template<> const DataTypeDefinition* getDataType<AudioInstanceWrapper>()  { return AudioInstanceWrapper::mDataType; }
	}

	namespace internal
	{
		template<>
		void pushStackGeneric<AudioInstanceWrapper>(AudioInstanceWrapper value, const NativeFunction::Context context)
		{
			context.mControlFlow.pushValueStack(value.mUniqueId);
		};

		template<>
		AudioInstanceWrapper popStackGeneric(const NativeFunction::Context context)
		{
			return AudioInstanceWrapper { context.mControlFlow.popValueStack<uint32>() };
		}
	}
}


void AudioBindings::registerBindings(lemon::Module& module)
{
	lemon::ModuleBindingsBuilder builder(module);

	// Data type
	AudioInstanceWrapper::mDataType = module.addCustomDataType("AudioInstance", lemon::BaseType::UINT_32);

	// Functions
	{
		const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION);
		const BitFlagSet<lemon::Function::Flag> compileTimeConstant(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::COMPILE_TIME_CONSTANT);
		const BitFlagSet<lemon::Function::Flag> deprecatedFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::DEPRECATED);


		// Global audio functions
		builder.addNativeFunction("Audio.getAudioKeyType", lemon::wrap(&Audio_getAudioKeyType), defaultFlags)
			.setParameters("audioKey");

		builder.addNativeFunction("Audio.isPlayingAudio", lemon::wrap(&Audio_isPlayingAudio), defaultFlags)
			.setParameters("audioKey");

		builder.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio1), defaultFlags)
			.setParameters("audioKey", "contextId");

		builder.addNativeFunction("Audio.playAudio", lemon::wrap(&Audio_playAudio2), defaultFlags)
			.setParameters("audioKey");

		builder.addNativeFunction("Audio.pauseChannel", lemon::wrap(&Audio_pauseChannel), defaultFlags)
			.setParameters("channel");

		builder.addNativeFunction("Audio.resumeChannel", lemon::wrap(&Audio_resumeChannel), defaultFlags)
			.setParameters("channel");

		builder.addNativeFunction("Audio.stopChannel", lemon::wrap(&Audio_stopChannel), defaultFlags)
			.setParameters("channel");

		builder.addNativeFunction("Audio.pauseContext", lemon::wrap(&Audio_pauseContext), defaultFlags)
			.setParameters("contextId");

		builder.addNativeFunction("Audio.resumeContext", lemon::wrap(&Audio_resumeContext), defaultFlags)
			.setParameters("contextId");

		builder.addNativeFunction("Audio.stopContext", lemon::wrap(&Audio_stopContext), defaultFlags)
			.setParameters("contextId");

		builder.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel), defaultFlags)
			.setParameters("channel", "seconds");

		builder.addNativeFunction("Audio.fadeInChannel", lemon::wrap(&Audio_fadeInChannel2), defaultFlags)
			.setParameters("channel", "length");

		builder.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel), defaultFlags)
			.setParameters("channel", "seconds");

		builder.addNativeFunction("Audio.fadeOutChannel", lemon::wrap(&Audio_fadeOutChannel2), defaultFlags)
			.setParameters("channel", "length");

		builder.addNativeFunction("Audio.playOverride", lemon::wrap(&Audio_playOverride), defaultFlags)
			.setParameters("audioKey", "contextId", "channelId", "overriddenChannelId");

		builder.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier), defaultFlags)
			.setParameters("channel", "contextId", "postfix", "relativeSpeed");

		builder.addNativeFunction("Audio.enableAudioModifier", lemon::wrap(&Audio_enableAudioModifier2), defaultFlags)
			.setParameters("channel", "contextId", "postfix", "relativeSpeed");

		builder.addNativeFunction("Audio.disableAudioModifier", lemon::wrap(&Audio_disableAudioModifier), defaultFlags)
			.setParameters("channel", "context");


		// Audio instance related functions
		builder.addNativeFunction("Audio.getAudioInstanceByAudioKey", lemon::wrap(&Audio_getAudioInstanceByAudioKey_u64), defaultFlags)
			.setParameters("audioKey");

		builder.addNativeFunction("Audio.getAudioInstanceByAudioKey", lemon::wrap(&Audio_getAudioInstanceByAudioKey), defaultFlags)
			.setParameters("audioKey");

		builder.addNativeFunction("Audio.getAudioInstanceByChannel", lemon::wrap(&Audio_getAudioInstanceByChannel), defaultFlags)
			.setParameters("channelId");

		builder.addNativeFunction("Audio.getAudioInstanceByInternalId", lemon::wrap(&Audio_getAudioInstanceByInternalId), defaultFlags)
			.setParameters("internalId");

		builder.addNativeFunction("Audio.getAudioInstanceByIndex", lemon::wrap(&Audio_getAudioInstanceByIndex), defaultFlags)
			.setParameters("index");

		builder.addNativeFunction("Audio.getNumAudioInstances", lemon::wrap(&Audio_getNumAudioInstances), defaultFlags);


		// Audio instance methods
		builder.addNativeMethod("AudioInstance", "isValid", lemon::wrap(&AudioInstance_isValid), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "getInternalId", lemon::wrap(&AudioInstance_getInternalId), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "getAudioKey", lemon::wrap(&AudioInstance_getAudioKey), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "pause", lemon::wrap(&AudioInstance_pause), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "resume", lemon::wrap(&AudioInstance_resume), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "stop", lemon::wrap(&AudioInstance_stop), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "stop", lemon::wrap(&AudioInstance_stop2), defaultFlags)
			.setParameters("this", "cutOffTime");

		builder.addNativeMethod("AudioInstance", "getVolume", lemon::wrap(&AudioInstance_getVolume), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "setVolume", lemon::wrap(&AudioInstance_setVolume), defaultFlags)
			.setParameters("this", "volume");

		builder.addNativeMethod("AudioInstance", "fadeToVolume", lemon::wrap(&AudioInstance_fadeToVolume), defaultFlags)
			.setParameters("this", "volume", "seconds");

		builder.addNativeMethod("AudioInstance", "getPlaybackPosition", lemon::wrap(&AudioInstance_getPlaybackPosition), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "getChannel", lemon::wrap(&AudioInstance_getChannel), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "getContext", lemon::wrap(&AudioInstance_getContext), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "getPlaybackSpeed", lemon::wrap(&AudioInstance_getPlaybackSpeed), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "setPlaybackSpeed", lemon::wrap(&AudioInstance_setPlaybackSpeed), defaultFlags)
			.setParameters("this", "speed");

		builder.addNativeMethod("AudioInstance", "getPanning", lemon::wrap(&AudioInstance_getPanning), defaultFlags)
			.setParameters("this");

		builder.addNativeMethod("AudioInstance", "setPanning", lemon::wrap(&AudioInstance_setPanning), defaultFlags)
			.setParameters("this", "panning");
	}
}
