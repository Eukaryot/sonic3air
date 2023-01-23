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
		jsonHelper.tryReadString("Name", mDisplayName);
		jsonHelper.tryReadString("ModVersion", mModVersion);
		jsonHelper.tryReadString("Author", mAuthor);
		jsonHelper.tryReadString("Description", mDescription);
		jsonHelper.tryReadString("URL", mURL);
	}

	// Fallback for name
	if (mDisplayName.empty())
		mDisplayName = mName;

	// Read settings
	Json::Value settingsJson = json["Settings"];
	if (settingsJson.isObject())
	{
		for (auto iteratorCategories = settingsJson.begin(); iteratorCategories != settingsJson.end(); ++iteratorCategories)
		{
			// TODO: Support categories
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
}
