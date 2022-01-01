/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#if defined (PLATFORM_ANDROID)

	#include "oxygen/platform/AndroidJavaInterface.h"
	#include "oxygen/application/Configuration.h"
	#include "oxygen/base/PlatformFunctions.h"
	#include "oxygen/helper/FileHelper.h"
	#include "oxygen/helper/Logging.h"

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
	}


	bool AndroidJavaInterface::hasRomFileAlready()
	{
		JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
		jobject activity = (jobject)SDL_AndroidGetActivity();
		jclass clazz(env->GetObjectClass(activity));
		jmethodID method_id = env->GetMethodID(clazz, "hasRomFileAlready", "()Z");
		jboolean result = env->CallBooleanMethod(activity, method_id);
		env->DeleteLocalRef(activity);
		env->DeleteLocalRef(clazz);
		return (result != 0);
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
		JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
		jobject activity = (jobject)SDL_AndroidGetActivity();
		jclass clazz(env->GetObjectClass(activity));
		jmethodID method_id = env->GetMethodID(clazz, "openRomFileSelectionDialog", "()V");
		env->CallVoidMethod(activity, method_id);
		env->DeleteLocalRef(activity);
		env->DeleteLocalRef(clazz);
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

#endif
