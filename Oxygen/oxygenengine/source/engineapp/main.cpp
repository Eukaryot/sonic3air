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
#include "oxygen/platform/CommandForwarder.h"
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
	CommandForwarder::setApplicationName("OxygenEngine");

	ArgumentsReader arguments("oxygen://");

#if defined(PLATFORM_VITA)
	argc = 0;
#else
	// Read command line arguments
	arguments.read(argc, argv);

	// For certain arguments, just try to forward them to an already running instance of the Oxygen Engine application
	if (!arguments.mForwardedCommand.empty())
	{
		if (CommandForwarder::trySendCommand("ForwardedCommand:" + arguments.mForwardedCommand))
			return 0;
	}
	if (!arguments.mUrl.empty())
	{
		if (CommandForwarder::trySendCommand("ForwardedCommand:Url:" + arguments.mUrl))
			return 0;
	}
	if (arguments.mStop)
		return 0;

	// Make sure we're in the correct working directory
	PlatformFunctions::changeWorkingDirectory(arguments.mExecutableCallPath);
#endif

	// Randomization is quite important for server communication
	randomize();

	// Create engine delegate and angine main instance
	try
	{
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate, arguments);

		myMain.execute();
	}
	catch (const std::exception& e)
	{
		RMX_ERROR("Caught unhandled exception in main loop: " << e.what(), );
	}

	return 0;
}
