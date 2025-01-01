/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class JsonHelper : public rmx::JsonHelper
{
public:
	static Json::Value loadFile(const std::wstring& filename);
	static bool saveFile(const std::wstring& filename, const Json::Value& value);

	static bool parseWString(std::wstring& output, const Json::Value::const_iterator& it);
	static bool parseVec2i(Vec2i& output, const Json::Value::const_iterator& it);
	static bool parseRecti(Recti& output, const Json::Value::const_iterator& it);

public:
	inline JsonHelper(const Json::Value& json) : rmx::JsonHelper(json) {}
};
