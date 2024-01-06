/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

#if defined(PLATFORM_ANDROID)

	class AndroidJavaInterface : public SingleInstance<AndroidJavaInterface>
	{
	public:
		enum class BinaryDialogResult
		{
			PENDING,	// No result yet
			FAILED,		// Dialog failed
			SUCCESS		// Dialog successful
		};

		struct RomFileInjection
		{
			BinaryDialogResult mDialogResult = BinaryDialogResult::PENDING;
			std::vector<uint8> mRomContent;
		};

	public:
		bool hasRomFileAlready();
		void openRomFileSelectionDialog();
		inline const RomFileInjection& getRomFileInjection() const  { return mRomFileInjection; }

		void onReceivedRomContent(const uint8* content, size_t bytes);
		void onRomContentSelectionFailed();

		uint64 startFileDownload(const char* url, const char* filenameUTF8);
		bool stopFileDownload(uint64 downloadId);
		void getDownloadStatus(uint64 downloadId, int& outStatus, uint64& outCurrentBytes, uint64& outTotalBytes);

	private:
		RomFileInjection mRomFileInjection;
	};

#endif
