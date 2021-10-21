/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/simulation/Simulation.h"
#include "oxygen/application/Application.h"


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

void LogDisplay::clearScriptLogValues()
{
	mScriptLogEntries.clear();
}

void LogDisplay::clearScriptLogValue(const std::string& key)
{
	mScriptLogEntries.erase(key);
}

LogDisplay::ScriptLogSingleEntry& LogDisplay::updateScriptLogValue(const std::string& key, const std::string& value)
{
	const uint32 frameNumber = Application::instance().getSimulation().getFrameNumber();
	ScriptLogEntry& entry = mScriptLogEntries[key];
	if (frameNumber != entry.mLastUpdate)
	{
		entry.mEntries.clear();
		entry.mLastUpdate = frameNumber;
	}
	ScriptLogSingleEntry& singleEntry = vectorAdd(entry.mEntries);
	singleEntry.mValue = value;
	return singleEntry;
}

void LogDisplay::clearColorLogEntries()
{
	mColorLogEntries.clear();
}

void LogDisplay::addColorLogEntry(const ColorLogEntry& entry)
{
	mColorLogEntries.emplace_back(entry);
}
