/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>
#include <atomic>
#include <thread>


class Downloader
{
public:
	enum class State
	{
		NONE,
		RUNNING,
		DONE,
		FAILED
	};

public:
	static bool isDownloaderSupported();

public:
	~Downloader();

	inline State getState() const  { return mState; }
	inline bool isRunning() const  { return mState == State::RUNNING; }
	inline uint64 getBytesDownloaded() const  { return mBytesDownloaded; }

	void setupDownload(std::string_view url, std::wstring_view outputFilename);
	void startDownload();
	void stopDownload();

private:
	static size_t writeDataStatic(void* data, size_t size, size_t nmemb, Downloader* downloader);
	static void performDownloadStatic(Downloader* downloader);

	size_t writeData(void* data, size_t size, size_t nmemb);
	void performDownload();

private:
	std::string mURL;
	std::thread* mThread = nullptr;
	State mState = State::NONE;
	std::wstring mOutputFilename;
	FileHandle mOutputFile;
	std::atomic<uint64> mBytesDownloaded = 0;
	std::atomic<bool> mThreadRunning = false;
};
