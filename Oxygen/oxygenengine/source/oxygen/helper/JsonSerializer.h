/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class JsonSerializer
{
public:
	inline JsonSerializer(bool read, Json::Value& json) : mReading(read), mCurrentJson(&json) {}
	inline explicit JsonSerializer(const Json::Value& json) : mReading(true), mCurrentJson(const_cast<Json::Value*>(&json)) {}

	bool isReading() const			{ return mReading; }
	Json::Value& getCurrentJson()	{ return *mCurrentJson; }

	bool serialize(const char* key, bool& value);
	bool serialize(const char* key, int& value);
	bool serialize(const char* key, float& value);
	bool serialize(const char* key, std::string& value);
	bool serialize(const char* key, std::wstring& value);

	bool serializeHexValue(const char* key, int& value, int numHexDigits);
	bool serializeComponents(const char* key, Vec2i& value);
	bool serializeVectorAsSizeString(const char* key, Vec2i& value);
	bool serializeHexColorRGB(const char* key, Color& value);
	bool serializeArray(const char* key, std::vector<std::string>& value);

	template <typename T, typename S>
	bool serializeAs(const char* key, S& value)
	{
		if (mReading)
		{
			T targetTypeValue;
			if (!serialize(key, targetTypeValue))
				return false;

			value = static_cast<S>(targetTypeValue);
			return true;
		}
		else
		{
			T targetTypeValue = static_cast<T>(value);
			return serialize(key, targetTypeValue);
		}
	}

	bool beginObject(const char* key);
	void endObject();

private:
	bool mReading = false;
	Json::Value* mCurrentJson = nullptr;
	std::vector<Json::Value*> mObjectStack;		// Not including the current JSON obejct
};
