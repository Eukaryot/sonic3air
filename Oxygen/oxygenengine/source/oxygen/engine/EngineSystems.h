/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/engine/modding/ModManager.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/application/input/ControlsIn.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/download/DownloadManager.h"
#include "oxygen/drawing/upscaler/UpscalerCollection.h"
#include "oxygen/network/EngineServerClient.h"
#include "oxygen/network/crowdcontrol/CrowdControlClient.h"
#include "oxygen/platform/CommandForwarder.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/resources/ResourcesCache.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/PersistentData.h"
#if defined(PLATFORM_ANDROID)
	#include "oxygen/platform/android/AndroidJavaInterface.h"
#endif


namespace oxygen
{
	class EngineSystems
	{
	public:
		// Main systems
		GameProfile		   mGameProfile;
		InputManager	   mInputManager;
		ControlsIn		   mControlsIn;
		VideoOut		   mVideoOut;
		ModManager		   mModManager;

		// Resources
		ResourcesCache	   mResourcesCache;
		FontCollection	   mFontCollection;
		PersistentData	   mPersistentData;
		UpscalerCollection mUpscalerCollection;

		// Platform-specific
	#if defined(PLATFORM_ANDROID)
		AndroidJavaInterface mAndroidJavaInterface;
	#endif

		// Helper systems
		LogDisplay		   mLogDisplay;
		CommandForwarder   mCommandForwarder;

		// Extensions
		DownloadManager	   mDownloadManager;
		EngineServerClient mEngineServerClient;
		CrowdControlClient mCrowdControlClient;
	};
}
