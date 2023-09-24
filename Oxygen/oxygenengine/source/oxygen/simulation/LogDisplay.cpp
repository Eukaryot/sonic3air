/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LogDisplay.h"


void LogDisplay::setModeDisplay(const String& string)
{
	mModeDisplayString = string;
}

void LogDisplay::setLogDisplay(const String& string, float time)
{
	mLogDisplayString = string;
	mLogDisplayTimeout = time;
}

void LogDisplay::clearLogErrors()
{
	mLogErrorStrings.clear();
}

void LogDisplay::addLogError(const String& string)
{
	mLogErrorStrings.push_back(string);
}
