/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/helper/ArgumentsReader.h"
#include "sonic3air/helper/PackageBuilder.h"

#include "oxygen/platform/PlatformFunctions.h"

#if defined(PLATFORM_VITA)
	#include <vitasdk.h>
	#include <vitaGL.h>
	#include "platform/vita/trophies.h"

	int init_msg_dialog(const char* msg)
	{
		SceMsgDialogUserMessageParam msg_param;
		memset(&msg_param, 0, sizeof(msg_param));
		msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
		msg_param.msg = (SceChar8 *)msg;

		SceMsgDialogParam param;
		sceMsgDialogParamInit(&param);
		_sceCommonDialogSetMagicNumber(&param.commonParam);
		param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
		param.userMsgParam = &msg_param;

		return sceMsgDialogInit(&param);
	}

	void warning(const char* msg)
	{
		init_msg_dialog(msg);

		while (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED) {
			vglSwapBuffers(GL_TRUE);
		}
		sceMsgDialogTerm();
	}

	void fatal_error(const char* msg)
	{
		vglInit(0);
		warning(msg);
		sceKernelExitProcess(0);
		while (1);
	}

	int file_exists(const char* path)
	{
		SceIoStat stat;
		return sceIoGetstat(path, &stat) >= 0;
	}
#endif

// HJW: I know it's sloppy to put this here... it'll get moved afterwards
// Building with my env (msys2,gcc) requires this stub for some reason
#ifndef pathconf
long pathconf(const char* path, int name)
{
	errno = ENOSYS;
	return -1;
}
#endif

#if defined(PLATFORM_WINDOWS) & !defined(__GNUC__)
extern "C"
{
	_declspec(dllexport) uint32 NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

#if defined(PLATFORM_VITA)
extern "C"
{
	// Any value highter than 324 MB will make the game either boot without sound or just crash the PSVITA due to lack of physical RAM
	int _newlib_heap_size_user = 324 * 1024 * 1024;
	unsigned int sceUserMainThreadStackSize = 4 * 1024 * 1024;	
}
#endif

int main(int argc, char** argv)
{
	EngineMain::earlySetup();

#if !defined(PLATFORM_VITA)
	// Read command line arguments
	ArgumentsReader arguments;
	arguments.read(argc, argv);

	// For certain arguments, just try to forward them to an already running instance of S3AIR
	if (!arguments.mUrl.empty())
	{
		if (CommandForwarder::trySendCommand("ForwardedCommand:Url:" + arguments.mUrl))
			return 0;
	}

	// Make sure we're in the correct working directory
	PlatformFunctions::changeWorkingDirectory(arguments.mExecutableCallPath);
#else
	scePowerSetArmClockFrequency(444);
	scePowerSetBusClockFrequency(222);
	scePowerSetGpuClockFrequency(222);
	scePowerSetGpuXbarClockFrequency(166);

	// Check for libshacccg.suprx existence
	if (!file_exists("ur0:/data/libshacccg.suprx") && !file_exists("ur0:/data/external/libshacccg.suprx"))
		fatal_error("Error: libshacccg.suprx is not installed.");

	vglInitExtended(0, 960, 544, 12 * 1024 * 1024, SCE_GXM_MULTISAMPLE_NONE);

	// Initing trophy system
	SceIoStat st;
	int r = trophies_init();
	if (r < 0 && sceIoGetstat("ux0:data/sonic3air/trophies.chk", &st) < 0) {
		FILE *f = fopen("ux0:data/sonic3air/trophies.chk", "w");
		fclose(f);
		warning("This game features unlockable trophies but NoTrpDrm is not installed. If you want to be able to unlock trophies, please install it.");
	}

	argc = 0;

	PlatformFunctions::changeWorkingDirectory(L"ux0:/data/sonic3air");
	ArgumentsReader arguments;
#endif

#if defined(PLATFORM_WINDOWS)
	// Check if the user has an old version of "audioremaster.bin", and remove it if that the case
	//  -> As the newer installations don't include that file, it is most likely an out-dated one, and could cause problems (at least an assert) down the line
	if (FTX::FileSystem->exists(L"data/audioremaster.bin"))
		FTX::FileSystem->removeFile(L"data/audioremaster.bin");
#endif

#if !defined(PLATFORM_ANDROID) && !defined(PLATFORM_VITA)
	if (arguments.mPack)
	{
		PackageBuilder::performPacking();
		if (!arguments.mNativize && !arguments.mDumpCppDefinitions)		// In case multiple arguments got combined, the others would get ignored without this check
			return 0;
	}
#endif

	try
	{
		// Create engine delegate and engine main instance
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate);

		// Evaluate some more arguments
		Configuration& config = Configuration::instance();
		if (arguments.mNativize)
		{
			config.mRunScriptNativization = 1;
			config.mScriptNativizationOutput = L"source/sonic3air/_nativized/NativizedCode.inc";
			config.mExitAfterScriptLoading = true;
		}
		if (arguments.mDumpCppDefinitions)
		{
			config.mDumpCppDefinitionsOutput = L"scripts/_reference/cpp_core_functions.lemon";
			config.mExitAfterScriptLoading = true;
		}

		// Now run the game
		myMain.execute(argc, argv);
	}
	catch (const std::exception& e)
	{
		RMX_ERROR("Caught unhandled exception in main loop: " << e.what(), );
	}

	return 0;
}
