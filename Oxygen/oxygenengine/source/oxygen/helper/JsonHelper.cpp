/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/JsonHelper.h"


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
