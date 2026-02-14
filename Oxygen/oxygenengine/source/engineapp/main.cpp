/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#define RMX_LIB
#include "engineapp/pch.h"
#include "engineapp/EngineDelegate.h"

#include "oxygen/application/ArgumentsReader.h"
#include "oxygen/platform/PlatformFunctions.h"


#if defined(PLATFORM_WINDOWS) && !defined(__GNUC__)
extern "C"
{
	_declspec(dllexport) uint32 NvOptimusEnablement = 1;
	_declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif


int main(int argc, char** argv)
{
	EngineMain::earlySetup();

	ArgumentsReader arguments;
	arguments.read(argc, argv);

	// Make sure we're in the correct working directory
	PlatformFunctions::changeWorkingDirectory(arguments.mExecutableCallPath);

	// Create engine delegate and angine main instance
	{
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate, arguments);

		myMain.execute();
	}

	return 0;
}
