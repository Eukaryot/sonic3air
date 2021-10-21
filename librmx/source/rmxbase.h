/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
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
#include "rmxbase/jsoncpp/json/json.h"	// Uses its own namespace "Json"

// RMX modules
#include "PlatformDefinitions.h"
#include "export.h"
#include "rmxbase/Types.h"
#include "rmxbase/Basics.h"
#include "rmxbase/ErrorHandler.h"
#include "rmxbase/Math.h"
#include "rmxbase/Sort.h"
#include "rmxbase/CArray.h"
#include "rmxbase/ObjectPool.h"
#include "rmxbase/SingleInstance.h"
#include "rmxbase/Singleton.h"
#include "rmxbase/SinglePtr.h"
#include "rmxbase/SmartPtr.h"
#include "rmxbase/GlobalObjectPtr.h"
#include "rmxbase/RC4Encryption.h"
#include "rmxbase/String.h"
#include "rmxbase/Tools.h"
#include "rmxbase/FileHandle.h"
#include "rmxbase/FileIO.h"
#include "rmxbase/FileProvider.h"
#include "rmxbase/FileSystem.h"
#include "rmxbase/FileCrawler.h"
#include "rmxbase/JsonHelper.h"
#include "rmxbase/InputStream.h"
#include "rmxbase/OutputStream.h"
#include "rmxbase/BinarySerializer.h"
#include "rmxbase/VectorBinarySerializer.h"
#include "rmxbase/RmxDeflate.h"
#include "rmxbase/ZlibDeflate.h"
#include "rmxbase/Color.h"
#include "rmxbase/Bitmap.h"


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
#include "rmxbase/StringImpl.h"


// Initialization
namespace rmxbase
{
	void initialize();
	void getBuildInfo(String& info);
}

#define INIT_RMX  { rmxbase::initialize(); }
