/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Downloader;


class RemasteredMusicDownload
{
public:
	enum class State
	{
		UNSUPPORTED,
		READY_FOR_DOWNLOAD,
		DOWNLOAD_PENDING,
		DOWNLOAD_RUNNING,
		DOWNLOAD_DONE,
		DOWNLOAD_FAILED,
		LOADED
	};

public:
	RemasteredMusicDownload();

	State getState();
	uint64 getBytesDownloaded() const;

	void startDownload();
	void removeDownload();
	void applyAfterDownload();

private:
	uint64 mDownloadID = 0;
	const Downloader* mCachedDownload = nullptr;
};
