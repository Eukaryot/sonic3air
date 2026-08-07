/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"

#if defined(PLATFORM_ANDROID)

	#include "oxygen/platform/android/AndroidJNIHelper.h"


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

		inline bool isValid() const  { return nullptr != mJString; }
		inline jstring operator*() const  { return mJString; }

		JNIEnv* mEnv = nullptr;
		jstring mJString = nullptr;
	};


	struct JNIByteArrayParam
	{
		JNIByteArrayParam(JNIEnv* env, const std::vector<uint8>& data) :
			mEnv(env)
		{
			mJByteArray = mEnv->NewByteArray(data.size());
			mEnv->SetByteArrayRegion(mJByteArray, 0, data.size(), reinterpret_cast<const jbyte*>(data.data()));
		}

		~JNIByteArrayParam()
		{
			if (nullptr != mJByteArray)
				mEnv->DeleteLocalRef(mJByteArray);
		}

		inline bool isValid() const  { return nullptr != mJByteArray; }
		inline jbyteArray operator*() const  { return mJByteArray; }

		JNIEnv* mEnv = nullptr;
		jbyteArray mJByteArray = nullptr;
	};


	struct JNITypeSignatureHelper
	{
		template<typename T> static const char* getSignature()	{ return T::UNKNOWN; }
		template<> const char* getSignature<void>()				{ return "V"; }
		template<> const char* getSignature<bool>()				{ return "Z"; }
		template<> const char* getSignature<uint8>()			{ return "B"; }
		template<> const char* getSignature<int16>()			{ return "S"; }
		template<> const char* getSignature<int32>()			{ return "I"; }
		template<> const char* getSignature<jlong>()			{ return "J"; }
		template<> const char* getSignature<float>()			{ return "F"; }
		template<> const char* getSignature<double>()			{ return "D"; }
		template<> const char* getSignature<jstring>()			{ return "Ljava/lang/String;"; }
		template<> const char* getSignature<jbyteArray>()		{ return "[B"; }

		static void	collectParameterSignatures(std::string& outSignature)
		{
			// End of template arguments recursion
		}

		template <typename T, typename... Args>
		static void	collectParameterSignatures(std::string& outSignature, const T&, const Args& ...args)
		{
			outSignature += getSignature<T>();
			collectParameterSignatures(outSignature, args...);
		}

		template <typename MethodType, typename... Args>
		static std::string getMethodSignature(const Args& ...args)
		{
			std::string signature;
			signature += "(";
			collectParameterSignatures(signature, args...);
			signature += ")";
			signature += getSignature<MethodType>();
			return signature;
		}
	};


	AndroidJNIHelper::AndroidJNIHelper()
	{
		mEnv = (JNIEnv*)SDL_AndroidGetJNIEnv();
		mActivity = (jobject)SDL_AndroidGetActivity();
		mActivityClass = mEnv->GetObjectClass(mActivity);
	}

	AndroidJNIHelper::~AndroidJNIHelper()
	{
		mEnv->DeleteLocalRef(mActivity);
		mEnv->DeleteLocalRef(mActivityClass);
	}

	void AndroidJNIHelper::callVoidMethod(const char* methodName)
	{
		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<void>();
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		mEnv->CallVoidMethod(mActivity, methodId);
	}

	void AndroidJNIHelper::callVoidMethod(const char* methodName, const char* a, const std::vector<uint8>& b)
	{
		JNIStringParam stringA(mEnv, a);
		JNIByteArrayParam byteArrayB(mEnv, b);
		if (!stringA.isValid() || !byteArrayB.isValid())
			return;

		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<void>(*stringA, *byteArrayB);
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		mEnv->CallVoidMethod(mActivity, methodId, *stringA, *byteArrayB);
	}

	bool AndroidJNIHelper::callBoolMethod(const char* methodName)
	{
		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<bool>();
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		jboolean result = mEnv->CallBooleanMethod(mActivity, methodId);
		return (result != 0);
	}

	bool AndroidJNIHelper::callBoolMethod(const char* methodName, uint64 a)
	{
		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<bool>((jlong)a);
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		return (bool)mEnv->CallBooleanMethod(mActivity, methodId, (jlong)a);
	}

	int AndroidJNIHelper::callIntMethod(const char* methodName, uint64 a)
	{
		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<int32>((jlong)a);
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		return mEnv->CallIntMethod(mActivity, methodId, (jlong)a);
	}

	uint64 AndroidJNIHelper::callLongMethod(const char* methodName, uint64 a)
	{
		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<jlong>((jlong)a);
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		return (uint64)mEnv->CallLongMethod(mActivity, methodId, (jlong)a);
	}

	uint64 AndroidJNIHelper::callLongMethod(const char* methodName, const char* a, const char* b)
	{
		JNIStringParam stringA(mEnv, a);
		JNIStringParam stringB(mEnv, b);
		if (!stringA.isValid() || !stringB.isValid())
			return 0;

		static const std::string signature = JNITypeSignatureHelper::getMethodSignature<jlong>(*stringA, *stringB);
		jmethodID methodId = mEnv->GetMethodID(mActivityClass, methodName, signature.c_str());
		return (uint64)mEnv->CallLongMethod(mActivity, methodId, *stringA, *stringB);
	}

#endif
