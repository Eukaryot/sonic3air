/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxbase.h"


namespace rmx
{

	FileProvider::~FileProvider()
	{
		for (FileSystem* fileSystem : mRegisteredMountPointFileSystems)
		{
			fileSystem->onFileProviderDestroyed(*this);
		}
	}

}
