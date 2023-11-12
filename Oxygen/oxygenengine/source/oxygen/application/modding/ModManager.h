/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
	inline const std::vector<Mod*>& getActiveMods() const  { return mActiveMods; }	// Sorted in inverse priority, i.e. highest prio mods are at the end of the list
	inline const std::unordered_map<uint64, Mod*>& getActiveModsByNameHash() const	{ return mActiveModsByNameHash; }
	inline const std::unordered_map<uint64, Mod*>& getModsByIDHash() const			{ return mModsByIDHash; }

	Mod* findModByIDHash(uint64 idHash) const  { Mod*const* ptr = mapFind(mModsByIDHash, idHash); return (nullptr != ptr) ? *ptr : nullptr; }

	void startup();
	void clear();
	bool rescanMods();
	void saveActiveMods();

	void setActiveMods(const std::vector<Mod*>& newActiveModsList);

	bool anyActiveModUsesFeature(uint64 featureNameHash) const;

	void copyModSettingsFromConfig();
	void copyModSettingsToConfig();

private:
	struct FoundMod
	{
		std::wstring mLocalPath;
		std::wstring mDirectoryName;
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
	std::unordered_map<uint64, Mod*> mModsByIDHash;
	std::map<std::wstring, ZipFileProvider*> mZipFileProviders;
};
