/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/download/DownloadManager.h"


const Downloader* DownloadManager::getDownloadByKey(uint64 key) const
{
	return mapFind(mAllDownloads, key);
}

void DownloadManager::startDownload(uint64 key, std::string_view url, std::wstring_view outputFilename)
{
	if (!Downloader::isDownloaderSupported())
		return;

	// Remove old download with the same key, if there is one
	removeDownload(key);

	// Create a new download
	Downloader& download = mAllDownloads[key];
	download.setupDownload(url, outputFilename);
	mDownloadQueue.push_back(&download);

	// Start the download immediately if no other is running
	tryStartNextDownload();
}

void DownloadManager::removeDownload(uint64 key)
{
	const auto it = mAllDownloads.find(key);
	if (it == mAllDownloads.end())
		return;

	Downloader* download = &it->second;
	if (download == mActiveDownload)
	{
		download->stopDownload();
		mActiveDownload = nullptr;
	}
	else
	{
		// Remove it from the queue as well, if it's enqueued
		for (auto it2 = mDownloadQueue.begin(); it2 != mDownloadQueue.end(); ++it2)
		{
			if (*it2 == download)
			{
				mDownloadQueue.erase(it2);
				break;
			}
		}
	}

	mAllDownloads.erase(it);
}

void DownloadManager::updateDownloads(float deltaSeconds)
{
	if (nullptr != mActiveDownload)
	{
		// Update running download
		if (mActiveDownload->getState() == Downloader::State::RUNNING)
		{
			// Do nothing here until the active download changes its state
			return;
		}

		// Active download has finished
		//  -> Now first make sure the download is really stopped
		//  -> And afterwards, start the next download
		mActiveDownload->stopDownload();
		mActiveDownload = nullptr;
	}

	tryStartNextDownload();
}

void DownloadManager::tryStartNextDownload()
{
	if (nullptr == mActiveDownload && !mDownloadQueue.empty())
	{
		// Start new download
		mActiveDownload = mDownloadQueue.front();
		mActiveDownload->startDownload();
		mDownloadQueue.pop_front();
	}
}
