/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/helper/Utils.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"
#include <rmxmedia.h>


namespace utils
{

	bool startsWith(const std::wstring& fullString, const std::wstring& prefix)
	{
		if (fullString.length() < prefix.length())
			return false;
		if (memcmp((void*)&fullString[0], (void*)&prefix[0], prefix.length() * sizeof(wchar_t)) != 0)
			return false;
		return true;
	}

	void splitTextIntoLines(std::vector<std::string>& outLines, const std::string& text, Font& font, int maxLineWidth)
	{
		outLines.clear();
		std::string line;
		size_t position = 0;
		while (position < text.length())
		{
			const size_t start = position;
			while (position < text.length() && text[position] != ' ' && text[position] != '\n')
				++position;

			if (position > start)	// Ignore multiple spaces
			{
				const std::string word = text.substr(start, position - start);
				if (line.empty())
				{
					line = word;
				}
				else
				{
					const std::string linePlusWord = line + ' ' + word;
					if (font.getWidth(linePlusWord.c_str()) <= maxLineWidth)
					{
						line = linePlusWord;
					}
					else
					{
						// New line needed
						outLines.emplace_back(line);
						line = word;
					}
				}
			}

			if (text[position] == '\n')
			{
				// New line needed
				outLines.emplace_back(line);
				line.clear();
			}

			++position;
		}

		if (!line.empty())
		{
			outLines.emplace_back(line);
		}
	}

	void shortenTextToFit(std::string& text, Font& font, int maxLineWidth)
	{
		// Any change needed at all?
		if (font.getWidth(text) <= maxLineWidth)
			return;

		if (text.length() < 3)
			return;		// It's no use making a change here...

		size_t ellipsisPosition = text.length() - 1;
		text.erase(ellipsisPosition, 2);	// Take away at leat two characters; taking only one would be ridiculous (even if it would suffice already)
		text += "...";

		while (ellipsisPosition > 3)
		{
			if (font.getWidth(text) <= maxLineWidth)
				return;

			--ellipsisPosition;
			text[ellipsisPosition] = '.';
			text.pop_back();
		}
	}

	uint32 getVersionNumberFromString(const std::string& versionString)
	{
		// Version string is expected to use the following format: "XX.XX.XX" or "XX.XX.XX.X" with X being decimal digits
		if (versionString.length() < 8)
			return 0;

		// If there's a 'v' at the start, skip that one
		size_t pos = 0;
		if (versionString[0] == 'v')
			++pos;

		uint32 output = 0;
		uint8 currentNumber = 0;
		int currentNumDigits = 0;
		int shiftOffset = 24;
		for (; pos < versionString.length(); ++pos)
		{
			if (versionString[pos] == '.')
			{
				if (currentNumDigits == 0)
					return 0;
				if (shiftOffset <= 0)
					return 0;
				output |= ((uint32)currentNumber << shiftOffset);
				shiftOffset -= 8;
				currentNumber = 0;
				currentNumDigits = 0;
			}
			else
			{
				const int digit = (versionString[pos] - '0');
				if (digit < 0 || digit > 9)
					return 0;
				if (currentNumDigits >= 2)
					return 0;
				currentNumber = (currentNumber << 4) + digit;
				++currentNumDigits;
			}
		}

		if (currentNumDigits == 0)
			return 0;
		if (shiftOffset > 8)	// Both 0 and 8 are allowed
			return 0;
		output |= ((uint32)currentNumber << shiftOffset);

		return output;
	}

	std::string getVersionStringFromNumber(uint32 versionNumber)
	{
		return String(0, "%02x.%02x.%02x.%x", (versionNumber >> 24) & 0xff, (versionNumber >> 16) & 0xff, (versionNumber >> 8) & 0xff, versionNumber & 0xff).toStdString();
	}

