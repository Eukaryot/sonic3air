/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

// Version number
#define RMXBASE_VERSION 0x00040100

// General includes
#include <cmath>
#include <float.h>
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <iostream>
#include <assert.h>
#include <vector>
#include <stack>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <algorithm>

// Libraries
#include "rmxbase/_jsoncpp/json/json.h"	// Uses its own namespace "Json"

// RMX modules
#include "PlatformDefinitions.h"
#include "export.h"
#include "rmxbase/base/Types.h"
#include "rmxbase/base/Basics.h"
#include "rmxbase/base/ErrorHandler.h"
#include "rmxbase/tools/BitArray.h"
#include "rmxbase/tools/BitFlagSet.h"
#include "rmxbase/math/Math.h"
#include "rmxbase/data/CArray.h"
#include "rmxbase/memory/ObjectPool.h"
#include "rmxbase/memory/OneTimeAllocPool.h"
#include "rmxbase/memory/UnalignedMemory.h"
#include "rmxbase/data/SingleInstance.h"
#include "rmxbase/data/Singleton.h"
#include "rmxbase/data/SinglePtr.h"
#include "rmxbase/data/SmartPtr.h"
#include "rmxbase/data/GlobalObjectPtr.h"
#include "rmxbase/memory/RC4Encryption.h"
#include "rmxbase/memory/String.h"
#include "rmxbase/tools/Tools.h"
#include "rmxbase/file/FileHandle.h"
#include "rmxbase/file/FileIO.h"
#include "rmxbase/file/FileProvider.h"
#include "rmxbase/file/RealFileProvider.h"
#include "rmxbase/file/FileSystem.h"
#include "rmxbase/file/FileCrawler.h"
#include "rmxbase/file/JsonHelper.h"
#include "rmxbase/memory/InputStream.h"
#include "rmxbase/memory/OutputStream.h"
#include "rmxbase/memory/BinarySerializer.h"
#include "rmxbase/memory/VectorBinarySerializer.h"
#include "rmxbase/memory/RmxDeflate.h"
#include "rmxbase/memory/ZlibDeflate.h"
#include "rmxbase/bitmap/Color.h"
#include "rmxbase/bitmap/BitmapCodecs.h"
#include "rmxbase/tools/Logging.h"


// Library linking via pragma
#if defined(PLATFORM_WINDOWS) && defined(RMX_LIB)
	#pragma comment(lib, "rmxbase.lib")
#endif



// Global object pointers
namespace FTX
{
	extern GlobalObjectPtr<rmx::FileSystem> FileSystem;
}


// This include depends on FTX::FileSystem, so add it afterwards
#include "rmxbase/memory/StringImpl.h"


// Initialization
namespace rmxbase
{
	void initialize();
	void getBuildInfo(String& info);
}

#define INIT_RMX  { rmxbase::initialize(); }
