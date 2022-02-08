/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Errors.h"


#ifdef DEBUG
	#define LEMON_DEBUG_BREAK(errorMessage) RMX_ERROR("Lemon script compilation failed:\n" << errorMessage, )
#elif 1
	#define LEMON_DEBUG_BREAK(errorMessage) if (rmx::ErrorHandling::isDebuggerAttached()) RMX_ERROR("Lemon script compilation failed:\n" << errorMessage, )
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

#define REPORT_ERROR_CODE(errorCode, data1, data2, errorMessage) \
{ \
	LEMON_DEBUG_BREAK(errorMessage); \
	std::ostringstream stream; \
	stream << errorMessage; \
	throw CompilerException(stream.str().c_str(), errorCode, data1, data2); \
}


namespace lemon
{

	class CompilerException : public std::runtime_error
	{
	public:
		inline CompilerException(const std::string& message, uint32 lineNumber = 0) :
			std::runtime_error(message)
		{
			mError.mLineNumber = lineNumber;
		}

		inline CompilerException(const std::string& message, CompilerError::Code errorCode, uint64 data1, uint64 data2, uint32 lineNumber = 0) :
			std::runtime_error(message)
		{
			mError.mCode = errorCode;
			mError.mData1 = data1;
			mError.mData2 = data2;
			mError.mLineNumber = lineNumber;
		}

	public:
		CompilerError mError;
	};

}
