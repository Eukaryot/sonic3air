/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/GameLoader.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/application/audio/AudioOutBase.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/rendering/RenderResources.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/simulation/PersistentData.h"
#if defined (PLATFORM_ANDROID)
	#include "oxygen/platform/AndroidJavaInterface.h"
#endif

GameLoader::UpdateResult GameLoader::updateLoading()
{
	switch (mState)
	{
		case State::UNLOADED:
		{
			RMX_LOG_INFO("Loading ROM...");
			if (!ResourcesCache::instance().loadRom())
			{
			#if defined(PLATFORM_ANDROID)
				AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
				if (javaInterface.hasRomFileAlready())
				{
					const bool success = ResourcesCache::instance().loadRomFromMemory(javaInterface.getRomFileInjection().mRomContent);
					if (success)
					{
						mState = State::ROM_LOADED;
						return UpdateResult::CONTINUE_IMMEDIATE;
					}
				}
			#endif

				RMX_LOG_INFO("ROM loading failed");

				const GameProfile& gameProfile = GameProfile::instance();
				if (!gameProfile.mRomInfos.empty())
				{
				#if defined(PLATFORM_WINDOWS)
					const std::string text = "This game requires an original " + gameProfile.mRomInfos[0].mSteamGameName + " ROM to work.\nIf you have one, click OK and select it in the following dialog.\n\nSee the Manual for details.";
					const PlatformFunctions::DialogResult result = PlatformFunctions::showDialogBox(rmx::ErrorSeverity::INFO, PlatformFunctions::DialogButtons::OK_CANCEL, gameProfile.mFullName, text);
					if (result == PlatformFunctions::DialogResult::CANCEL)
					{
						return UpdateResult::FAILURE;
					}

					const std::wstring romPath = PlatformFunctions::openFileSelectionDialog(L"Select game ROM", L"", L"Genesis ROM files\0*.bin;*.68K\0All Files\0*.*\0\0");
					const bool success = ResourcesCache::instance().loadRomFromFile(romPath);
					if (success)
					{
						mState = State::ROM_LOADED;
						return UpdateResult::CONTINUE_IMMEDIATE;
					}
					else
					{
						// How about another try?
						mState = State::UNLOADED;
						return UpdateResult::CONTINUE_IMMEDIATE;
					}

				#elif defined(PLATFORM_ANDROID)
					javaInterface.openRomFileSelectionDialog();
					mState = State::WAITING_FOR_ROM;
					return UpdateResult::CONTINUE;

				#else
					RMX_ERROR("ROM could not be loaded!\nAn original " + gameProfile.mRomInfos[0].mSteamGameName + " ROM must be added manually. See the Manual for details.\n\nThe application will now close.", );
					return UpdateResult::FAILURE;

				#endif
				}
				else
				{
					RMX_ERROR("ROM could not be loaded!\nAn original game ROM must be added manually. See the Manual for details.\n\nThe application will now close.", );
				}
			}
			RMX_LOG_INFO("ROM found at: " << WString(Configuration::instance().mLastRomPath).toStdString());

			mState = State::ROM_LOADED;
			return UpdateResult::CONTINUE_IMMEDIATE;
		}

		case State::WAITING_FOR_ROM:
		{
		#if defined(PLATFORM_ANDROID)
			AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
			const AndroidJavaInterface::BinaryDialogResult result = javaInterface.getRomFileInjection().mDialogResult;
			switch (result)
			{
				case AndroidJavaInterface::BinaryDialogResult::SUCCESS:
				{
					const bool success = ResourcesCache::instance().loadRomFromMemory(javaInterface.getRomFileInjection().mRomContent);
					if (success)
					{
						mState = State::ROM_LOADED;
						return UpdateResult::CONTINUE_IMMEDIATE;
					}
					// Fallthrough by design
				}

				case AndroidJavaInterface::BinaryDialogResult::FAILED:
				{
					// How about another try?
					mState = State::UNLOADED;
					return UpdateResult::CONTINUE_IMMEDIATE;
				}

				default:
					break;
			}
		#endif
			return UpdateResult::CONTINUE;
		}

		case State::ROM_LOADED:
		{
			// Initialize mods
			RMX_LOG_INFO("Mod manager initialization...");
			ModManager::instance().startup();

			// Load sprites
			RMX_LOG_INFO("Loading sprites");
			VideoOut::instance().getRenderResources().loadSpriteCache();

			// Load resources
			RMX_LOG_INFO("Resource cache loading...");
			ResourcesCache::instance().loadAllResources();

			// Load persistent data
			RMX_LOG_INFO("Persistent data loading...");
			PersistentData::instance().loadFromFile(Configuration::instance().mPersistentDataFilename);

			// Load audio definitions
			EngineMain::instance().getAudioOut().handleGameLoaded();

			// Game loaded
			mState = State::READY;
			return UpdateResult::SUCCESS;
		}

		case State::READY:
		{
			// Nothing to do
			return UpdateResult::SUCCESS;
		}

		case State::FAILED:
		{
			return UpdateResult::FAILURE;
		}
	}

	// Unhandled state
	return UpdateResult::FAILURE;
}
