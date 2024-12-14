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
