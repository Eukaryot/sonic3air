/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Downloader.h"

#ifdef PLATFORM_WINDOWS
	#define CURL_STATICLIB
	#include <curl/curl.h>

	#pragma comment(lib, "libcurl.lib")
	#pragma comment(lib, "ws2_32.lib")
	#pragma comment(lib, "wldap32.lib")
	#pragma comment(lib, "crypt32.lib")
#endif


bool Downloader::isDownloaderSupporter()
{
#ifdef PLATFORM_WINDOWS
	return true;
#else
	return false;
#endif
}

Downloader::~Downloader()
{
	if (nullptr != mThread)
	{
		mThread->join();
		delete mThread;
	}
}

void Downloader::startDownload(const std::string& url, const std::wstring& outputFilename)
{
	mURL = url;
	mState = State::RUNNING;
	mOutputFilename = outputFilename;
	mThread = new std::thread(&Downloader::performDownloadStatic, this);
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
	const size_t bytes = size * nmemb;
	const size_t written = mOutputFile.write(data, bytes);
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
	curl_easy_setopt(curl, CURLOPT_URL, mURL.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Downloader::writeDataStatic);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
	const CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);
	mOutputFile.close();
	mState = State::DONE;
#endif
}
