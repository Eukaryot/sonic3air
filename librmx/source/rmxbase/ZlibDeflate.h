/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace ZlibDeflate
{
	FUNCTION_EXPORT bool decode(std::vector<uint8>& output, const void* inputData, size_t inputSize);
	FUNCTION_EXPORT bool encode(std::vector<uint8>& output, const void* inputData, size_t inputSize, int compressionLevel = 5);
}
