/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"


namespace rmx
{
#if defined(__arm__) && !defined(__vita__)

	// Do not access memory directly, but byte-wise to avoid "SIGBUS illegal alignment" issues (this can happen e.g. in Android Release builds)

	template<>
	uint16 readMemoryUnaligned(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint16)ptr[0]) + ((uint16)ptr[1] << 8);
	}

	template<>
	uint32 readMemoryUnaligned(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint32)ptr[0]) + ((uint32)ptr[1] << 8) + ((uint32)ptr[2] << 16) + ((uint32)ptr[3] << 24);
	}

	template<>
	uint64 readMemoryUnaligned(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint64)ptr[0]) + ((uint64)ptr[1] << 8) + ((uint64)ptr[2] << 16) + ((uint64)ptr[3] << 24) + ((uint64)ptr[4] << 32) + ((uint64)ptr[5] << 40) + ((uint64)ptr[6] << 48) + ((uint64)ptr[7] << 56);
	}

	template<>
	uint16 readMemoryUnalignedSwapped(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint16)ptr[1]) + ((uint16)ptr[0] << 8);
	}

	template<>
	uint32 readMemoryUnalignedSwapped(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint32)ptr[3]) + ((uint32)ptr[2] << 8) + ((uint32)ptr[1] << 16) + ((uint32)ptr[0] << 24);
	}

	template<>
	uint64 readMemoryUnalignedSwapped(const void* pointer)
	{
		const uint8* ptr = (uint8*)pointer;
		return ((uint64)ptr[7]) + ((uint64)ptr[6] << 8) + ((uint64)ptr[5] << 16) + ((uint64)ptr[4] << 24) + ((uint64)ptr[3] << 32) + ((uint64)ptr[2] << 40) + ((uint64)ptr[1] << 48) + ((uint64)ptr[0] << 56);
	}

#endif
}
