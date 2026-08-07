/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/Configuration.h"


class ConfigurationImpl : public ::Configuration
{
public:
	ConfigurationImpl();

protected:
	inline void preLoadInitialization() override {}
	bool loadConfigurationInternal(JsonSerializer& serializer) override;
	bool loadSettingsInternal(JsonSerializer& serializer, SettingsType settingsType) override;
	inline void saveSettingsInternal(JsonSerializer& serializer, SettingsType settingsType) override {}
};
