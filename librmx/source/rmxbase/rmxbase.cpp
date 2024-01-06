/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"
#include "buildinfo.inc"


// Global object pointers
namespace FTX
{
	GlobalObjectPtr<rmx::FileSystem> FileSystem;
}


void rmxbase::initialize()
{
	// Checks for data types with fixed sizes
	assert(sizeof(int8) == 1);
	assert(sizeof(uint8) == 1);
	assert(sizeof(int16) == 2);
	assert(sizeof(uint16) == 2);
	assert(sizeof(int32) == 4);
	assert(sizeof(uint32) == 4);
	assert(sizeof(int64) == 8);

	// Setup defaults
	FTX::FileSystem.createDefault();

	// Initialize bitmap codecs
	Bitmap::mCodecs.add<BitmapCodecBMP>();
	Bitmap::mCodecs.add<BitmapCodecPNG>();
	Bitmap::mCodecs.add<BitmapCodecJPG>();
	Bitmap::mCodecs.add<BitmapCodecICO>();
}

void rmxbase::getBuildInfo(String& info)
{
	info.clear() << "rmxbase";
#ifdef DEBUG
	info << "D";
#endif
	String bnum;
	bnum << BUILD_NUMBER;
	bnum.fillLeft('0', 4);
	info << " " << RMXBASE_VERSION << "." << bnum << "  ";
#ifdef _MSC_VER
	info << "MSC " << (_MSC_VER / 100) << "." << (_MSC_VER % 100);
#elif defined(__GNUC__)
	info << "GCC " << __GNUC__ << "." << __GNUC_MINOR__ << "." << __GNUC_PATCHLEVEL__;
#else
	info << "unknown compiler";
#endif
}
