/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/download/Downloader.h"


class DownloadManager : public SingleInstance<DownloadManager>
{
public:
	const Downloader* getDownloadByKey(uint64 key) const;

	void startDownload(uint64 key, std::string_view url, std::wstring_view outputFilename);
	void removeDownload(uint64 key);

	void updateDownloads(float deltaSeconds);

private:
	void tryStartNextDownload();

private:
	std::map<uint64, Downloader> mAllDownloads;
	Downloader* mActiveDownload = nullptr;
	std::deque<Downloader*> mDownloadQueue;
};
