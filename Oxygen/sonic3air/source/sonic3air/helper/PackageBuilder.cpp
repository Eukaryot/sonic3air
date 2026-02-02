/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/helper/PackageBuilder.h"
#include "sonic3air/version.inc"

#include "oxygen/file/FilePackage.h"


void PackageBuilder::performPacking()
{
	// Check for engine data directory
	const std::wstring engineBasePath = L"../oxygenengine/";
	if (!FTX::FileSystem->exists(engineBasePath))
	{
		RMX_ERROR("Packaging failed! Could not find Oxygen Engine directory, expected at \"" << rmx::convertToUTF8(engineBasePath) << "\".", );
		return;
	}

	// Update metadata.json
	String metadata;
	metadata << "{\r\n"
		<< "\t\"Game\" : \"Sonic 3 - Angel Island Revisited\",\r\n"
		<< "\t\"Author\" : \"Eukaryot (original game by SEGA)\",\r\n"
		<< "\t\"Version\" : \"" << BUILD_STRING << "\",\r\n"
		<< "\t\"GameAppBuild\" : \"" << rmx::hexString(BUILD_NUMBER, 8) << "\"\r\n"
		<< "}\r\n";
	metadata.saveFile("data/metadata.json");

	// "gamedata.bin" = data directory except audio (and also excluding metadata.json)
	{
		std::vector<std::wstring> includedPaths = { L"data/" };
		std::vector<std::wstring> excludedPaths = { L"data/audio/", L"data/metadata.json" };
		FilePackage::createFilePackage(L"gamedata.bin", L"", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "audiodata.bin" = emulated / original audio directory
	{
		std::vector<std::wstring> includedPaths = { L"data/audio/original/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"audiodata.bin", L"", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "audioremaster.bin" = remastered audio directory
	{
		std::vector<std::wstring> includedPaths = { L"data/audio/remastered/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"audioremaster.bin", L"", includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}

	// "enginedata.bin" = engine's full data directory
	{
		std::vector<std::wstring> includedPaths = { L"data/" };
		std::vector<std::wstring> excludedPaths = { };
		FilePackage::createFilePackage(L"enginedata.bin", engineBasePath, includedPaths, excludedPaths, L"_master_image_template/data/", BUILD_NUMBER);
	}
}
