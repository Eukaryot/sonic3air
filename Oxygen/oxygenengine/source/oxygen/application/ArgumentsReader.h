/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class ArgumentsReader
{
public:
	std::wstring mExecutableCallPath;
	std::wstring mProjectPath;
	int mDisplayIndex = -1;

public:
	virtual ~ArgumentsReader()  {}

	void read(int argc, char** argv)
	{
		if (argc <= 0)
			return;

		WString wstr;
		wstr.fromUTF8(std::string(argv[0]));
		mExecutableCallPath = wstr.toStdWString();

		for (int i = 1; i < argc; ++i)
		{
			const std::string parameter(argv[i]);
			if (rmx::startsWith(parameter, "-display="))
			{
				mDisplayIndex = (int)rmx::parseInteger(parameter.substr(9));;
			}
			else if (parameter[0] == '-')
			{
				readParameter(parameter);
			}
			else
			{
				std::wstring path = String(parameter).toStdWString();
				FTX::FileSystem->normalizePath(path, true);
				mProjectPath = path;
			}
		}
	}

protected:
	virtual bool readParameter(const std::string& parameter)  { return false; }
};
