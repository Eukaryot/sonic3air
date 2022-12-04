/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/drawing/DrawerTexture.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"


// Other platforms than Windows with Visual C++ need to the zlib library dependency into their build separately
#if defined(PLATFORM_WINDOWS) && defined(_MSC_VER)
	#pragma comment(lib, "minizip.lib")
#endif

#include "unzip.h"


namespace
{
	bool ExtractCurrentFileFromZip(unzFile zipFile, const std::wstring& outputBasePath)
	{
		// Get file info
		unz_file_info64 fileInfo;
		char localFilename[256];
		int result = unzGetCurrentFileInfo64(zipFile, &fileInfo, localFilename, sizeof(localFilename), nullptr, 0, nullptr, 0);
		if (result != UNZ_OK)
			return false;

		// For security reasons, skip files with a path starting with ".."
		// TODO: Better exclude all paths with ".." somewhere in between
		if (localFilename[0] == '.' && localFilename[1] == '.')
		{
			// That still counts as a success
			return true;
		}

		// Get the file name only, without the local path
		const char* filenameOnly = "";
		{
			const char* ptr = filenameOnly = localFilename;
			while (*ptr != 0)
			{
				if ((*ptr == '/') || (*ptr == '\\'))
					filenameOnly = ptr + 1;
				++ptr;
			}
		}

		if (filenameOnly[0] == 0)
		{
			// Create directory
			FTX::FileSystem->createDirectory(outputBasePath + String(localFilename).toStdWString());
			return true;
		}
		else
		{
			// Extract file
			result = unzOpenCurrentFile(zipFile);
			if (result == UNZ_OK)
			{
				std::vector<uint8> fileContent;
				fileContent.resize((size_t)fileInfo.uncompressed_size);

				const int readResult = unzReadCurrentFile(zipFile, &fileContent[0], (int)fileContent.capacity());
				if (readResult <= 0)
				{
					result = readResult;
				}
				else
				{
					fileContent.resize(readResult);
					FTX::FileSystem->saveFile(outputBasePath + String(localFilename).toStdWString(), fileContent);
				}

				// TODO: Setting the file date is a nice to have
				//change_file_date(write_filename, fileInfo.dosDate, fileInfo.tmu_date);
			}
			return (unzCloseCurrentFile(zipFile) == UNZ_OK) && (result == UNZ_OK);
		}
	}
}


bool FileHelper::loadPaletteBitmap(PaletteBitmap& bitmap, const std::wstring& filename, bool showError)
{
	std::vector<uint8> content;
	if (!FTX::FileSystem->readFile(filename, content))
	{
		RMX_CHECK(!showError, "Failed to load image file '" << *WString(filename).toString() << "': File not found", );
		return false;
	}

	if (!bitmap.loadBMP(content))
	{
		RMX_CHECK(!showError, "Failed to load image file '" << *WString(filename).toString() << "': Format not supported", );
		return false;
	}
	return true;
}

bool FileHelper::loadBitmap(Bitmap& bitmap, const std::wstring& filename, bool showError)
{
	std::vector<uint8> content;
	if (!FTX::FileSystem->readFile(filename, content))
	{
		RMX_CHECK(!showError, "Failed to load image file '" << *WString(filename).toString() << "': File not found", );
		return false;
	}

	// Get file type
	String format;
	WString fname = filename;
	const int pos = fname.findChar(L'.', fname.length()-1, -1);
	if (pos > 0)
	{
		format = fname.getSubString(pos+1, -1).toString();
	}

	MemInputStream stream(&content[0], (int)content.size());
	Bitmap::LoadResult loadResult;
	if (!bitmap.decode(stream, loadResult, *format))
	{
		RMX_CHECK(!showError, "Failed to load image file '" << *WString(filename).toString() << "': Format not supported", );
		return false;
	}
	return true;
}

bool FileHelper::loadTexture(DrawerTexture& texture, const std::wstring& filename, bool showError)
{
	if (!texture.isValid())
	{
		EngineMain::instance().getDrawer().createTexture(texture);
	}

	Bitmap& bitmap = texture.accessBitmap();
	if (!loadBitmap(bitmap, filename, showError))
		return false;

	texture.bitmapUpdated();
	return true;
}

bool FileHelper::loadShader(Shader& shader, const std::wstring& filename, const std::string& techname, const std::string& additionalDefines)
{
	std::vector<uint8> content;
	if (!FTX::FileSystem->readFile(filename, content))
		return false;

	if (shader.load(content, techname, additionalDefines))
	{
		RMX_LOG_INFO("Loaded shader '" << WString(filename).toStdString() << "'");
	}
	else
	{
		RMX_ERROR("Shader loading failed for '" << WString(filename).toStdString() << "':\n" << shader.getCompileLog().toStdString(), );
	}
	return true;
}

bool FileHelper::extractZipFile(const std::wstring& zipFilename, const std::wstring& outputBasePath)
{
	unzFile zipFile = unzOpen64(*WString(zipFilename).toString());
	unz_global_info64 globalInfo;
	int result = unzGetGlobalInfo64(zipFile, &globalInfo);
	if (result != UNZ_OK)
		return false;

	// Make sure the base path for output ends with a slash
	std::wstring basePath = outputBasePath;
	FTX::FileSystem->normalizePath(basePath, true);

	for (ZPOS64_T i = 0; i < globalInfo.number_entry; ++i)
	{
		if (!ExtractCurrentFileFromZip(zipFile, basePath))
		{
			// If one file extraction fails, don't continue with the rest
			return false;
		}

		if ((i + 1) < globalInfo.number_entry)
		{
			result = unzGoToNextFile(zipFile);
			if (result != UNZ_OK)
				return false;
		}
	}
	return true;
}
