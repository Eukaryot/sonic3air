/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "engineapp/pch.h"
#include "engineapp/ConfigurationImpl.h"

#include "oxygen/helper/JsonHelper.h"


ConfigurationImpl::ConfigurationImpl()
{
	// Change the default value for script optimization, it should be relatively low to not introduce wrong debugging outputs
	mScriptOptimizationLevel = 1;
}

bool ConfigurationImpl::loadConfigurationInternal(JsonSerializer& serializer)
{
	// Enable dev mode in any case
	Configuration::instance().mDevMode.mEnabled = true;

	return true;
}

bool ConfigurationImpl::loadSettingsInternal(JsonSerializer& serializer, SettingsType settingsType)
{
	return true;
}
