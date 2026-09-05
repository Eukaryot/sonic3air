/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/extensions/test/JsonReader.h"


// Just for testing stuff
class TestExtension : public SingleInstance<TestExtension>
{
public:
	static void registerScriptBindings(lemon::ModuleBindingsBuilder& builder);

public:
	void initialize();

	JsonReader& getMainJsonReader()		{ return mMainJsonReader; }
	JsonReader& getSecondJsonReader()	{ return mSecondJsonReader; }

private:
	JsonReader mMainJsonReader;
	JsonReader mSecondJsonReader;
};
