/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class JsonHelper;


class Configuration
{
public:
	inline static bool hasInstance()		 { return (nullptr != mSingleInstance); }
	inline static Configuration& instance()  { return *mSingleInstance; }

public:
	Configuration();

	bool loadConfiguration(const std::wstring& filename);

public:
	// Server setup
	uint16 mUDPPort = 21094;
	uint16 mTCPPort = 21095;

private:
	static inline Configuration* mSingleInstance = nullptr;
};
