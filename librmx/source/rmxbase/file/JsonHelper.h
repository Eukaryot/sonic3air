/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{
	class API_EXPORT JsonHelper
	{
	public:
		static Json::Value loadFile(const std::string& filename);
		static Json::Value loadFile(const std::wstring& filename);
		static Json::Value loadFromStream(std::istream& stream);
		static Json::Value loadFromMemory(const std::vector<uint8>& content, std::string* outErrors = nullptr);
		static bool saveFile(const std::wstring& filename, const Json::Value& value);

	public:
		JsonHelper(const Json::Value& json);

		bool tryReadString(const std::string& key, std::string& output);
		bool tryReadString(const std::string& key, std::wstring& output);
		bool tryReadInt(const std::string& key, int& output);
		bool tryReadInt(const std::string& key, uint8& output);
		bool tryReadBool(const std::string& key, bool& output);
		bool tryReadFloat(const std::string& key, float& output);
		bool tryReadStringArray(const std::string& key, std::vector<std::string>& output);

		template<typename T>
		bool tryReadAsInt(const std::string& key, T& output)
		{
			int value = 0;
			if (tryReadInt(key, value))
			{
				output = static_cast<T>(value);
				return true;
			}
			return false;
		}

	public:
		const Json::Value& mJson;
	};
}
