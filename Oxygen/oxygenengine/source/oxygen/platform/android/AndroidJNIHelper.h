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

	#include <jni.h>

	class AndroidJNIHelper
	{
	public:
		AndroidJNIHelper();
		~AndroidJNIHelper();

		void callVoidMethod(const char* methodName);
		void callVoidMethod(const char* methodName, const char* a, const std::vector<uint8>& b);
		bool callBoolMethod(const char* methodName);
		bool callBoolMethod(const char* methodName, uint64 a);
		int callIntMethod(const char* methodName, uint64 a);
		uint64 callLongMethod(const char* methodName, uint64 a);
		uint64 callLongMethod(const char* methodName, const char* a, const char* b);

	private:
		JNIEnv* mEnv = nullptr;
		jobject mActivity = nullptr;
		jclass mActivityClass = nullptr;
	};

#endif
