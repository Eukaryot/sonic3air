/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class ArgumentsReader
{
public:
	// Read parameters
	std::wstring mExecutableCallPath;
	std::wstring mProjectPath;
	std::string mUrl;
	std::string mForwardedCommand;
	bool mStop = false;
	int mDisplayIndex = -1;

public:
	explicit ArgumentsReader(const char* urlSchemePrefix) : mUrlSchemePrefix(urlSchemePrefix) {}
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
			if (rmx::startsWith(parameter, mUrlSchemePrefix))
			{
				mUrl = parameter;
			}
			else if (parameter[0] == '-')
			{
				readParameterBase(parameter);
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
	void readParameterBase(const std::string& parameter)
	{
		if (rmx::startsWith(parameter, "-display="))
		{
			mDisplayIndex = (int)rmx::parseInteger(parameter.substr(9));
		}
		else if (rmx::startsWith(parameter, "-forward="))
		{
			mForwardedCommand = parameter.substr(9);
		}
		else if (parameter == "-stop")
		{
			// The stop parameter is meant to be used in conjunction with "-forward" or an URL, to ensure that the new
			// instance will close in any case - especially when there was no already running instance to forward to
			mStop = true;
		}
		else
		{
			// Pass it on to sub-class implementation
			readParameter(parameter);
		}
	}

	virtual bool readParameter(const std::string& parameter) { return false; }

protected:
	std::string mUrlSchemePrefix;	// Something like "oxygen://"
};
