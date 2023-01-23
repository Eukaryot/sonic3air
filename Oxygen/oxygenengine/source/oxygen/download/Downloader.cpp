/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/download/Downloader.h"

#ifdef PLATFORM_WINDOWS
	#define CURL_STATICLIB
	#include <curl/curl.h>

	#pragma comment(lib, "libcurl.lib")
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "wldap32.lib")
	#pragma comment(lib, "crypt32.lib")
#endif


bool Downloader::isDownloaderSupported()
{
#ifdef PLATFORM_WINDOWS
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
#ifdef PLATFORM_WINDOWS
	CURL* curl = curl_easy_init();
	if (nullptr == curl)
	{
		mState = State::FAILED;
		return;
	}

	mOutputFile.open(mOutputFilename, FILE_ACCESS_WRITE);
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
#endif
}
