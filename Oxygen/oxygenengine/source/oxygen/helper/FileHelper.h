/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class PaletteBitmap;
class DrawerTexture;
class Shader;


class FileHelper
{
public:
	static bool loadPaletteBitmap(PaletteBitmap& bitmap, const std::wstring& filename, bool showError = true);
	static bool loadBitmap(Bitmap& bitmap, const std::wstring& filename, bool showError = true);
	static bool loadTexture(DrawerTexture& texture, const std::wstring& filename, bool showError = true);

#ifdef RMX_WITH_OPENGL_SUPPORT
	static bool loadShader(Shader& shader, const std::wstring& filename, const std::string& techname = "", const std::string& additionalDefines = "");
#endif

	static bool extractZipFile(const std::wstring& zipFilename, const std::wstring& outputBasePath);
};
