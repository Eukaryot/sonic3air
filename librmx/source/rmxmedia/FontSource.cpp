/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


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

const float stdfont_size = 16;
const int stdfont_width = 12;
const int stdfont_height = 20;
const int stdfont_ascender = 15;
const int stdfont_lineheight = 25;


FontSourceStd::FontSourceStd(float size)
{
	RMX_ASSERT(size >= 1.0f && size < 100.0f, "Invalid standard font size of " << size);
	mSize = size;
	mAscender = stdfont_ascender;
	mDescender = stdfont_height - stdfont_ascender;
	mLineHeight = stdfont_lineheight;
}

bool FontSourceStd::fillGlyphInfo(FontSource::GlyphInfo& info)
{
	// Use standard font
	const int index = clamp((int)info.mUnicode - 32, 0, 95);

	// Create bitmap & copy data
	Bitmap bmp1;
	bmp1.create(stdfont_width, stdfont_height);
	for (int i = 0; i < stdfont_width * stdfont_height; ++i)
	{
		int bits = (stdfont_data[index][i/16] >> ((i%16)*2)) % 4;
		bmp1.mData[i] = 0x00ffffff + ((bits * 85) << 24);
	}

	if (mSize != stdfont_size)
	{
		// Rescale if needed
		info.mBitmap.rescale(bmp1, roundToInt(stdfont_width * mSize / stdfont_size),
								  roundToInt(stdfont_height * mSize / stdfont_size));
	}
	else
	{
		info.mBitmap.copy(bmp1);
	}

	info.mLeftIndent = 0;
	info.mTopIndent = 0;
	info.mAdvance = roundToInt(stdfont_width * mSize / stdfont_size);
	return true;
}



/* ----- FontSourceBitmap ------------------------------------------------------------------------------------------- */

FontSourceBitmap::FontSourceBitmap(const String& jsonFilename)
{
	// Read JSON file
	Json::Value root = rmx::JsonHelper::loadFile(*jsonFilename.toWString());
	RMX_CHECK(!root.isNull(), "Failed to load bitmap font JSON file at '" << *jsonFilename << "'", );

	rmx::JsonHelper rootHelper(root);
	rootHelper.tryReadInt("ascender", mAscender);
	rootHelper.tryReadInt("descender", mDescender);
	rootHelper.tryReadInt("lineheight", mLineHeight);
	rootHelper.tryReadInt("space", mSpaceBetweenCharacters);

	// Load bitmap
	std::string parentPath;
	rmx::FileIO::splitPath(*jsonFilename, &parentPath, nullptr, nullptr);

	std::string textureName;
	rootHelper.tryReadString("texture", textureName);
	RMX_CHECK(!textureName.empty(), "Texture field is missing or empty in bitmap font JSON file at '" << *jsonFilename << "'", );

	Bitmap bitmap(parentPath + "/" + textureName);
	if (bitmap.empty())
	{
		RMX_ERROR("Failed to load font bitmap from '" << parentPath << "/" << textureName << "' (referenced in '" << *jsonFilename << "')", );
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
}

bool FontSourceBitmap::fillGlyphInfo(FontSource::GlyphInfo& info)
{
	auto it = mCharacterBitmaps.find(info.mUnicode);
	if (it == mCharacterBitmaps.end())
	{
		// Resolve redirect if possible
		const auto it2 = mCharacterRedirects.find(info.mUnicode);
		if (it2 == mCharacterRedirects.end())
			return false;

		it = mCharacterBitmaps.find(it2->second);
		if (it == mCharacterBitmaps.end())
			return false;
	}

	const Bitmap& bitmap = it->second;
	info.mBitmap = bitmap;
	info.mAdvance = bitmap.mWidth + mSpaceBetweenCharacters;
	info.mLeftIndent = 0;
	info.mTopIndent = 0;

	return true;
}
