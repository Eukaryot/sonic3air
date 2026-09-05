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
	JsonReader() = default;
	JsonReader(Json::Value& json) : mJsonRoot(&json), mCurrentJson(&json) {}

	void enterRoot();
	bool enterObject(std::string_view key);

	std::string getStringValue(std::string_view key);

private:
	const Json::Value* mJsonRoot = nullptr;
	const Json::Value* mCurrentJson = nullptr;

	// TODO: Add the hierarchy of parents to be able to go up
};
