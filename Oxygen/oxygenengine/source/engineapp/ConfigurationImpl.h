#pragma once

#include "oxygen/application/Configuration.h"


class ConfigurationImpl : public ::Configuration
{
public:
	ConfigurationImpl();

protected:
	inline void preLoadInitialization() override {}
	bool loadConfigurationInternal(JsonHelper& jsonHelper) override;
	bool loadSettingsInternal(JsonHelper& jsonHelper, SettingsType settingsType) override;
	inline void saveSettingsInternal(Json::Value& root, SettingsType settingsType) override {}
};
