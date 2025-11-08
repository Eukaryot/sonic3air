/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/JsonHelper.h"


namespace
{
	bool parseAddressRange(GameProfile::AddressRange& outRange, const String& input)
	{
		const int pos = input.findChar('-', 0, 1);
		if (pos >= 0 && pos < input.length())
		{
			String part1 = input.getSubString(0, pos);
			String part2 = input.getSubString(pos + 1, -1);
			part1.trimWhitespace();
			part2.trimWhitespace();
			const uint32 value1 = (uint32)rmx::parseInteger(part1);
			const uint32 value2 = (uint32)rmx::parseInteger(part2);
			if (value1 <= value2)
			{
				outRange.first = value1;
				outRange.second = value2;
				return true;
			}
		}
		else
		{
			String part = input;
			part.trimWhitespace();
			const uint32 value = (uint32)rmx::parseInteger(part);
			outRange.first = value;
			outRange.second = value;
			return true;
		}
		return false;
	}
}


bool GameProfile::loadOxygenProjectFromFile(const std::wstring& filename)
{
	// Open file
	const Json::Value root = JsonHelper::loadFile(filename);
	if (root.isNull())
		return false;

	return loadOxygenProjectFromJson(root);
}

bool GameProfile::loadOxygenProjectFromJson(const Json::Value& jsonRoot)
{
	rmx::JsonHelper rootHelper(jsonRoot);
	Configuration& config = Configuration::instance();

	// Setup defaults
	mAsmStackRange.first  = 0xfffffd00;
	mAsmStackRange.second = 0xfffffe00;

	// Update configuration
	{
		std::string romFile;
		config.mMainScriptName = L"main.lemon";

		rootHelper.tryReadString("Rom", romFile);
		rootHelper.tryReadString("MainScriptName", config.mMainScriptName);
		rootHelper.tryReadBool("CompileScripts", config.mForceCompileScripts);

	#ifndef PLATFORM_MAC
		config.mRomPath = config.mProjectPath + String(romFile).toStdWString();
	#endif
		config.mSaveStatesDir = config.mProjectPath + L"saves/states/";
		config.mScriptsDir    = config.mProjectPath + L"scripts/";
		config.mGameDataPath  = config.mProjectPath + L"data";			// Can be overwritten down below
		config.mAnalysisDir   = config.mProjectPath + L"___internal/analysis/";
	}

	// Load metadata
	{
		rootHelper.tryReadString("ShortName", mShortName);
		rootHelper.tryReadString("FullName", mFullName);
	}

	// Load ROM check
	{
		std::string romcheck_;
		if (rootHelper.tryReadString("RomCheck", romcheck_))
		{
			String romcheck = romcheck_;
			std::vector<String> parts;
			romcheck.split(parts, ',');

			for (String& part : parts)
			{
				part.trimWhitespace();
				const int pos = part.findChar('=', 0, 1);
				if (pos >= 0 && pos < part.length())
				{
					const String key = part.getSubString(0, pos);
					const String value = part.getSubString(pos + 1, -1);

					if (key == "size")
					{
						mRomCheck.mSize = (uint32)rmx::parseInteger(value);
					}
					else if (key == "checksum")
					{
						mRomCheck.mChecksum = rmx::parseInteger(value);
					}
				}
			}
		}
	}

	// Load ROM infos
	{
		mRomInfos.clear();
		const Json::Value romCheckJson = jsonRoot["RomInfos"];
		if (romCheckJson.isObject())
		{
			for (auto it = romCheckJson.begin(); it != romCheckJson.end(); ++it)
			{
				JsonHelper jsonHelper(*it);
				RomInfo& romInfo = vectorAdd(mRomInfos);

				jsonHelper.tryReadString("SteamGameName", romInfo.mSteamGameName);
				jsonHelper.tryReadString("SteamRomName", romInfo.mSteamRomName);

				std::string overwritesString;
				if (jsonHelper.tryReadString("Overwrites", overwritesString))
				{
					String value = overwritesString;
					const int pos = value.findChar(':', 0, 1);
					if (pos >= 0 && pos < value.length())
					{
						const String address = value.getSubString(0, pos);
						const String byteValue = value.getSubString(pos + 1, -1);
						romInfo.mOverwrites.emplace_back((uint32)rmx::parseInteger(address), (uint8)rmx::parseInteger(byteValue));
					}
				}

				std::string blankRegionsString;
				if (jsonHelper.tryReadString("BlankRegions", blankRegionsString))
				{
					AddressRange range;
					if (parseAddressRange(range, blankRegionsString))
					{
						romInfo.mBlankRegions.emplace_back(range);
					}
					else
					{
						RMX_ERROR("Invalid range in BlankRegions", );
					}
				}

				std::string headerChecksumString;
				if (jsonHelper.tryReadString("HeaderChecksum", headerChecksumString))
				{
					romInfo.mHeaderChecksum = rmx::parseInteger(headerChecksumString);
				}

				jsonHelper.tryReadString("DiffFileName", romInfo.mDiffFileName);
			}
		}
	}

	// Load paths
	{
		rootHelper.tryReadString("GameDataPath", mGameDataPath);
		if (!mGameDataPath.empty())
		{
			config.mGameDataPath = config.mProjectPath + mGameDataPath;
		}
	}

	// Load data packages
	{
		mDataPackages.clear();
		const Json::Value dataPackagesJson = jsonRoot["DataPackages"];
		if (dataPackagesJson.isObject())
		{
			for (auto it = dataPackagesJson.begin(); it != dataPackagesJson.end(); ++it)
			{
				const std::string key = it.key().asString();
				JsonHelper jsonHelper(*it);

				DataPackage& dataPackage = vectorAdd(mDataPackages);
				dataPackage.mFilename = String(key).toStdWString();
				jsonHelper.tryReadBool("Required", dataPackage.mRequired);
			}
		}
	}

	// Load emulation section
	{
		const Json::Value emulationJson = jsonRoot["Emulation"];
		if (!emulationJson.isNull())
		{
			rmx::JsonHelper emulationJsonHelper(emulationJson);
			emulationJsonHelper.tryReadBool("PushPopAddressOnCall", mPushPopAddressOnCall);

			const Json::Value asmStackRangeJson = emulationJson["AsmStackRange"];
			if (asmStackRangeJson.isString())
			{
				AddressRange range;
				if (parseAddressRange(range, asmStackRangeJson.asCString()))
				{
					mAsmStackRange.first = range.first | 0xffff0000;
					mAsmStackRange.second = range.second | 0xffff0000;
				}
				else
				{
					RMX_ERROR("Invalid range in AsmStackRange: " << asmStackRangeJson.asCString(), );
				}
			}

			const Json::Value stackTranslationJson = emulationJson["StackTranslation"];
			if (stackTranslationJson.isArray())
			{
				for (Json::Value it : stackTranslationJson)
				{
					StackLookupEntry& stackLookup = vectorAdd(mStackLookups);
					const Json::Value asmStackJson = it["AsmStack"];
					const Json::Value lemonStackJson = it["LemonStack"];
					if (asmStackJson.isArray() && lemonStackJson.isArray())
					{
						for (Json::Value it2 : asmStackJson)
						{
							AddressRange range;
							if (parseAddressRange(range, it2.asCString()))
							{
								stackLookup.mAsmStack.emplace_back(range);
							}
							else
							{
								RMX_ERROR("Invalid range in AsmStack: " << it2.asCString(), );
							}
						}

						for (Json::Value it2 : lemonStackJson)
						{
							LemonStackEntry& entry = vectorAdd(stackLookup.mLemonStack);
							const String content(it2.asCString());
							const int pos = content.findChar('@', 0, 1);
							if (pos >= 0)
							{
								entry.mFunctionName = *content.getSubString(0, pos);
								if (entry.mFunctionName.back() == ' ')
									entry.mFunctionName.erase(pos-1);
								entry.mLabelName = *content.getSubString(pos, -1);
							}
							else
							{
								entry.mFunctionName = *content;
							}
						}
					}
				}
			}
		}
	}

	return true;
}
