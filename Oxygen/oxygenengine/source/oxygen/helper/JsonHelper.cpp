/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/JsonHelper.h"


namespace
{
	static std::vector<String> tempParts;
}


Json::Value JsonHelper::loadFile(const std::wstring& filename)
{
	std::vector<uint8> content;
	if (FTX::FileSystem->readFile(filename, content))
	{
		if (!content.empty())	// Silently ignore empty JSON files
		{
			std::string errors;
			Json::Value result = loadFromMemory(content, &errors);
			if (errors.empty())
				return result;

			RMX_ERROR("Error parsing JSON file '" << *WString(filename).toString() << "':\n" << errors, );
		}
	}
	return Json::Value();
}

bool JsonHelper::saveFile(const std::wstring& filename, const Json::Value& value)
{
	const String output(value.toStyledString());
	return FTX::FileSystem->saveFile(filename, *output, output.length());
}

bool JsonHelper::parseWString(std::wstring& output, const Json::Value::const_iterator& it)
{
	if (it->isString() && !it->asString().empty())
	{
		output = *String(it->asString()).toWString();
		return true;
	}
	return false;
}

bool JsonHelper::parseVec2i(Vec2i& output, const Json::Value::const_iterator& it)
{
	if (it->isString() && !it->asString().empty())
	{
		String(it->asString()).split(tempParts, ',');
		if (tempParts.size() == 2)
		{
			output.x = tempParts[0].parseInt();
			output.y = tempParts[1].parseInt();
			return true;
		}
	}
	return false;
}

bool JsonHelper::parseRecti(Recti& output, const Json::Value::const_iterator& it)
{
	if (it->isString() && !it->asString().empty())
	{
		String(it->asString()).split(tempParts, ',');
		if (tempParts.size() == 4)
		{
			output.x = tempParts[0].parseInt();
			output.y = tempParts[1].parseInt();
			output.width = tempParts[2].parseInt();
			output.height = tempParts[3].parseInt();
			return true;
		}
	}
	return false;
}
