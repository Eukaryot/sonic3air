/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/SourceFileInfo.h"
#include "lemon/program/Module.h"


namespace lemon
{
	std::wstring SourceFileInfo::getFullFilePath() const
	{
		return mModule->getScriptBasePath() + mLocalPath + mFilename;
	}

	std::wstring SourceFileInfo::getLocalFilePath() const
	{
		return mLocalPath + mFilename;
	}
}
