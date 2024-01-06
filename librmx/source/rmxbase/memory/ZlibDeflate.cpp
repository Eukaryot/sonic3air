/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include "zlib.h"

// Other platforms than Windows with Visual C++ need to the zlib library dependency into their build separately
#if defined(PLATFORM_WINDOWS) && defined(_MSC_VER)
	#pragma comment(lib, "zlib.lib")
#endif


bool ZlibDeflate::decode(std::vector<uint8>& output, const void* inputData, size_t inputSize)
{
	// Setup inflate
	z_stream strm;
	strm.zalloc = nullptr;
	strm.zfree = nullptr;
	strm.opaque = nullptr;
	int zlibResult = inflateInit(&strm);
	if (zlibResult != Z_OK)
		return false;

	const constexpr size_t CHUNK_SIZE = 0x4000;
	strm.next_in = (Bytef*)inputData;
	strm.avail_in = (uInt)inputSize;

	// Run inflate on input until output buffer not full
	do
	{
		const size_t outputOffset = output.size();
		output.resize(outputOffset + CHUNK_SIZE);
		strm.avail_out = CHUNK_SIZE;
		strm.next_out = &output[outputOffset];

		zlibResult = inflate(&strm, Z_NO_FLUSH);
		if (zlibResult != Z_OK && zlibResult != Z_STREAM_END)
		{
			inflateEnd(&strm);
			return false;
		}

		const unsigned int bytesWritten = CHUNK_SIZE - strm.avail_out;
		if (bytesWritten < CHUNK_SIZE)
		{
			// Reduce output size again
			output.resize(outputOffset + bytesWritten);
		}
	}
	while (strm.avail_out == 0);

	// Clean up
	zlibResult = inflateEnd(&strm);
	return (zlibResult == Z_STREAM_END || zlibResult == Z_OK);
}

bool ZlibDeflate::encode(std::vector<uint8>& output, const void* inputData, size_t inputSize, int compressionLevel)
{
	z_stream stream;
	stream.zalloc = (alloc_func)0;
	stream.zfree = (free_func)0;
	stream.opaque = (voidpf)0;

	int zlibResult = deflateInit(&stream, compressionLevel);
	if (zlibResult != Z_OK)
		return false;

	output.resize(inputSize + (inputSize + 999) / 1000 + 12);
	size_t outputLeft = output.size();

	stream.next_out = &output[0];
	stream.avail_out = (uInt)output.size();
	stream.next_in = (z_const Bytef*)inputData;
	stream.avail_in = (uInt)inputSize;

	zlibResult = deflate(&stream, Z_FINISH);

	output.resize((size_t)stream.total_out);
	deflateEnd(&stream);
	return (zlibResult == Z_STREAM_END || zlibResult == Z_OK);
}
