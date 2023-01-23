/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygenserver/pch.h"
#include "oxygenserver/Configuration.h"


Configuration::Configuration()
{
	mSingleInstance = this;
}

bool Configuration::loadConfiguration(const std::wstring& filename)
{
	// Open file
	const Json::Value root = rmx::JsonHelper::loadFile(filename);
	if (root.isNull())
		return false;
	rmx::JsonHelper rootHelper(root);

	rootHelper.tryReadAsInt("UDPPort", mUDPPort);
	rootHelper.tryReadAsInt("TCPPort", mTCPPort);
	return true;
}
