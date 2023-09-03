/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
	// Mode display (single line)
	void setModeDisplay(const String& string);

	// Log display (single line)
	void setLogDisplay(const String& string, float time = 2.0f);

	// Log error display
	void clearLogErrors();
	void addLogError(const String& string);

private:
	String mModeDisplayString;

	String mLogDisplayString;
	float  mLogDisplayTimeout = 0.0f;

	std::vector<String> mLogErrorStrings;
};
