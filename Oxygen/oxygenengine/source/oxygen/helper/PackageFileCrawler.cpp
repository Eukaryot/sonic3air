/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/PackageFileCrawler.h"


void PackageFileCrawler::addFiles(const WString& filemask, bool recursive)
{
	// Search with the file helper, to go through the file package
	FTX::FileSystem->listFilesByMask(*filemask, recursive, mFileEntries);
}
