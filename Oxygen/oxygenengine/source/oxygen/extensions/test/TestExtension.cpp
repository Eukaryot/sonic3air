/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/extensions/test/TestExtension.h"
#include "oxygen/extensions/test/JsonReaderWrapper.h"
#include "oxygen/helper/JsonHelper.h"

#include <lemon/program/ModuleBindingsBuilder.h>


namespace functions
{
	JsonReaderWrapper getMainJsonReader()
	{
		// Just use a fixed handle for now
		//  -> Also see "JsonReaderWrapper::getJsonReader"
		//  -> TODO: A more generic solution will likely require some central management of all JsonReaders...
		return JsonReaderWrapper(1);
	}

	JsonReaderWrapper getSecondJsonReader()
	{
		return JsonReaderWrapper(2);
	}
}


void TestExtension::registerScriptBindings(lemon::ModuleBindingsBuilder& builder)
{
	// Using EXCLUDE_FROM_DEFINITIONS for this test extension, so this won't show up in "cpp_core_functions.lemon"
	const BitFlagSet<lemon::Function::Flag> defaultFlags(lemon::Function::Flag::ALLOW_INLINE_EXECUTION, lemon::Function::Flag::EXCLUDE_FROM_DEFINITIONS);

	// Functions
	builder.addNativeFunction("TestExtension.getMainJsonReader", lemon::wrap(functions::getMainJsonReader), defaultFlags);
	builder.addNativeFunction("TestExtension.getSecondJsonReader", lemon::wrap(functions::getSecondJsonReader), defaultFlags);
}


void TestExtension::initialize()
{
	const std::string jsonContent = R"(
	{
		"Object1":
		{
			"Key1": "StringValue",
			"Key2": 123,
			"Key3": 0.12345
		},
		"Array1":
		[
			2, 3, 5, 7, 11, 13
		]
	})";

	mMainJson = JsonHelper::loadFromString(jsonContent);
	mMainJsonReader = JsonReader(mMainJson);		// Note: It's important to reset a JsonReader whenever its source JSON changes!

	mSecondJson = JsonHelper::loadFromString("{}");		// Just an empty JSON object
	mSecondJsonReader = JsonReader(mSecondJson);

	/*
		Example usage in scripts:

		JsonReader json = TestExtension.getMainJsonReader()
		json.enterRoot()
		if (json.enterObject("Object1"))
			debugLog("Value read: " + json.getStringValue("Key1"))
	*/
}
