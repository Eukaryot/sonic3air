/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


Font::CodecList Font::mCodecs;

FontSource* FontSourceStdFactory::construct(const FontSourceKey& key)
{
	if (key.mName.empty())
	{
		return new FontSourceStd(key.mSize);
	}
	return nullptr;
}

FontSource* FontSourceBitmapFactory::construct(const FontSourceKey& key)
{
	if (key.mName.endsWith(".json"))
	{
		return new FontSourceBitmap(key.mName);
	}
	return nullptr;
}


Font::Font()
{
	mAdvance = 0.0f;
}

Font::Font(const String& filename, float size)
{
	mAdvance = 0.0f;
	load(filename, size);
}

Font::Font(float size)
{
	mAdvance = 0.0f;
	load(nullptr, size);
}

Font::~Font()
{
	delete mFontSource;
}

void Font::load(const String& name, float size)
{
	mKey.mName = name;
	mKey.mSize = size;
	rebuildFontSource();
}

void Font::setSize(float size)
{
	mKey.mSize = size;
	rebuildFontSource();
}

void Font::setShadow(bool enable, const Vec2f offset, float blur, float alpha)
{
	mKey.mShadowEnabled = enable;
	mKey.mShadowOffset = offset;
	mKey.mShadowBlur = blur;
	mKey.mShadowAlpha = alpha;
	rebuildFontSource();
}

void Font::addFontProcessor(FontProcessor& processor)
{
	mKey.mProcessor = &processor;
	rebuildFontSource();
}

int Font::getWidth(const StringReader& text)
{
	return getWidth(text, 0, (int)text.mLength);
}

int Font::getWidth(const StringReader& text, int pos, int len)
{
	if (nullptr == mFontSource)
		return 0;

	if (pos < 0 || pos >= (int)text.mLength)
		return 0;
	if (len < 0 || pos+len > (int)text.mLength)
		len = (int)text.mLength - pos;

	int width = 0;
	for (int i = 0; i < len; ++i)
	{
		const uint32 ch = text[pos+i];
		const FontSource::GlyphInfo* info = mFontSource->getGlyph(ch);
		if (nullptr == info)
			continue;

		width += info->advance;
	}

	width += roundToInt((len - 1) * mAdvance);
	return width;
}

int Font::getHeight()
{
	return (nullptr == mFontSource) ? 0 : mFontSource->getHeight();
}

int Font::getLineHeight()
{
	return (nullptr == mFontSource) ? 0 : mFontSource->getLineHeight();
}

Vec2f Font::alignText(const Rectf& rect, const StringReader& text, int alignment)
{
	Vec2f result(rect.x, rect.y);
	if (nullptr == mFontSource)
		return result;

	if (alignment >= 2 && alignment <= 9)
	{
		--alignment;
		int align_x = alignment % 3;
		int align_y = alignment / 3;

		if (align_x > 0)
			result.x += ((int)rect.width - getWidth(text)) * align_x / 2;

		if (align_y > 0)
			result.y += ((int)rect.height - mFontSource->getHeight()) * align_y / 2;
	}

	return result;
}

void Font::wordWrapText(std::vector<std::wstring>& output, int maxLineWidth, const StringReader& text, int spacing)
{
	if (nullptr == mFontSource)
		return;

	struct TextAndWidth
	{
		std::wstring mText;
		int mWidth = 0;

		bool empty() const							{ return mText.empty(); }
		void clear()								{ mText = L""; mWidth = 0; }
		void operator+=(const TextAndWidth& other)	{ mText += other.mText; mWidth += other.mWidth; }
		void operator<<(TextAndWidth& other)		{ *this += other; other.clear(); }

		void pushAsLineInto(std::vector<std::wstring>& output)
		{
			output.emplace_back();
			mText.swap(output.back());
			clear();
		}
	};

	output.clear();
	TextAndWidth currentLine;
	TextAndWidth currentWord;
	TextAndWidth whitespaceBeforeWord;

	for (size_t k = 0; k < text.mLength; ++k)
	{
		const wchar_t ch = text[k];
		if (ch == L'\n')
		{
			// Enforced line break
			if (currentWord.empty())
			{
				whitespaceBeforeWord.clear();
			}
			else
			{
				currentLine << whitespaceBeforeWord;
				currentLine << currentWord;
			}
			currentLine.pushAsLineInto(output);
		}
		else
		{
			const FontSource::GlyphInfo* info = mFontSource->getGlyph(ch);
			if (nullptr == info)
				continue;

			if (ch == L' ' || ch == L'\t')
			{
				// Whitespace starts a new word
				if (!currentWord.empty())
				{
					currentLine << whitespaceBeforeWord;
					currentLine << currentWord;
				}

				whitespaceBeforeWord.mText += ch;
				whitespaceBeforeWord.mWidth += info->advance + spacing;
			}
			else
			{
				// Check if line break is needed now
				const int newCombinedLineWidth = currentLine.mWidth + whitespaceBeforeWord.mWidth + currentWord.mWidth + info->advance;
				if (newCombinedLineWidth > maxLineWidth)
				{
					if (currentLine.empty())
					{
						// Special case: There's not enough space for this word in the whole line, so break in between
						currentLine << whitespaceBeforeWord;
						currentLine << currentWord;
					}
					else
					{
						whitespaceBeforeWord.clear();
					}

					currentLine.pushAsLineInto(output);
				}

				currentWord.mText += ch;
				currentWord.mWidth += info->advance + spacing;
			}
		}
	}

	// Don't forget to output the last line as well
	if (!currentWord.empty())
	{
		currentLine << whitespaceBeforeWord;
		currentLine << currentWord;
	}
	if (!currentLine.empty())
	{
		currentLine.pushAsLineInto(output);
	}
}

