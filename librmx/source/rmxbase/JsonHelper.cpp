/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace
{
	template<typename CharT, typename TraitsT = std::char_traits<CharT>>
	class vectorwrapbuf : public std::basic_streambuf<CharT, TraitsT>
	{
	public:
		vectorwrapbuf(std::vector<CharT> &vec)
		{
			// This is needed here
			this->setg(vec.data(), vec.data(), vec.data() + vec.size());
		}
	};
}


namespace rmx
{

	Json::Value JsonHelper::loadFile(const std::string& filename)
	{
		std::vector<uint8> content;
		if (FTX::FileSystem->readFile(filename, content))
		{
			return loadFromMemory(content);
		}
		return Json::Value();
	}

	Json::Value JsonHelper::loadFile(const std::wstring& filename)
	{
		std::vector<uint8> content;
		if (FTX::FileSystem->readFile(filename, content))
		{
			return loadFromMemory(content);
		}
		return Json::Value();
	}

	Json::Value JsonHelper::loadFromStream(std::istream& stream)
	{
		if (!stream.good())
			return Json::Value();

		stream.seekg(0, std::ios::end);
		const uint32 size = (uint32)stream.tellg();
		stream.seekg(0, std::ios::beg);

		std::vector<uint8> content;
		content.resize(size);
		stream.read((char*)content.data(), size);

		return loadFromMemory(content);
	}

	Json::Value JsonHelper::loadFromMemory(const std::vector<uint8>& content, std::string* outErrors)
	{
		Json::Value root;
		vectorwrapbuf<char> databuf(reinterpret_cast<std::vector<char>&>(const_cast<std::vector<uint8>&>(content)));
		std::istream str(&databuf);
		if (str.good())
		{
			Json::CharReaderBuilder rbuilder;
			rbuilder["collectComments"] = false;
			std::string errs;
			Json::parseFromStream(rbuilder, str, &root, &errs);

			if (!errs.empty())
			{
				if (nullptr != outErrors)
				{
					*outErrors = errs;
				}
				else
				{
					RMX_ASSERT(errs.empty(), "Error parsing JSON file: " + errs);
				}
			}
		}
		return root;
	}

	bool JsonHelper::saveFile(const std::wstring& filename, const Json::Value& value)
	{
		const String output(value.toStyledString());
		return FTX::FileSystem->saveFile(filename, (char*)*output, output.length());
	}


	JsonHelper::JsonHelper(const Json::Value& json) :
		mJson(json)
	{
	}

	bool JsonHelper::tryReadString(const std::string& key, std::string& output)
	{
		const Json::Value& value = mJson[key];
		if (value.isString())
		{
			output = value.asString();
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadString(const std::string& key, std::wstring& output)
	{
		const Json::Value& value = mJson[key];
		if (value.isString())
		{
			output = *String(value.asString()).toWString();		// TODO: Is there an encoding for non-ASCII characters?
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadInt(const std::string& key, int& output)
	{
		const Json::Value& value = mJson[key];
		if (value.isInt())
		{
			output = value.asInt();
			return true;
		}
		else if (value.isString())
		{
			output = String(value.asString()).parseInt();
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadInt(const std::string& key, uint8& output)
	{
		int result = 0;
		if (tryReadInt(key, result))
		{
			output = result;
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadBool(const std::string& key, bool& output)
	{
		const Json::Value& value = mJson[key];
		if (value.isBool())
		{
			output = value.asBool();
			return true;
		}
		else if (value.isInt())
		{
			output = (value.asInt() != 0);
			return true;
		}
		else if (value.isString())
		{
			String str(value.asString());
			if (str == "true")
				output = true;
			else if (str == "false")
				output = false;
			else
				output = (str.parseInt() != 0);
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadFloat(const std::string& key, float& output)
	{
		const Json::Value& value = mJson[key];
		if (value.isDouble())
		{
			output = (float)value.asDouble();
			return true;
		}
		else if (value.isInt())
		{
			output = (float)value.asInt();
			return true;
		}
		else if (value.isString())
		{
			output = String(value.asString()).parseFloat();
			return true;
		}
		return false;
	}

	bool JsonHelper::tryReadStringArray(const std::string& key, std::vector<std::string>& output)
	{
		output.clear();
		const Json::Value& value = mJson[key];
		if (value.isArray())
		{
			for (const auto& element : value)
			{
				output.push_back(element.asString());
			}
			return true;
		}
		return false;
	}

}
