/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


#ifdef int8
	#undef int8
#endif
#ifdef uint8
	#undef uint8
#endif
#ifdef int16
	#undef int16
#endif
#ifdef uint16
	#undef uint16
#endif
#ifdef int32
	#undef int32
#endif
#ifdef uint32
	#undef uint32
#endif
#ifdef int64
	#undef int64
#endif
#ifdef uint64
	#undef uint64
#endif


// Data types
typedef signed char		int8;
typedef unsigned char	uint8;
typedef signed short	int16;
typedef unsigned short	uint16;
typedef signed int		int32;
typedef unsigned int	uint32;
typedef int64_t			int64;
typedef uint64_t		uint64;
