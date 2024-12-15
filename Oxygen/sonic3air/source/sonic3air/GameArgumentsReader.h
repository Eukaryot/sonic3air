/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/ArgumentsReader.h"


class GameArgumentsReader : public ArgumentsReader
{
public:
	std::string mUrl;
	bool mPack = false;
	bool mNativize = false;
	bool mDumpCppDefinitions = false;

protected:
	virtual bool readParameter(const std::string& parameter) override
	{
		if (rmx::startsWith(parameter, "sonic3air://"))
		{
			mUrl = parameter;
			return true;
		}
		else if (parameter == "-pack")
		{
			mPack = true;
			return true;
		}
		else if (parameter == "-nativize")
		{
			mNativize = true;
			return true;
		}
		else if (parameter == "-dumpcppdefinitions")
		{
			mDumpCppDefinitions = true;
			return true;
		}
		return false;
	}
};
