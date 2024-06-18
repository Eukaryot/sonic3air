/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/platform/PlatformSpecifics.h"

#if defined(PLATFORM_VITA)
	#include <vitasdk.h>
	#include <vitaGL.h>
	#include "sonic3air/platform/vita/trophies.h"
#endif


namespace
{
#if defined(PLATFORM_VITA)
	int init_msg_dialog(const char* msg)
	{
		SceMsgDialogUserMessageParam msg_param;
		memset(&msg_param, 0, sizeof(msg_param));
		msg_param.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;
		msg_param.msg = (SceChar8*)msg;

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

		while (sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
		{
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
}


void PlatformSpecifics::platformStartup()
{
#if defined(PLATFORM_VITA)
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
	if (r < 0 && sceIoGetstat("ux0:data/sonic3air/trophies.chk", &st) < 0)
	{
		FILE* f = fopen("ux0:data/sonic3air/trophies.chk", "w");
		fclose(f);
		warning("This game features unlockable trophies but NoTrpDrm is not installed. If you want to be able to unlock trophies, please install it.");
	}

	changeWorkingDirectory(L"ux0:/data/sonic3air");
#endif
}
