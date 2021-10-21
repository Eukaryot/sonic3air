/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


#ifdef DEBUG
	#define LEMON_DEBUG_BREAK(errorMessage) RMX_ERROR("Lemon script compilation failed:\n" << errorMessage, )
#else
	#define LEMON_DEBUG_BREAK(errorMessage)
#endif


#define CHECK_ERROR_NOLINE(expression, errorMessage) \
{ \
	if (!(expression)) \
	{ \
		LEMON_DEBUG_BREAK(errorMessage); \
		std::ostringstream stream; \
		stream << errorMessage; \
		throw CompilerException(stream.str().c_str()); \
	} \
}


#define CHECK_ERROR(expression, errorMessage, lineNumber) \
{ \
	if (!(expression)) \
	{ \
		LEMON_DEBUG_BREAK(errorMessage); \
		std::ostringstream stream; \
		stream << errorMessage; \
		throw CompilerException(stream.str().c_str(), lineNumber); \
	} \
}


namespace lemon
{

	class CompilerException : public std::runtime_error
	{
	public:
		inline CompilerException(const std::string& message, uint32 lineNumber = 0) : std::runtime_error(message), mLineNumber(lineNumber) {}

	public:
		uint32 mLineNumber;
	};

}
