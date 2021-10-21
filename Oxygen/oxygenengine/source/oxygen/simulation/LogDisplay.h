/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class LogDisplay : public SingleInstance<LogDisplay>
{
friend class Application;

public:
	struct ScriptLogSingleEntry
	{
		std::string mValue;
		std::vector<uint64> mCallStack;
	};
	struct ScriptLogEntry
	{
		std::vector<ScriptLogSingleEntry> mEntries;
		uint32 mLastUpdate = 0;
	};
	typedef std::map<std::string, ScriptLogEntry> ScriptLogEntryMap;

	struct ColorLogEntry
	{
		std::string mName;
		std::vector<Color> mColors;
	};
	typedef std::vector<ColorLogEntry> ColorLogEntryArray;

public:
	// Mode display (single line)
	void setModeDisplay(const String& string);

	// Log display (single line)
	void setLogDisplay(const String& string, float time = 2.0f);

	// Log error display
	void clearLogErrors();
	void addLogError(const String& string);

	// Debug script log
	inline const ScriptLogEntryMap& getScriptLogEntries() const  { return mScriptLogEntries; }
	void clearScriptLogValues();
	void clearScriptLogValue(const std::string& key);
	ScriptLogSingleEntry& updateScriptLogValue(const std::string& key, const std::string& value);

	// Debug color log
	inline const ColorLogEntryArray& getColorLogEntries() const { return mColorLogEntries; }
	void clearColorLogEntries();
	void addColorLogEntry(const ColorLogEntry& entry);

private:
	String mModeDisplayString;

	String mLogDisplayString;
	float  mLogDisplayTimeout = 0.0f;

	std::vector<String> mLogErrorStrings;

	ScriptLogEntryMap mScriptLogEntries;
	ColorLogEntryArray mColorLogEntries;

	Vec2i mWorldSpaceOffset;
};
