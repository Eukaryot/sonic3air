/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/download/Downloader.h"

#if defined(PLATFORM_WINDOWS)
	#define PLATFORM_SUPPORTS_DOWNLOADER
	#define USING_CURL

	#define CURL_STATICLIB
	#include <curl/curl.h>

	#pragma comment(lib, "libcurl.lib")
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "wldap32.lib")
	#pragma comment(lib, "crypt32.lib")

#elif defined(PLATFORM_LINUX) || defined(PLATFORM_MAC)
	#define PLATFORM_SUPPORTS_DOWNLOADER
	#define USING_CURL

	#define CURL_STATICLIB
	#include <curl/curl.h>

#elif defined(PLATFORM_ANDROID)
	#define PLATFORM_SUPPORTS_DOWNLOADER
	#include "oxygen/platform/android/AndroidJavaInterface.h"
#endif


bool Downloader::isDownloaderSupported()
{
#ifdef PLATFORM_SUPPORTS_DOWNLOADER
	return true;
#else
	return false;
#endif
}

Downloader::~Downloader()
{
	stopDownload();
}

void Downloader::setupDownload(std::string_view url, std::wstring_view outputFilename)
{
	mURL = url;
	mOutputFilename = outputFilename;
}

void Downloader::startDownload()
{
	mState = State::RUNNING;
	mThread = new std::thread(&Downloader::performDownloadStatic, this);
}

void Downloader::stopDownload()
{
	// Stop the thread if it's running
	if (nullptr != mThread)
	{
		mThreadRunning = false;
		mThread->join();
		delete mThread;
		mThread = nullptr;
	}

	if (mState == State::RUNNING || mState == State::FAILED)
	{
		// Delete the output file
		FTX::FileSystem->removeFile(Configuration::instance().mAppDataPath + mOutputFilename);
	}
	mState = State::NONE;
}

size_t Downloader::writeDataStatic(void* data, size_t size, size_t nmemb, Downloader* downloader)
{
	return downloader->writeData(data, size, nmemb);
}

void Downloader::performDownloadStatic(Downloader* downloader)
{
	downloader->performDownload();
}

size_t Downloader::writeData(void* data, size_t size, size_t nmemb)
{
	// Check for a signal to stop the download
	if (!mThreadRunning)
		return 0;

	// Write data to the output file
	const size_t bytes = size * nmemb;
	const size_t written = mOutputFile.write(data, bytes);
	mBytesDownloaded += written;
	return written;
}

void Downloader::performDownload()
{
#if defined(USING_CURL)

	CURL* curl = curl_easy_init();
	if (nullptr == curl)
	{
		mState = State::FAILED;
		return;
	}

	mOutputFile.open(Configuration::instance().mAppDataPath + mOutputFilename, FILE_ACCESS_WRITE);
	mThreadRunning = true;

	curl_easy_setopt(curl, CURLOPT_URL, mURL.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Downloader::writeDataStatic);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	mOutputFile.close();

	// Check if aborted
	if (mThreadRunning)
	{
		mThreadRunning = false;
		mState = State::DONE;
	}
	else
	{
		// TODO: Delete the output file
		mState = State::FAILED;
	}

#elif defined(PLATFORM_ANDROID)

	AndroidJavaInterface& javaInterface = AndroidJavaInterface::instance();
	const uint64 downloadId = javaInterface.startFileDownload(mURL.c_str(), *WString(mOutputFilename).toUTF8());

	// The download runs in its own thread, so we just have to wait here...
	mThreadRunning = true;
	while (true)
	{
		if (!mThreadRunning)
		{
			// Download was stopped by the user
			javaInterface.stopFileDownload(downloadId);
			mState = State::FAILED;
			return;
		}

		// States:
		//  0x00 = Invalid download request
		//  0x01 = Pending download
		//  0x02 = Running
		//  0x04 = Paused
		//  0x08 = Finished successfully
		//  0x10 = Failed
		int state;
		uint64 bytesDownloaded;
		uint64 bytesTotal;
		javaInterface.getDownloadStatus(downloadId, state, bytesDownloaded, bytesTotal);
		mBytesDownloaded = bytesDownloaded;

		if (state == 0x00 || state == 0x10)
		{
			mState = State::FAILED;
			return;
		}
		else if (state == 0x08)
		{
			mThreadRunning = false;
			mState = State::DONE;
			return;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(250));
	}

#endif
}
