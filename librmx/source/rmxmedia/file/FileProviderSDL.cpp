/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"

#if defined(PLATFORM_ANDROID)
	#include <jni.h>
	#include <android/asset_manager_jni.h>
#endif


namespace rmx
{

#if defined(PLATFORM_ANDROID)
	static AAssetManager* gAssetManager = nullptr;
#endif

	namespace
	{
		SDL_RWops* openFileWithRWops(const std::wstring& filename)
		{
			String fname = WString(filename).toUTF8();
		#if defined(PLATFORM_ANDROID)
			if (fname.startsWith("./"))
				fname.remove(0, 2);
		#endif
			return SDL_RWFromFile(*fname, "r");
		}
	}



	bool FileProviderSDL::exists(const std::wstring& path)
	{
		// Note that this works only for files, not directories
		SDL_RWops* context = openFileWithRWops(path);
		if (nullptr != context)
		{
			SDL_RWclose(context);
			return true;
		}
		return false;
	}

	bool FileProviderSDL::getFileSize(const std::wstring& filename, uint64& outFileSize)
	{
		SDL_RWops* context = openFileWithRWops(filename);
		if (nullptr != context)
		{
			const Sint64 totalSize = SDL_RWsize(context);
			SDL_RWclose(context);
			outFileSize = (uint64)std::max<Sint64>(totalSize, 0);
			return true;
		}
		return false;
	}

	bool FileProviderSDL::readFile(const std::wstring& filename, std::vector<uint8>& outData)
	{
		if (filename.back() == L'/')
			return false;

		outData.clear();
		SDL_RWops* context = openFileWithRWops(filename);
		if (nullptr == context)
			return false;

		const Sint64 totalSize = SDL_RWsize(context);
		if (totalSize == 0)
			return true;

		if (totalSize > 0)	// Otherwise size could not be determined
		{
			outData.reserve((size_t)totalSize);
		}

		const constexpr size_t BUFFER_SIZE = 0x1000;
		char data[BUFFER_SIZE];
		while (true)
		{
			const size_t bytesRead = SDL_RWread(context, data, 1, BUFFER_SIZE);
			if (bytesRead == 0)
				break;

			const size_t pos = outData.size();
			outData.resize(pos + bytesRead);
			memcpy(&outData[pos], data, bytesRead);
		}
		SDL_RWclose(context);
		return true;
	}

	InputStream* FileProviderSDL::createInputStream(const std::wstring& filename)
	{
		InputStream* inputStream = new FileInputStreamSDL(filename);
		if (!inputStream->valid())
		{
			delete inputStream;
			return nullptr;
		}
		return inputStream;
	}

	bool FileProviderSDL::listFiles(const std::wstring& path, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)
	{
	#if defined(PLATFORM_ANDROID)
		RMX_ASSERT(!recursive, "Recursive listing of files inside APK is not supported, recursion will be ignored there (called for '" << WString(path).toStdString() << "')");
		if (!path.empty())
		{
			if (nullptr == gAssetManager)
			{
				JNIEnv* env = (JNIEnv*)SDL_AndroidGetJNIEnv();
				jclass sdlActivityJavaClass = env->FindClass("org/libsdl/app/SDLActivity");

				jmethodID getContextMethodID = env->GetStaticMethodID(sdlActivityJavaClass, "getContext", "()Landroid/content/Context;");
				jobject javaContext = env->CallStaticObjectMethod(sdlActivityJavaClass, getContextMethodID);

				jmethodID getAssetsMethodID = env->GetMethodID(env->GetObjectClass(javaContext), "getAssets", "()Landroid/content/res/AssetManager;");
				jobject javaAssetManager = env->CallObjectMethod(javaContext, getAssetsMethodID);
				gAssetManager = AAssetManager_fromJava(env, javaAssetManager);
			}

			std::wstring basePath = path;
			FileIO::normalizePath(basePath, true);

			String plainPath = WString(basePath).toUTF8();
			plainPath[plainPath.length() - 1] = 0;		// Remove trailing slash

			AAssetDir* assetDir = AAssetManager_openDir(gAssetManager, *plainPath);
			RMX_ASSERT(nullptr != assetDir, "Got a null pointer for assetDir when listing files in '" << WString(path).toStdString() << "', that shouldn't happen even if the directory does not exist");
			while (true)
			{
				const char* filename = AAssetDir_getNextFileName(assetDir);
				if (nullptr == filename)
					break;

				FileIO::FileEntry& entry = vectorAdd(outFileEntries);
				entry.mFilename = String(filename).toStdWString();
				entry.mPath = basePath;
				entry.mTime = 0;
				entry.mSize = 0;
			}
			AAssetDir_close(assetDir);
		}
		// Also add the entries found by "normal" file access by the base class implementation
	#endif

		return false;
	}

	bool FileProviderSDL::listFilesByMask(const std::wstring& filemask, bool recursive, std::vector<FileIO::FileEntry>& outFileEntries)
	{
	#if defined(PLATFORM_ANDROID)
		std::wstring maskBasePath;
		std::wstring maskFileName;
		std::wstring maskFileExt;
		FileIO::splitPath(filemask, &maskBasePath, &maskFileName, &maskFileExt);

		listFiles(maskBasePath, recursive, outFileEntries);
		FileIO::filterMaskMatches(outFileEntries, maskFileName + L'.' + maskFileExt);
		return true;
	#else
		return false;
	#endif
	}

}
