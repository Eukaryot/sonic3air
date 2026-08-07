/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class ROMDataAnalyser : public SingleInstance<ROMDataAnalyser>
{
public:
	ROMDataAnalyser();
	~ROMDataAnalyser();

	bool hasEntry(std::string_view categoryName, uint32 address) const;
	void beginEntry(std::string_view categoryName, uint32 address);
	void endEntry();

	void addKeyValue(std::string_view key, std::string_view value);
	void beginObject(std::string_view key);
	void endObject();

private:
	struct Object
	{
		std::map<std::string, std::string> mKeyValuePairs;
		std::map<std::string, Object> mChildObjects;
	};
	struct Entry
	{
		Object mContent;
	};
	struct Category
	{
		std::string mName;
		std::map<uint32, Entry> mEntries;
	};

private:
	Category* findCategory(std::string_view categoryName, bool create);
	Entry* findEntry(std::string_view categoryName, uint32 address, bool create, Category** outCategory = nullptr);

	void loadDataFromJSONs(std::wstring_view filepath);
	void recursiveLoadDataFromJSON(const Json::Value& json, Object& outObject);
	void saveDataToJSONs(std::wstring_view filepath);
	void recursiveSaveDataToJSON(Json::Value& outJson, const Object& object);

	void processData();

private:
	std::map<uint64, Category> mCategories;

	// Write position
	Category* mCurrentCategory = nullptr;
	Entry* mCurrentEntry = nullptr;
	std::vector<Object*> mCurrentObjectStack;

	bool mAnyChange = false;
};
