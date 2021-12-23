/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/EngineDelegate.h"
#include "sonic3air/version.inc"

#include "oxygen/base/PlatformFunctions.h"
#include "oxygen/file/FilePackage.h"


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


struct Arguments
{
	bool mPack = false;
	bool mNativize = false;
	bool mDumpCppDefinitions = false;
};

void readArguments(Arguments& outArguments, int argc, char** argv)
{
#if !defined(ENDUSER) && !defined(PLATFORM_ANDROID)
	if (argc == 2)
	{
		const std::string parameter(argv[1]);
		if (parameter == "-pack")
		{
			outArguments.mPack = true;
		}
		else if (parameter == "-nativize")
		{
			outArguments.mNativize = true;
		}
		else if (parameter == "-dumpcppdefinitions")
		{
			outArguments.mDumpCppDefinitions = true;
		}
	}
#endif
}

void performPacking()
{
	// Update metadata.json
	String metadata;
	metadata << "{\r\n"
			 << "\t\"Game\" : \"Sonic 3 - Angel Island Revisited\",\r\n"
			 << "\t\"Author\" : \"Eukaryot (original game by SEGA)\",\r\n"
			 << "\t\"Version\" : \"" << BUILD_STRING << "\",\r\n"
			 << "\t\"GameAppBuild\" : \"" << rmx::hexString(BUILD_NUMBER, 8) << "\"\r\n"
			 << "}\r\n";
	metadata.saveFile("data/metadata.json");

	// "gamedata.bin" = data directory except audio and shaders
	{
		std::vector<std::wstring> includedPaths = { L"data/" };
		std::vector<std::wstring> excludedPaths = { L"data/audio/", L"data/shader/", L"data/metadata.json" };
		FilePackage::createFilePackage(L"gamedata.bin", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "audiodata.bin" = emulated / original audio directory
	{
		std::vector<std::wstring> includedPaths = { L"data/audio/original/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"audiodata.bin", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "audioremaster.bin" = remastered audio directory
	{
		std::vector<std::wstring> includedPaths = { L"data/audio/remastered/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"audioremaster.bin", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "enginedata.bin" = shaders directory
	{
		std::vector<std::wstring> includedPaths = { L"data/shader/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"enginedata.bin", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}
}


int main(int argc, char** argv)
{
	EngineMain::earlySetup();

	// Make sure we're in the correct working directory
	PlatformFunctions::changeWorkingDirectory(argc == 0 ? "" : argv[0]);

	// Read command line arguments
	Arguments arguments;
	readArguments(arguments, argc, argv);

	if (arguments.mPack)
	{
		performPacking();
		return 0;
	}

	try
	{
		// Create engine delegate and engine main instance
		EngineDelegate myDelegate;
		EngineMain myMain(myDelegate);

		if (arguments.mNativize)
		{
			Configuration::instance().mRunScriptNativization = 1;
			Configuration::instance().mScriptNativizationOutput = L"source/sonic3air/_nativized/NativizedCode.inc";
		}
		if (arguments.mDumpCppDefinitions)
		{
			Configuration::instance().mDumpCppDefinitionsOutput = L"source/sonic3air/_nativized/NativizedCode.inc";
		}

		myMain.execute(argc, argv);
	}
	catch (const std::exception& e)
	{
		RMX_ERROR("Caught unhandled exception in main loop: " << e.what(), );
	}

	return 0;
}
