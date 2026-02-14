/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


/* ----- FontSource ---------------------------------------------------------------------------------------------- */

const FontSource::GlyphInfo* FontSource::getGlyph(uint32 unicode)
{
	auto it = mGlyphMap.find(unicode);
	if (it != mGlyphMap.end())
		return &it->second;

	// Create glyph
	GlyphInfo info;
	info.mUnicode = unicode;

	const bool result = fillGlyphInfo(info);
	if (!result || info.mBitmap.empty())
		return nullptr;

	// Add to map
	it = mGlyphMap.emplace(unicode, info).first;
	return &it->second;
}



/* ----- FontSourceStd ------------------------------------------------------------------------------------------- */

#include "StdFontData.inc"

namespace rmx::stdfont
{
	static const constexpr float SIZE = 16;
	static const constexpr int WIDTH = 12;
	static const constexpr int HEIGHT = 20;
	static const constexpr int ASCENDER = 15;
	static const constexpr int LINEHEIGHT = 25;
}

FontSourceStd::FontSourceStd(float size)
{
	RMX_ASSERT(size >= 1.0f && size < 100.0f, "Invalid standard font size of " << size);
	mSize = size;
	mAscender = rmx::stdfont::ASCENDER;
	mDescender = rmx::stdfont::HEIGHT - rmx::stdfont::ASCENDER;
	mLineHeight = rmx::stdfont::LINEHEIGHT;
}

bool FontSourceStd::fillGlyphInfo(FontSource::GlyphInfo& info)
{
	// Use standard font
	const int index = clamp((int)info.mUnicode - 32, 0, 95);

	// Create bitmap & copy data
	Bitmap bmp1;
	bmp1.create(rmx::stdfont::WIDTH, rmx::stdfont::HEIGHT);
	for (int i = 0; i < bmp1.getPixelCount(); ++i)
	{
		int bits = (stdfont_data[index][i/16] >> ((i%16)*2)) % 4;
		bmp1.getData()[i] = 0x00ffffff + ((bits * 85) << 24);
	}

	if (mSize != rmx::stdfont::SIZE)
	{
		// Rescale if needed
		info.mBitmap.rescale(bmp1, roundToInt(rmx::stdfont::WIDTH * mSize / rmx::stdfont::SIZE),
								   roundToInt(rmx::stdfont::HEIGHT * mSize / rmx::stdfont::SIZE));
	}
	else
	{
		info.mBitmap.copy(bmp1);
	}

	info.mAdvance = roundToInt(rmx::stdfont::WIDTH * mSize / rmx::stdfont::SIZE);
	return true;
}



/* ----- FontSourceBitmap ------------------------------------------------------------------------------------------- */

FontSourceBitmap::FontSourceBitmap(const std::wstring& jsonFilename, bool showErrors)
{
	// Read JSON file
	Json::Value root;
	{
		std::vector<uint8> content;
		if (!FTX::FileSystem->readFile(jsonFilename, content))
		{
			RMX_CHECK(showErrors, "Failed to load bitmap font JSON file at '" << WString(jsonFilename).toStdString() << "': File not found", );
			return;
		}

		root = rmx::JsonHelper::loadFromMemory(content);
		if (root.isNull())
		{
			RMX_CHECK(showErrors, "Failed to load bitmap font JSON file at '" << WString(jsonFilename).toStdString() << "': Error loading JSON content", );
			return;
		}
	}

	rmx::JsonHelper rootHelper(root);
	rootHelper.tryReadInt("ascender", mAscender);
	rootHelper.tryReadInt("descender", mDescender);
	rootHelper.tryReadInt("lineheight", mLineHeight);
	rootHelper.tryReadInt("space", mSpaceBetweenCharacters);

	// Load bitmap
	std::wstring parentPath;
	rmx::FileIO::splitPath(jsonFilename, &parentPath, nullptr, nullptr);

	std::string textureName;
	rootHelper.tryReadString("texture", textureName);
	RMX_CHECK(!textureName.empty(), "Texture field is missing or empty in bitmap font JSON file at '" << WString(jsonFilename).toStdString() << "'", );

	Bitmap bitmap;
	if (!bitmap.load(parentPath + L"/" + String(textureName).toStdWString()))
	{
		RMX_CHECK(showErrors, "Failed to load font bitmap from '" << WString(parentPath).toStdString() << "/" << textureName << "' (referenced in '" << WString(jsonFilename).toStdString() << "')", );
		return;
	}

	Json::Value charactersJson = root["characters"];
	for (auto iterator = charactersJson.begin(); iterator != charactersJson.end(); ++iterator)
	{
		wchar_t character;
		{
			WString key;
			Json::String keyString = iterator.key().asString();
			key.fromUTF8(keyString.c_str(), keyString.length());
			character = key[0];
		}
		String value = iterator->asString();

		if (value.startsWith("redirect:"))
		{
			value.remove(0, 9);
			while (value[0] == ' ')
				value.remove(0, 1);
			mCharacterRedirects[character] = value[0];
		}
		else
		{
			std::vector<String> components;
			value.split(components, ' ');
			if (components.size() == 4)
			{
				Recti rect;
				for (size_t k = 0; k < components.size(); ++k)
				{
					rect[k] = components[k].parseInt();
				}
				mCharacterBitmaps[character].copy(bitmap, rect);
			}
		}
	}
	mLoadingSucceeded = true;
}

bool FontSourceBitmap::fillGlyphInfo(FontSource::GlyphInfo& info)
{
	auto it = mCharacterBitmaps.find(info.mUnicode);
	if (it == mCharacterBitmaps.end())
	{
		// Resolve redirect if possible
		wchar_t* redirect = mapFind(mCharacterRedirects, (wchar_t)info.mUnicode);
		if (nullptr == redirect)
			return false;

		it = mCharacterBitmaps.find(*redirect);
		if (it == mCharacterBitmaps.end())
			return false;
	}

	const Bitmap& bitmap = it->second;
	info.mBitmap = bitmap;
	info.mAdvance = bitmap.getWidth() + mSpaceBetweenCharacters;
	return true;
}
