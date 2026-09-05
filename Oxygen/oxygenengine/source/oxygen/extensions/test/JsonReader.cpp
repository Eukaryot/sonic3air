/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/extensions/test/JsonReader.h"
#include "oxygen/extensions/test/JsonReaderWrapper.h"
#include "oxygen/helper/JsonHelper.h"

#include <lemon/program/ModuleBindingsBuilder.h>


namespace functions
{
	void jsonReader_enterRoot(JsonReaderWrapper jsonReaderWrapper)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			jsonReader->enterRoot();
		}
	}

	bool jsonReader_enterObject(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			return jsonReader->enterObject(key.getString());
		}
		return false;
	}

	bool jsonReader_enterArray(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			return jsonReader->enterArray(key.getString());
		}
		return false;
	}

	bool jsonReader_leave(JsonReaderWrapper jsonReaderWrapper)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			return jsonReader->leave();
		}
		return false;
	}

	bool jsonReader_hasKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->hasKey(key.getString()));
	}

	bool jsonReader_isStringByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isString(key.getString()));
	}

	bool jsonReader_isIntegerByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isInteger(key.getString()));
	}

	bool jsonReader_isDoubleByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isDouble(key.getString()));
	}

	bool jsonReader_isObjectByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isObject(key.getString()));
	}

	bool jsonReader_isArrayByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isArray(key.getString()));
	}

	lemon::StringRef jsonReader_getStringByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			const std::string str = jsonReader->getString(key.getString());

			lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			return lemon::StringRef(runtime->addString(str));
		}
		return lemon::StringRef();
	}

	int64 jsonReader_getIntegerByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader) ? jsonReader->getInteger(key.getString()) : 0;
	}

	double jsonReader_getDoubleByKey(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader) ? jsonReader->getDouble(key.getString()) : 0.0;
	}

	bool jsonReader_hasIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->hasIndex((int)index));
	}

	bool jsonReader_isStringByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isString((int)index));
	}

	bool jsonReader_isIntegerByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isInteger((int)index));
	}

	bool jsonReader_isDoubleByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isDouble((int)index));
	}

	bool jsonReader_isObjectByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isObject((int)index));
	}

	bool jsonReader_isArrayByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader && jsonReader->isArray((int)index));
	}

	lemon::StringRef jsonReader_getStringByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			const std::string str = jsonReader->getString((int)index);

			lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			return lemon::StringRef(runtime->addString(str));
		}
		return lemon::StringRef();
	}

	int64 jsonReader_getIntegerByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader) ? jsonReader->getInteger((int)index) : 0;
	}

	double jsonReader_getDoubleByIndex(JsonReaderWrapper jsonReaderWrapper, uint32 index)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader) ? jsonReader->getDouble((int)index) : 0.0;
	}

	uint32 jsonReader_getNumChildren(JsonReaderWrapper jsonReaderWrapper)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		return (nullptr != jsonReader) ? (uint32)jsonReader->getNumChildren() : 0;
	}
}


