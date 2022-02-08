/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/modding/Mod.h"
#include <functional>

class ZipFileProvider;


class ModManager : public SingleInstance<ModManager>
{
public:
	~ModManager();

	inline const std::vector<Mod*>& getAllMods() const	   { return mAllMods; }
	inline const std::vector<Mod*>& getActiveMods() const  { return mActiveMods; }
	inline const std::unordered_map<uint64, Mod*>& getActiveModsByNameHash() const  { return mActiveModsByNameHash; }

	void startup();
	void clear();
	bool rescanMods();
	void saveActiveMods();

	void setActiveMods(const std::vector<Mod*>& newActiveModsList);

	void copyModSettingsFromConfig();
	void copyModSettingsToConfig();

private:
	struct FoundMod
	{
		std::wstring mLocalPath;
		std::wstring mModName;
		Json::Value mModJson;
	};

private:
	bool scanMods();
	void scanDirectoryRecursive(std::vector<FoundMod>& outFoundMods, const std::wstring& localPath);
	void findZipsRecursively(std::vector<std::wstring>& outZipPaths, const std::wstring& localPath, int maxDepth);
	bool processModZipFile(const std::wstring& zipLocalPath);
	void onActiveModsChanged(bool duringStartup = false);

private:
	std::wstring mBasePath;
	std::vector<Mod*> mAllMods;
	std::vector<Mod*> mActiveMods;
	std::unordered_map<uint64, Mod*> mActiveModsByNameHash;		// Each mod is registered by both its internal name and display name
	std::unordered_map<uint64, Mod*> mModsByLocalDirectoryHash;
	std::map<std::wstring, ZipFileProvider*> mZipFileProviders;
};
