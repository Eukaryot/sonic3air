/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#if defined(PLATFORM_ANDROID)

	#include "oxygen/platform/android/AndroidJavaInterface.h"
	#include "oxygen/platform/android/AndroidJNIHelper.h"
	#include "oxygen/application/Configuration.h"
	#include "oxygen/helper/FileHelper.h"
	#include "oxygen/helper/Logging.h"
	#include "oxygen/platform/PlatformFunctions.h"

	#include <jni.h>

	extern "C"
	{
		JNIEXPORT void JNICALL Java_org_eukaryot_sonic3air_GameActivity_receivedRomContent(JNIEnv* env, jclass jclazz, jboolean success, jbyteArray data)
		{
			RMX_LOG_INFO("C++ receive ROM content... " << (success ? "success" : "failure"));
			AndroidJavaInterface& instance = AndroidJavaInterface::instance();
			if (success)
			{
				jboolean isCopy = false;
				jbyte* content = env->GetByteArrayElements(data, &isCopy);
				const jsize bytes = env->GetArrayLength(data);
				instance.onReceivedRomContent(reinterpret_cast<uint8*>(content), (size_t)bytes);
				env->ReleaseByteArrayElements(data, content, JNI_ABORT);
			}
			else
			{
				instance.onRomContentSelectionFailed();
			}
		}

		JNIEXPORT void JNICALL Java_org_eukaryot_sonic3air_GameActivity_receivedFileContent(JNIEnv* env, jclass jclazz, jboolean success, jbyteArray data, jstring path)
		{
			RMX_LOG_INFO("C++ receive file content... " << (success ? "success" : "failure"));
			AndroidJavaInterface& instance = AndroidJavaInterface::instance();
			if (success)
			{
				jboolean isCopy = false;
				jbyte* content = env->GetByteArrayElements(data, &isCopy);
				const jsize bytes = env->GetArrayLength(data);
				const char* str = env->GetStringUTFChars(path, 0);
				RMX_LOG_INFO("C++ receive file content - file name = " << str);

				instance.onFileImportSuccess(reinterpret_cast<uint8*>(content), (size_t)bytes, str);
				env->ReleaseByteArrayElements(data, content, JNI_ABORT);
			}
			else
			{
				instance.onFileImportFailed();
			}
		}

		JNIEXPORT void JNICALL Java_org_eukaryot_sonic3air_GameActivity_grantedFolderAccess(JNIEnv* env, jclass jclazz, jboolean success, jstring path)
		{
			const char* str = env->GetStringUTFChars(path, 0);
			RMX_LOG_INFO("C++ folder access path = " << str);

			// Example result: "/tree/primary:S3AIR_Savedata" (the path does not include a trailing slash)

			// TODO: Store the path somewhere, or call whoever might be interested in the result
		}
	}



	bool AndroidJavaInterface::hasRomFileAlready()
	{
		return AndroidJNIHelper().callBoolMethod("hasRomFileAlready");
	}

	void AndroidJavaInterface::openRomFileSelectionDialog(const std::string& gameName)
	{
		PlatformFunctions::showMessageBox("ROM required", "A " + gameName + " ROM must be added manually.\n\nUse the following file selection dialog to load it, e.g. from a location on your device like the Downloads folder.");

		// Check if the user opened the ROM file outside while the dialog was open
		if (mRomFileInjection.mDialogResult == BinaryDialogResult::SUCCESS)
			return;

		mRomFileInjection.mDialogResult = BinaryDialogResult::PENDING;

		// Open file selection dialog
		AndroidJNIHelper().callVoidMethod("openRomFileSelectionDialog");
	}

	void AndroidJavaInterface::onReceivedRomContent(const uint8* content, size_t bytes)
	{
		mRomFileInjection.mRomContent.resize(bytes);
		memcpy(&mRomFileInjection.mRomContent[0], content, bytes);
		mRomFileInjection.mDialogResult = BinaryDialogResult::SUCCESS;
	}

	void AndroidJavaInterface::onRomContentSelectionFailed()
	{
		mRomFileInjection.mDialogResult = BinaryDialogResult::FAILED;
	}


	void AndroidJavaInterface::openFileSelectionDialog()
	{
		mFileSelection.mDialogResult = BinaryDialogResult::PENDING;
		AndroidJNIHelper().callVoidMethod("openFileSelectionDialog");
	}

	void AndroidJavaInterface::onFileImportSuccess(const uint8* content, size_t bytes, std::string_view path)
	{
		WString wpath;
		wpath.fromUTF8(path);
		mFileSelection.mPath = wpath.toStdWString();

		mFileSelection.mFileContent.resize(bytes);
		memcpy(&mFileSelection.mFileContent[0], content, bytes);

		mFileSelection.mDialogResult = BinaryDialogResult::SUCCESS;
	}

	void AndroidJavaInterface::onFileImportFailed()
	{
		mFileSelection.mDialogResult = BinaryDialogResult::FAILED;
	}


	void AndroidJavaInterface::openFileExportDialog(const std::wstring& filename, const std::vector<uint8>& contents)
	{
		AndroidJNIHelper().callVoidMethod("openFileExportDialog", rmx::convertToUTF8(filename).c_str(), contents);
	}


	void AndroidJavaInterface::openFolderAccessDialog()
	{
		AndroidJNIHelper().callVoidMethod("openFolderAccessDialog");
	}


	uint64 AndroidJavaInterface::startFileDownload(const std::string& urlUTF8, const std::string& filenameUTF8)
	{
		return AndroidJNIHelper().callLongMethod("startFileDownload", urlUTF8.c_str(), filenameUTF8.c_str());
	}

	bool AndroidJavaInterface::stopFileDownload(uint64 downloadId)
	{
		return AndroidJNIHelper().callBoolMethod("stopFileDownload", downloadId);
	}

	void AndroidJavaInterface::getDownloadStatus(uint64 downloadId, int& outStatus, uint64& outCurrentBytes, uint64& outTotalBytes)
	{
		outCurrentBytes = 0;
		outTotalBytes = 0;
		outStatus = AndroidJNIHelper().callIntMethod("getDownloadStatus", downloadId);
		if (outStatus == 0x00)
			return;

		outTotalBytes = AndroidJNIHelper().callLongMethod("getDownloadTotalBytes", downloadId);
		if (outStatus <= 0x04)
		{
			// Download still active (running, pending, paused)
			outCurrentBytes = AndroidJNIHelper().callLongMethod("getDownloadCurrentBytes", downloadId);
		}
		else if (outStatus == 0x08)
		{
			// Finished
			outCurrentBytes = outTotalBytes;
		}
	}

#endif
