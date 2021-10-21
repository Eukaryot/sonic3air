/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class Log
{
public:
	static void startup(const std::wstring& filename);
	static void shutdown();

	static void log(const std::string& string);
};


#define LOG_INFO(_message_) \
	{ \
		std::ostringstream stream; \
		stream << _message_; \
		Log::log(stream.str()); \
	}
