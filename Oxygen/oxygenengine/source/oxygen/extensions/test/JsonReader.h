/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

namespace lemon
{
	class ModuleBindingsBuilder;
}


class JsonReader
{
public:
	static void registerScriptBindings(lemon::ModuleBindingsBuilder& builder);

public:
	bool loadFromString(const std::string& jsonContent);

	void enterRoot();
	bool enterObject(std::string_view key);
	bool enterArray(std::string_view key);
	bool leave();

	size_t getNumChildren() const;

	bool hasKey(std::string_view key) const;
	bool isString(std::string_view key) const;
	bool isInteger(std::string_view key) const;
	bool isDouble(std::string_view key) const;
	bool isObject(std::string_view key) const;
	bool isArray(std::string_view key) const;
	std::string getString(std::string_view key) const;
	int64 getInteger(std::string_view key) const;
	double getDouble(std::string_view key) const;

	bool hasIndex(int index) const;
	bool isString(int index) const;
	bool isInteger(int index) const;
	bool isDouble(int index) const;
	bool isObject(int index) const;
	bool isArray(int index) const;
	std::string getString(int index) const;
	int64 getInteger(int index) const;
	double getDouble(int index) const;

private:
	const Json::Value* getValueByKey(std::string_view key) const;
	const Json::Value* getValueByIndex(int index) const;

private:
	Json::Value mJsonRoot;

	const Json::Value* mCurrentJson = nullptr;
	std::vector<const Json::Value*> mHierarchy;		// Not including current location (i.e. what mCurrentJson points to)
};
