/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/modding/Mod.h"
#include "oxygen/helper/JsonHelper.h"


void Mod::loadFromJson(const Json::Value& json)
{
	Json::Value metadataJson = json["Metadata"];
	if (metadataJson.isObject())
	{
		JsonHelper jsonHelper(metadataJson);
		jsonHelper.tryReadString("Name", mDisplayName);			// Old property, for backward compatibility with old game versions (still recommended)
		jsonHelper.tryReadString("DisplayName", mDisplayName);	// New property, for forward compatibility with future game versions
		jsonHelper.tryReadString("UniqueID", mUniqueID);
		jsonHelper.tryReadString("ModVersion", mModVersion);
		jsonHelper.tryReadString("Author", mAuthor);
		jsonHelper.tryReadString("Description", mDescription);
		jsonHelper.tryReadString("URL", mURL);
	}

	// Fallback for names
	if (mDisplayName.empty())
		mDisplayName = mDirectoryName;
	if (mUniqueID.empty())
		mUniqueID = mDirectoryName;

	// Read settings
	Json::Value settingsJson = json["Settings"];
	if (settingsJson.isObject())
	{
		for (auto iteratorCategories = settingsJson.begin(); iteratorCategories != settingsJson.end(); ++iteratorCategories)
		{
			const Json::Value categoryJson = *iteratorCategories;
			if (!categoryJson.isArray())
				continue;

			for (Json::ArrayIndex k = 0; k < categoryJson.size(); ++k)
			{
				Json::Value content = categoryJson[k];
				std::string categoryName;
				std::string internalName;
				std::string displayName;
				std::string variableName;
				std::string defaultValue;

				JsonHelper jsonHelper(content);
				jsonHelper.tryReadString("Category", categoryName);
				jsonHelper.tryReadString("InternalName", internalName);
				jsonHelper.tryReadString("DisplayName", displayName);
				jsonHelper.tryReadString("Variable", variableName);
				jsonHelper.tryReadString("DefaultValue", defaultValue);
				if (internalName.empty() || displayName.empty() || variableName.empty())	// The rest is optional
					continue;

				Json::Value optionsJson = content["Options"];
				if (!optionsJson.isObject())
					continue;

				SettingCategory* settingCategory = nullptr;
				{
					const uint64 categoryNameHash = categoryName.empty() ? 0 : rmx::getMurmur2_64(categoryName);
					for (SettingCategory& settingCategory_ : mSettingCategories)
					{
						if (settingCategory_.mNameHash == categoryNameHash)
						{
							settingCategory = &settingCategory_;
						}
					}

					if (nullptr == settingCategory)
					{
						settingCategory = &vectorAdd(mSettingCategories);
						settingCategory->mDisplayName = categoryName;
						settingCategory->mNameHash = categoryNameHash;
					}
				}

				Setting& setting = vectorAdd(settingCategory->mSettings);
				setting.mIdentifier = internalName;
				setting.mDisplayName = displayName;
				setting.mBinding = variableName;
				if (!defaultValue.empty())
				{
					setting.mDefaultValue = (uint32)rmx::parseInteger(defaultValue);
					setting.mCurrentValue = setting.mDefaultValue;
				}

				for (auto it2 = optionsJson.begin(); it2 != optionsJson.end(); ++it2)
				{
					if (!it2->isString())
						continue;

					const std::string valueString = it2.key().asString();

					Setting::Option& option = vectorAdd(setting.mOptions);
					option.mDisplayName = it2->asString();
					option.mValue = (uint32)rmx::parseInteger(valueString);
				}

				// Sanity check: We need at least one option; though exactly one does not make that much sense
				if (setting.mOptions.empty())
				{
					settingCategory->mSettings.pop_back();
				}
			}
		}
	}

	// Read used features
	Json::Value featuresModsJson = json["UsesFeatures"];
	if (featuresModsJson.isObject())
	{
		for (auto iteratorFeatures = featuresModsJson.begin(); iteratorFeatures != featuresModsJson.end(); ++iteratorFeatures)
		{
			const Json::Value featureJson = *iteratorFeatures;
			if (featureJson.isBool())
			{
				const std::string featureName = iteratorFeatures.key().asString();
				const uint64 key = rmx::getMurmur2_64(featureName);
				if (featureJson.asBool())
				{
					mUsedFeatures[key].mFeatureName = featureName;
				}
				else
				{
					mUsedFeatures.erase(key);	// In case it was previously added
				}
			}
		}
	}

	// Read relationships with other mods
	Json::Value otherModsJson = json["OtherMods"];
	if (otherModsJson.isObject())
	{
		for (auto iteratorOtherMods = otherModsJson.begin(); iteratorOtherMods != otherModsJson.end(); ++iteratorOtherMods)
		{
			const Json::Value modJson = *iteratorOtherMods;
			if (!modJson.isObject())
				continue;

			OtherModInfo& otherModInfo = vectorAdd(mOtherModInfos);
			otherModInfo.mModID = iteratorOtherMods.key().asString();
			otherModInfo.mModIDHash = rmx::getMurmur2_64(otherModInfo.mModID);

			JsonHelper jsonHelper(modJson);
			if (!jsonHelper.tryReadString("DisplayName", otherModInfo.mDisplayName))
				otherModInfo.mDisplayName = otherModInfo.mModID;
			jsonHelper.tryReadString("MinimumVersion", otherModInfo.mMinimumVersion);
			jsonHelper.tryReadBool("IsRequired", otherModInfo.mIsRequired);

			const Json::Value& priorityValue = modJson["Priority"];
			if (priorityValue.isString())
			{
				String str = priorityValue.asString();
				str.lowerCase();
				if (str == "higher")
					otherModInfo.mRelativePriority = +1;
				else if (str == "lower")
					otherModInfo.mRelativePriority = -1;
			}
		}
	}
}

const Mod::UsedFeature* Mod::getUsedFeature(std::string_view featureName) const
{
	return mapFind(mUsedFeatures, rmx::getMurmur2_64(featureName));
}

const Mod::UsedFeature* Mod::getUsedFeature(uint64 featureNameHash) const
{
	return mapFind(mUsedFeatures, featureNameHash);
}
