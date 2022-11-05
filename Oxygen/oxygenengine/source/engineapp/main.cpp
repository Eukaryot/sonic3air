#define RMX_LIB
#include "engineapp/pch.h"
#include "engineapp/EngineDelegate.h"

#include "oxygen/base/PlatformFunctions.h"


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

	// Make sure we're in the correct working directory
	if (argc > 0)
	{
		WString wstr;
		wstr.fromUTF8(std::string(argv[0]));
		PlatformFunctions::changeWorkingDirectory(wstr.toStdWString());
	}

	// Create engine delegate and angine main instance
	{
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate);

		myMain.execute(argc, argv);
	}

	return 0;
}
