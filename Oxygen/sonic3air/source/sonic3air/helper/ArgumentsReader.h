/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
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
	std::string mUrl;

	bool mPack = false;
	bool mNativize = false;
	bool mDumpCppDefinitions = false;

public:
	void read(int argc, char** argv)
	{
		if (argc > 0)
		{
			WString wstr;
			wstr.fromUTF8(std::string(argv[0]));
			mExecutableCallPath = wstr.toStdWString();

			for (int i = 1; i < argc; ++i)
			{
				const std::string parameter(argv[i]);
				if (rmx::startsWith(parameter, "sonic3air://"))
				{
					mUrl = parameter;
				}
				else if (parameter == "-pack")
				{
					mPack = true;
				}
				else if (parameter == "-nativize")
				{
					mNativize = true;
				}
				else if (parameter == "-dumpcppdefinitions")
				{
					mDumpCppDefinitions = true;
				}
			}
		}
	}
};
