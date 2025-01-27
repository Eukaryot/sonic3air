/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#if defined(PLATFORM_ANDROID)

	#include "oxygen/platform/AndroidJavaInterface.h"
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

		JNIEXPORT void JNICALL Java_org_eukaryot_sonic3air_GameActivity_gotApkPath(JNIEnv* env, jclass jclazz, jstring path)
		{
			const char* str = env->GetStringUTFChars(path, 0);
			RMX_LOG_INFO("C++ APK path = " << str);

			FileHelper::extractZipFile(String(str).toStdWString(), Configuration::instance().mAppDataPath + L"apkContent/");
			env->ReleaseStringUTFChars(path, str);
		}

		JNIEXPORT void JNICALL Java_org_eukaryot_sonic3air_GameActivity_grantedFolderAccess(JNIEnv* env, jclass jclazz, jboolean success, jstring path)
		{
			const char* str = env->GetStringUTFChars(path, 0);
			RMX_LOG_INFO("C++ folder access path = " << str);

			// Example result: "/tree/primary:S3AIR_Savedata" (the path does not include a trailing slash)

			// TODO: Store the path somewhere, or call whoever might be interested in the result
		}
	}


	struct JNIStringParam
	{
		JNIStringParam(JNIEnv* env, const char* str) :
			mEnv(env)
		{
			mJString = mEnv->NewStringUTF(str);
		}

		~JNIStringParam()
		{
			if (nullptr != mJString)
				mEnv->DeleteLocalRef(mJString);
		}

		inline bool IsValid() const  { return nullptr != mJString; }
		inline jstring operator*() const  { return mJString; }

		JNIEnv* mEnv = nullptr;
		jstring mJString = nullptr;
	};

	struct JNICallHelper
	{
		JNICallHelper()
		{
			mEnv = (JNIEnv*)SDL_AndroidGetJNIEnv();
			mActivity = (jobject)SDL_AndroidGetActivity();
			mActivityClass = mEnv->GetObjectClass(mActivity);
		}

		~JNICallHelper()
		{
			mEnv->DeleteLocalRef(mActivity);
			mEnv->DeleteLocalRef(mActivityClass);
		}

		void callVoidMethod(const char* methodName)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "()V");
			mEnv->CallVoidMethod(mActivity, methodId);
		}

		bool callBooleanMethod(const char* methodName, uint64 a)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "(J)Z");
			return (bool)mEnv->CallBooleanMethod(mActivity, methodId, (jlong)a);
		}

		int callIntMethod(const char* methodName, uint64 a)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "(J)I");
            return mEnv->CallIntMethod(mActivity, methodId, (jlong)a);
		}

		uint64 callLongMethod(const char* methodName, uint64 a)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "(J)J");
			return (uint64)mEnv->CallLongMethod(mActivity, methodId, (jlong)a);
		}

		uint64 callLongMethod(const char* methodName, const char* a, const char* b)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "(Ljava/lang/String;Ljava/lang/String;)J");
			JNIStringParam stringA(mEnv, a);
			JNIStringParam stringB(mEnv, b);
			if (!stringA.IsValid() || !stringB.IsValid())
				return 0;
			return (uint64)mEnv->CallLongMethod(mActivity, methodId, *stringA, *stringB);
		}

		bool callBoolMethod(const char* methodName)
		{
			jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, "()Z");
			jboolean result = mEnv->CallBooleanMethod(mActivity, methodId);
			return (result != 0);
		}

		JNIEnv* mEnv = nullptr;
		jobject mActivity = nullptr;
		jclass mActivityClass = nullptr;
	};


	bool AndroidJavaInterface::hasRomFileAlready()
	{
		return JNICallHelper().callBoolMethod("hasRomFileAlready");
	}

	void AndroidJavaInterface::openRomFileSelectionDialog()
	{
		// TODO: This is S3AIR specific code...
		PlatformFunctions::showMessageBox("ROM required", "The original Sonic 3 & Knuckles Steam ROM must be added manually.\n\nUse the following file selection dialog to load it, e.g. from a location on your device like the Downloads folder.");

		// Check if the user opened the ROM file outside while the dialog was open
		if (mRomFileInjection.mDialogResult == BinaryDialogResult::SUCCESS)
			return;

		mRomFileInjection.mDialogResult = BinaryDialogResult::PENDING;

		// Open file selection dialog
		JNICallHelper().callVoidMethod("openRomFileSelectionDialog");
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

	uint64 AndroidJavaInterface::startFileDownload(const char* url, const char* filenameUTF8)
	{
		return JNICallHelper().callLongMethod("startFileDownload", url, filenameUTF8);
	}

	bool AndroidJavaInterface::stopFileDownload(uint64 downloadId)
	{
		return JNICallHelper().callBooleanMethod("stopFileDownload", downloadId);
	}

	void AndroidJavaInterface::getDownloadStatus(uint64 downloadId, int& outStatus, uint64& outCurrentBytes, uint64& outTotalBytes)
	{
		outCurrentBytes = 0;
		outTotalBytes = 0;
		outStatus = JNICallHelper().callIntMethod("getDownloadStatus", downloadId);
		if (outStatus == 0x00)
			return;

		outTotalBytes = JNICallHelper().callLongMethod("getDownloadTotalBytes", downloadId);
		if (outStatus <= 0x04)
		{
			// Download still active (running, pending, paused)
			outCurrentBytes = JNICallHelper().callLongMethod("getDownloadCurrentBytes", downloadId);
		}
		else if (outStatus == 0x08)
		{
			// Finished
			outCurrentBytes = outTotalBytes;
		}
	}

	void AndroidJavaInterface::openFolderAccessDialog()
	{
		JNICallHelper().callVoidMethod("openFolderAccessDialog");
	}

#endif