void Font::getTypeInfos(std::vector<TypeInfo>& output, Vec2f pos, const StringReader& text, int spacing)
{
	if (nullptr == mFontSource)
		return;

	output.resize(text.mLength);

	for (size_t k = 0; k < text.mLength; ++k)
	{
		const uint32 ch = text[k];
		output[k].unicode = ch;
		output[k].bitmap = nullptr;

		const FontSource::GlyphInfo* info = mFontSource->getGlyph(ch);
		if (nullptr == info)
			continue;

		output[k].bitmap = &info->bitmap;
		output[k].pos.x = pos.x + (float)info->leftIndent;
		output[k].pos.y = pos.y + (float)info->topIndent;

		pos.x += info->advance + spacing;
	}
}

void Font::print(int x, int y, const StringReader& text, int alignment)
{
	FTX::Painter->print(*this, Rectf((float)x, (float)y, 0.0f, 0.0f), text, alignment);
}

void Font::print(int x, int y, int w, int h, const StringReader& text, int alignment)
{
	FTX::Painter->print(*this, Rectf((float)x, (float)y, (float)w, (float)h), text, alignment);
}

void Font::print(const Rectf& rect, const StringReader& text, int alignment)
{
	FTX::Painter->print(*this, rect, text, alignment);
}

void Font::printBitmap(Bitmap& outBitmap, Vec2i& outDrawPosition, const Recti& rect, const StringReader& text, int alignment, int spacing)
{
	// Render text into a bitmap
	if (nullptr == mFontSource || text.mLength == 0)
	{
		outBitmap.clear();
		return;
	}

	std::vector<Font::TypeInfo> typeInfos;
	std::vector<FontOutput::ExtendedTypeInfo> extendedTypeInfos;
	getTypeInfos(typeInfos, Vec2f(0.0f, 0.0f), text, spacing);

	FontOutput* output = FTX::Painter->getFontOutput(*this);
	output->applyToTypeInfos(extendedTypeInfos, typeInfos);
	if (extendedTypeInfos.empty())
	{
		outBitmap.clear();
		return;
	}

	// Get bounds
	Vec2i boundsMin(+10000, +10000);
	Vec2i boundsMax(-10000, -10000);
	for (const FontOutput::ExtendedTypeInfo& extendedTypeInfo : extendedTypeInfos)
	{
		const Vec2i minPos = extendedTypeInfo.mDrawPosition;
		const Vec2i maxPos = minPos + extendedTypeInfo.mBitmap->getSize();
		boundsMin.x = std::min(boundsMin.x, minPos.x);
		boundsMin.y = std::min(boundsMin.y, minPos.y);
		boundsMax.x = std::max(boundsMax.x, maxPos.x);
		boundsMax.y = std::max(boundsMax.y, maxPos.y);
	}

	// Setup and fill bitmap
	outBitmap.create(boundsMax.x - boundsMin.x, boundsMax.y - boundsMin.y, 0);

	for (const FontOutput::ExtendedTypeInfo& extendedTypeInfo : extendedTypeInfos)
	{
		outBitmap.insertBlend(extendedTypeInfo.mDrawPosition.x - boundsMin.x, extendedTypeInfo.mDrawPosition.y - boundsMin.y, *extendedTypeInfo.mBitmap);
	}

	// Apply alignment
	outDrawPosition = Vec2i(rect.x, rect.y) + boundsMin;
	const int alignX = (alignment - 1) % 3;
	const int alignY = (alignment - 1) / 3;
	if (alignX > 0)
	{
		int width = 0;
		for (size_t i = 0; i < text.mLength; ++i)
		{
			const FontSource::GlyphInfo* info = mFontSource->getGlyph(text[i]);
			if (nullptr != info)
			{
				width += info->advance;
			}
		}

		if (alignX == 1)
		{
			outDrawPosition.x += (rect.width - width) / 2;
		}
		else
		{
			outDrawPosition.x += rect.width - width;
		}
	}
	if (alignY > 0)
	{
		const int height = mFontSource->getHeight();
		if (alignY == 1)
		{
			outDrawPosition.y += (rect.height - height) / 2;
		}
		else
		{
			outDrawPosition.y += rect.height - height;
		}
	}
}

void Font::rebuildFontSource()
{
	delete mFontSource;
	mFontSource = nullptr;

	for (IFontSourceFactory* factory : Font::mCodecs.mList)
	{
		mFontSource = factory->construct(mKey);
		if (nullptr != mFontSource)
			return;
	}
}
