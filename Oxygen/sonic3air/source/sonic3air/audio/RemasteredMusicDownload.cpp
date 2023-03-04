/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/audio/RemasteredMusicDownload.h"
#include "sonic3air/audio/AudioOut.h"

#include "oxygen/download/DownloadManager.h"


RemasteredMusicDownload::RemasteredMusicDownload() :
	mDownloadID(rmx::getMurmur2_64("RemasteredSoundtrack"))
{
}

RemasteredMusicDownload::State RemasteredMusicDownload::getState()
{
	if (!Downloader::isDownloaderSupported())
		return State::UNSUPPORTED;

	mCachedDownload = DownloadManager::instance().getDownloadByKey(mDownloadID);
	if (nullptr != mCachedDownload)
	{
		switch (mCachedDownload->getState())
		{
			case Downloader::State::NONE:		return State::DOWNLOAD_PENDING;
			case Downloader::State::RUNNING:	return State::DOWNLOAD_RUNNING;
			case Downloader::State::DONE:		return State::DOWNLOAD_DONE;
			case Downloader::State::FAILED:		return State::DOWNLOAD_FAILED;
		}
	}
	return State::READY_FOR_DOWNLOAD;
}

uint64 RemasteredMusicDownload::getBytesDownloaded() const
{
	return mCachedDownload->getBytesDownloaded();
}

void RemasteredMusicDownload::startDownload()
{
	DownloadManager::instance().startDownload(mDownloadID, "https://sonic3air.org/download/audioremaster.bin", L"data/audioremaster.bin");
}

void RemasteredMusicDownload::removeDownload()
{
	DownloadManager::instance().removeDownload(mDownloadID);
}

void RemasteredMusicDownload::applyAfterDownload()
{
	// Load remastered music package
	EngineMain::instance().reloadFilePackage(L"audioremaster.bin", false);
	AudioOut::instance().reloadRemasteredSoundtrack();
}
