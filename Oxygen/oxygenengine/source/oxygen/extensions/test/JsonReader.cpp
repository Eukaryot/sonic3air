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

	lemon::StringRef jsonReader_getStringValue(JsonReaderWrapper jsonReaderWrapper, lemon::StringRef key)
	{
		JsonReader* jsonReader = jsonReaderWrapper.getJsonReader();
		if (nullptr != jsonReader)
		{
			const std::string str = jsonReader->getStringValue(key.getString());

			lemon::Runtime* runtime = lemon::Runtime::getActiveRuntime();
			RMX_ASSERT(nullptr != runtime, "No lemon script runtime active");
			return lemon::StringRef(runtime->addString(str));
		}
		return lemon::StringRef();
	}
}


void JsonReader::registerScriptBindings(lemon::ModuleBindingsBuilder& builder)
{
	// Using EXCLUDE_FROM_DEFINITIONS for the moment, so this won't show up in "cpp_core_functions.lemon"
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::EXCLUDE_FROM_DEFINITIONS);

	// Data type
	JsonReaderWrapper::mDataType = &builder.getModule().addCustomDataType("JsonReader", lemon::BaseType::UINT_64);

	// Methods
	builder.addNativeMethod("JsonReader", "enterRoot", lemon::wrap(functions::jsonReader_enterRoot), defaultFlags);
	builder.addNativeMethod("JsonReader", "enterObject", lemon::wrap(functions::jsonReader_enterObject), defaultFlags);
	builder.addNativeMethod("JsonReader", "getStringValue", lemon::wrap(functions::jsonReader_getStringValue), defaultFlags);
}

void JsonReader::enterRoot()
{
	mCurrentJson = mJsonRoot;
}

bool JsonReader::enterObject(std::string_view key)
{
	if (nullptr == mCurrentJson || key.empty())
		return false;

	const Json::Value* value = mCurrentJson->find(key.data(), key.data() + key.length());
	if (nullptr == value || !value->isObject())
		return false;

	mCurrentJson = value;
	return true;
}

std::string JsonReader::getStringValue(std::string_view key)
{
	if (nullptr == mCurrentJson || key.empty())
		return std::string();

	const Json::Value* value = mCurrentJson->find(key.data(), key.data() + key.length());
	if (nullptr == value)
		return std::string();

	return value->asString();
}
