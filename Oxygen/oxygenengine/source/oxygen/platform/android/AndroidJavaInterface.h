/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
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
			INACTIVE,	// Dialog not active
			PENDING,	// No result yet
			FAILED,		// Dialog failed
			SUCCESS		// Dialog successful
		};

		struct RomFileInjection
		{
			BinaryDialogResult mDialogResult = BinaryDialogResult::INACTIVE;
			std::vector<uint8> mRomContent;
		};

		struct FileSelection
		{
			BinaryDialogResult mDialogResult = BinaryDialogResult::INACTIVE;
			std::wstring mPath;
			std::vector<uint8> mFileContent;
		};

	public:
		inline const RomFileInjection& getRomFileInjection() const  { return mRomFileInjection; }
		bool hasRomFileAlready();
		void openRomFileSelectionDialog(const std::string& gameName);
		void onReceivedRomContent(const uint8* content, size_t bytes);
		void onRomContentSelectionFailed();

		inline FileSelection& getFileSelection()  { return mFileSelection; }
		void openFileSelectionDialog();
		void onFileImportSuccess(const uint8* content, size_t bytes, std::string_view path);
		void onFileImportFailed();

		void openFileExportDialog(const std::wstring& filename, const std::vector<uint8>& contents);

		void openFolderAccessDialog();

		uint64 startFileDownload(const std::string& urlUTF8, const std::string& filenameUTF8);
		bool stopFileDownload(uint64 downloadId);
		void getDownloadStatus(uint64 downloadId, int& outStatus, uint64& outCurrentBytes, uint64& outTotalBytes);

	private:
		RomFileInjection mRomFileInjection;
		FileSelection mFileSelection;
	};

#endif
