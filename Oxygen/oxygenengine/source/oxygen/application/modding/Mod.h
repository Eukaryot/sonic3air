/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Mod
{
friend class ModManager;	// For access to mWillGetActive

public:
	enum class State
	{
		INACTIVE,
		ACTIVE,
		FAILED
	};

	struct Setting
	{
		struct Option
		{
			std::string mDisplayName;
			uint32 mValue = 0;
		};

		std::string mIdentifier;
		std::string mDisplayName;
		std::string mBinding;
		std::vector<Option> mOptions;
		uint32 mDefaultValue = 0;
		uint32 mCurrentValue = 0;
	};

	struct SettingCategory
	{
		std::string mDisplayName;
		uint64 mNameHash = 0;
		std::vector<Setting> mSettings;
	};

	struct OtherMod
	{
		std::string mModName;
		std::string mMinimumVersion;
		bool mIsRequired = false;
		int mRelativePriority = 0;
	};

public:
	std::string mUniqueID;				// Unique mod ID
	std::string mDirectoryName;			// Directory name (an alternative internal name for the sake of compatibility)
	std::wstring mLocalDirectory;		// Local path inside mods directory, excluding the trailing slash, e.g. "my-sample-mod" or "modfolder/my-sample-mod"
	std::wstring mFullPath;				// Complete path, now including the trailing slash, e.g. "<savedatadir>/mods/modfolder/my-sample-mod/"
	uint64 mLocalDirectoryHash = 0;
	State mState = State::INACTIVE;
	std::string mFailedMessage;
	uint32 mActivePriority = 0;			// Priority in mod loading, starting at 0 for lowest priority; this is also the index in mActiveMods, and is not valid for inactive mods

	// Meta data
	std::string mDisplayName;
	std::string mModVersion;
	std::string mAuthor;
	std::string mDescription;
	std::string mURL;

	// Settings
	std::vector<SettingCategory> mSettingCategories;

	// Relationships with other mods
	std::vector<OtherMod> mOtherMods;

public:
	void loadFromJson(const Json::Value& json);

private:
	bool mDirty = false;			// Only temporarily used by ModManager
	bool mWillGetActive = false;	// Only temporarily used by ModManager
};