	void buildSpriteAtlas(const std::wstring& outputFilename, const std::wstring& imagesFileMask)
	{
		FileCrawler fc;
		fc.addFiles(imagesFileMask);
		if (fc.size() == 0)
			return;

		// Load sprites and build atlas
		std::vector<Bitmap> sprites;
		sprites.reserve(fc.size());

		SpriteAtlasBase spriteAtlas;
		for (size_t fileIndex = 0; fileIndex < fc.size(); ++fileIndex)
		{
			Bitmap& bitmap = vectorAdd(sprites);
			if (bitmap.load(fc[fileIndex]->mPath + fc[fileIndex]->mFilename))
			{
				spriteAtlas.add((uint32)sprites.size(), Vec2i(bitmap.mWidth, bitmap.mHeight));
			}
			else
			{
				sprites.pop_back();
			}
		}

		// Build page content
		std::vector<Bitmap> pages;
		pages.resize((size_t)spriteAtlas.getNumPages());

		for (int pageIndex = 0; pageIndex < spriteAtlas.getNumPages(); ++pageIndex)
		{
			SpriteAtlasBase::Page page;
			if (spriteAtlas.getPage(pageIndex, page))
			{
				pages[pageIndex].create(page.mPageSize.x, page.mPageSize.y);
			}
		}

		for (size_t spriteIndex = 0; spriteIndex < sprites.size(); ++spriteIndex)
		{
			SpriteAtlasBase::Sprite sprite;
			if (spriteAtlas.getSprite((uint32)spriteIndex, sprite))
			{
				pages[sprite.mPage.mIndex].insert(sprite.mRect.x, sprite.mRect.y, sprites[spriteIndex]);
			}
		}

		// Save pages
		if (spriteAtlas.getNumPages() == 1)
		{
			pages[0].save(outputFilename);
		}
		else
		{
			for (int pageIndex = 0; pageIndex < spriteAtlas.getNumPages(); ++pageIndex)
			{
				// TODO: Put the number between file name and file ending
				pages[pageIndex].save(WString(0, L"%s-%d", outputFilename.c_str(), pageIndex));
			}
		}
	}

	void buildSpriteAtlas2(const std::wstring& outputFilename, const std::wstring& imagesFileMask)
	{
		FileCrawler fc;
		fc.addFiles(imagesFileMask);
		if (fc.size() == 0)
			return;

		std::vector<PaletteBitmap> bitmaps;
		bitmaps.reserve(fc.size());
		Color palette[0x100];

		std::vector<uint8> buffer;
		Vec2i imgSize;
		for (size_t i = 0; i < fc.size(); ++i)
		{
			buffer.clear();
			if (!fc.loadFile(i, buffer))
				continue;

			bitmaps.emplace_back();
			PaletteBitmap& bitmap = bitmaps.back();
			if ((bitmaps.size() == 1) ? bitmap.loadBMP(buffer, palette) : bitmap.loadBMP(buffer))
			{
				imgSize.x = std::max<int>(imgSize.x, bitmap.mWidth);
				imgSize.y = std::max<int>(imgSize.y, bitmap.mHeight);
			}
			else
			{
				bitmaps.pop_back();
			}
		}

		PaletteBitmap output;
		output.create(imgSize.x * 16, imgSize.y * (((int)bitmaps.size() + 15) / 16));
		output.clear(0xff);
		palette[0xff] = Color(0.15f, 0.15f, 0.15f);
		for (size_t i = 0; i < bitmaps.size(); ++i)
		{
			output.copyRect(bitmaps[i], Recti(0, 0, bitmaps[i].mWidth, bitmaps[i].mHeight), Vec2i(imgSize.x * ((int)i % 16) + (imgSize.x - bitmaps[i].mWidth) / 2, imgSize.y * ((int)i / 16) + (imgSize.y - bitmaps[i].mHeight) / 2));
		}

		buffer.clear();
		if (output.saveBMP(buffer, palette))
		{
			FTX::FileSystem->saveFile(outputFilename, (uint8*)& buffer[0], buffer.size());
		}
	}
}