void JsonReader::registerScriptBindings(lemon::ModuleBindingsBuilder& builder)
{
	// Using EXCLUDE_FROM_DEFINITIONS for the moment, so this won't show up in "cpp_core_functions.lemon"
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::EXCLUDE_FROM_DEFINITIONS);

	// Data type
	JsonReaderWrapper::mDataType = &builder.getModule().addCustomDataType("JsonReader", lemon::BaseType::UINT_64);

	// Methods
	{
		// Navigation
		builder.addNativeMethod("JsonReader", "enterRoot", lemon::wrap(functions::jsonReader_enterRoot), defaultFlags);
		builder.addNativeMethod("JsonReader", "enterObject", lemon::wrap(functions::jsonReader_enterObject), defaultFlags);
		builder.addNativeMethod("JsonReader", "enterArray", lemon::wrap(functions::jsonReader_enterArray), defaultFlags);
		builder.addNativeMethod("JsonReader", "leave", lemon::wrap(functions::jsonReader_leave), defaultFlags);

		// For a JSON object
		builder.addNativeMethod("JsonReader", "hasKey", lemon::wrap(functions::jsonReader_hasKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "isString",  lemon::wrap(functions::jsonReader_isStringByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "isInteger", lemon::wrap(functions::jsonReader_isIntegerByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "isDouble",  lemon::wrap(functions::jsonReader_isDoubleByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "isObject",  lemon::wrap(functions::jsonReader_isObjectByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "isArray",   lemon::wrap(functions::jsonReader_isArrayByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "getString",  lemon::wrap(functions::jsonReader_getStringByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "getInteger", lemon::wrap(functions::jsonReader_getIntegerByKey), defaultFlags);
		builder.addNativeMethod("JsonReader", "getDouble",  lemon::wrap(functions::jsonReader_getDoubleByKey), defaultFlags);

		// For a JSON array
		builder.addNativeMethod("JsonReader", "hasIndex", lemon::wrap(functions::jsonReader_hasIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "isString",  lemon::wrap(functions::jsonReader_isStringByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "isInteger", lemon::wrap(functions::jsonReader_isIntegerByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "isDouble",  lemon::wrap(functions::jsonReader_isDoubleByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "isObject",  lemon::wrap(functions::jsonReader_isObjectByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "isArray",   lemon::wrap(functions::jsonReader_isArrayByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "getString",  lemon::wrap(functions::jsonReader_getStringByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "getInteger", lemon::wrap(functions::jsonReader_getIntegerByIndex), defaultFlags);
		builder.addNativeMethod("JsonReader", "getDouble",  lemon::wrap(functions::jsonReader_getDoubleByIndex), defaultFlags);

		// Misc
		builder.addNativeMethod("JsonReader", "getNumChildren", lemon::wrap(functions::jsonReader_getNumChildren), defaultFlags);
	}
}

bool JsonReader::loadFromString(const std::string& jsonContent)
{
	mJsonRoot = JsonHelper::loadFromString(jsonContent);
	enterRoot();
	return mJsonRoot.isObject();
}

void JsonReader::enterRoot()
{
	mCurrentJson = &mJsonRoot;
	mHierarchy.clear();
}

bool JsonReader::enterObject(std::string_view key)
{
	if (nullptr == mCurrentJson || key.empty())
		return false;

	const Json::Value* value = mCurrentJson->find(key.data(), key.data() + key.length());
	if (nullptr == value || !value->isObject())
		return false;

	mHierarchy.push_back(mCurrentJson);
	mCurrentJson = value;
	return true;
}

bool JsonReader::enterArray(std::string_view key)
{
	if (nullptr == mCurrentJson || key.empty())
		return false;

	const Json::Value* value = mCurrentJson->find(key.data(), key.data() + key.length());
	if (nullptr == value || !value->isArray())
		return false;

	mHierarchy.push_back(mCurrentJson);
	mCurrentJson = value;
	return true;
}

bool JsonReader::leave()
{
	if (nullptr == mCurrentJson || mHierarchy.empty())
		return false;

	mCurrentJson = mHierarchy.back();
	mHierarchy.pop_back();
	return true;
}

size_t JsonReader::getNumChildren() const
{
	if (nullptr == mCurrentJson || (!mCurrentJson->isArray() && !mCurrentJson->isObject()))
		return 0;
	return (size_t)mCurrentJson->size();
}

bool JsonReader::hasKey(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value);
}

bool JsonReader::isString(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value && value->isString());
}

bool JsonReader::isInteger(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value && value->isInt());
}

bool JsonReader::isDouble(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value && value->isDouble());
}

bool JsonReader::isObject(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value && value->isObject());
}

bool JsonReader::isArray(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value && value->isArray());
}

std::string JsonReader::getString(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value) ? value->asString() : std::string();
}

int64 JsonReader::getInteger(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value) ? value->asInt64() : 0;
}

double JsonReader::getDouble(std::string_view key) const
{
	const Json::Value* value = getValueByKey(key);
	return (nullptr != value) ? value->asDouble() : 0.0;
}

bool JsonReader::hasIndex(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value);
}

bool JsonReader::isString(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value && value->isString());
}

bool JsonReader::isInteger(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value && value->isInt());
}

bool JsonReader::isDouble(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value && value->isDouble());
}

std::string JsonReader::getString(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value) ? value->asString() : std::string();
}

int64 JsonReader::getInteger(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value) ? value->asInt64() : 0;
}

double JsonReader::getDouble(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value) ? value->asDouble() : 0.0;
}

bool JsonReader::isObject(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value && value->isObject());
}

bool JsonReader::isArray(int index) const
{
	const Json::Value* value = getValueByIndex(index);
	return (nullptr != value && value->isArray());
}

const Json::Value* JsonReader::getValueByKey(std::string_view key) const
{
	if (nullptr == mCurrentJson || !mCurrentJson->isObject() || key.empty())
		return nullptr;
	return mCurrentJson->find(key.data(), key.data() + key.length());
}

const Json::Value* JsonReader::getValueByIndex(int index) const
{
	if (nullptr == mCurrentJson || !mCurrentJson->isArray() || !mCurrentJson->isValidIndex(index))
		return nullptr;
	return &(*mCurrentJson)[index];
}
