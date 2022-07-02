/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


Font::CodecList Font::mCodecs;


int FontKey::compare(const FontKey& other) const
{
	if (mProcessors.size() != other.mProcessors.size())
		return (mProcessors.size() < other.mProcessors.size() ? -1 : 1);
	for (size_t k = 0; k < mProcessors.size(); ++k)
		COMPARE_CASCADE(::compare(mProcessors[k].get(), other.mProcessors[k].get()));
	COMPARE_CASCADE(FontSourceKey::compare(other));
	return 0;
}


FontSource* FontSourceStdFactory::construct(const FontSourceKey& key)
{
	if (key.mName.empty() && key.mSize > 0.0f)
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
}

Font::Font(const String& filename, float size)
{
	loadFromFile(filename, size);
}

Font::Font(float size) :
	Font("", size)
{
}

Font::~Font()
{
	delete mFontSource;
}

bool Font::loadFromFile(const String& name, float size)
{
	mKey.mName = name;
	mKey.mSize = size;
	invalidateFontSource();
	return (nullptr != getFontSource());
}

void Font::setSize(float size)
{
	mKey.mSize = size;
	invalidateFontSource();
}

void Font::clearFontProcessors()
{
	mKey.mProcessors.clear();
	invalidateFontSource();
}

void Font::addFontProcessor(const std::shared_ptr<FontProcessor>& processor)
{
	mKey.mProcessors.emplace_back(processor);
	invalidateFontSource();
}

int Font::getWidth(const StringReader& text)
{
	return getWidth(text, 0, (int)text.mLength);
}

int Font::getWidth(const StringReader& text, int pos, int len)
{
	FontSource* fontSource = getFontSource();
	if (nullptr == fontSource)
		return 0;

	if (pos < 0 || pos >= (int)text.mLength)
		return 0;
	if (len < 0 || pos+len > (int)text.mLength)
		len = (int)text.mLength - pos;

	int width = 0;
	for (int i = 0; i < len; ++i)
	{
		const uint32 ch = text[pos+i];
		const FontSource::GlyphInfo* info = fontSource->getGlyph(ch);
		if (nullptr == info)
			continue;

		width += info->mAdvance;
	}

	width += roundToInt((len - 1) * mAdvance);
	return width;
}

int Font::getHeight()
{
	FontSource* fontSource = getFontSource();
	return (nullptr == fontSource) ? 0 : fontSource->getHeight();
}

int Font::getLineHeight()
{
	FontSource* fontSource = getFontSource();
	return (nullptr == fontSource) ? 0 : fontSource->getLineHeight();
}

Vec2f Font::alignText(const Rectf& rect, const StringReader& text, int alignment)
{
	Vec2f result(rect.x, rect.y);
	FontSource* fontSource = getFontSource();
	if (nullptr == fontSource)
		return result;

	if (alignment >= 2 && alignment <= 9)
	{
		--alignment;
		int align_x = alignment % 3;
		int align_y = alignment / 3;

		if (align_x > 0)
			result.x += ((int)rect.width - getWidth(text)) * align_x / 2;

		if (align_y > 0)
			result.y += ((int)rect.height - fontSource->getHeight()) * align_y / 2;
	}

	return result;
}

void Font::wordWrapText(std::vector<std::wstring>& output, int maxLineWidth, const StringReader& text, int spacing)
{
	FontSource* fontSource = getFontSource();
	if (nullptr == fontSource)
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
			const FontSource::GlyphInfo* info = fontSource->getGlyph(ch);
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
				whitespaceBeforeWord.mWidth += info->mAdvance + spacing;
			}
			else
			{
				// Check if line break is needed now
				const int newCombinedLineWidth = currentLine.mWidth + whitespaceBeforeWord.mWidth + currentWord.mWidth + info->mAdvance;
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
				currentWord.mWidth += info->mAdvance + spacing;
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
	FontSource* fontSource = getFontSource();
	if (nullptr == fontSource)
		return;

	output.resize(text.mLength);

	for (size_t k = 0; k < text.mLength; ++k)
	{
		const uint32 ch = text[k];
		output[k].unicode = ch;
		output[k].bitmap = nullptr;

		const FontSource::GlyphInfo* info = fontSource->getGlyph(ch);
		if (nullptr == info)
			continue;

		output[k].bitmap = &info->mBitmap;
		output[k].pos.x = pos.x + (float)info->mLeftIndent;
		output[k].pos.y = pos.y + (float)info->mTopIndent;

		pos.x += info->mAdvance + spacing;
	}
}

void Font::applyToTypeInfos(std::vector<ExtendedTypeInfo>& outTypeInfos, const std::vector<TypeInfo>& inTypeInfos)
{
	outTypeInfos.reserve(inTypeInfos.size());
	for (size_t i = 0; i < inTypeInfos.size(); ++i)
	{
		const TypeInfo& typeInfo = inTypeInfos[i];
		if (nullptr == typeInfo.bitmap)
			continue;

		CharacterInfo& characterInfo = applyEffects(typeInfo);

		ExtendedTypeInfo& extendedTypeInfo = vectorAdd(outTypeInfos);
		extendedTypeInfo.mCharacter = typeInfo.unicode;
		extendedTypeInfo.mBitmap = &characterInfo.mCachedBitmap;
		extendedTypeInfo.mDrawPosition = Vec2i(typeInfo.pos) - Vec2i(characterInfo.mBorderLeft, characterInfo.mBorderTop);
	}
}

Font::CharacterInfo& Font::applyEffects(const TypeInfo& typeInfo)
{
	const uint32 character = typeInfo.unicode;
	CharacterInfo& characterInfo = mCharacterMap[character];
	if (characterInfo.mCachedBitmap.empty())
	{
		FontProcessingData fontProcessingData;
		fontProcessingData.mBitmap = *typeInfo.bitmap;

		// Run font processors
		for (const std::shared_ptr<FontProcessor>& processor : mKey.mProcessors)
		{
			processor->process(fontProcessingData);
		}

		characterInfo.mBorderLeft = fontProcessingData.mBorderLeft;
		characterInfo.mBorderRight = fontProcessingData.mBorderRight;
		characterInfo.mBorderTop = fontProcessingData.mBorderTop;
		characterInfo.mBorderBottom = fontProcessingData.mBorderBottom;
		characterInfo.mCachedBitmap.swap(fontProcessingData.mBitmap);
	}
	return characterInfo;
}

void Font::printBitmap(Bitmap& outBitmap, Vec2i& outDrawPosition, const Recti& drawRect, const StringReader& text, int alignment, int spacing)
{
	Recti innerRect;
	printBitmap(outBitmap, innerRect, text, spacing);
	outDrawPosition = applyAlignment(drawRect, innerRect, alignment);
}

void Font::printBitmap(Bitmap& outBitmap, Recti& outInnerRect, const StringReader& text, int spacing)
{
	// Render text into a bitmap
	FontSource* fontSource = getFontSource();
	if (nullptr == fontSource || text.mLength == 0)
	{
		outBitmap.clear();
		return;
	}

	std::vector<TypeInfo> typeInfos;
	std::vector<ExtendedTypeInfo> extendedTypeInfos;
	getTypeInfos(typeInfos, Vec2f(0.0f, 0.0f), text, spacing);

	applyToTypeInfos(extendedTypeInfos, typeInfos);
	if (extendedTypeInfos.empty())
	{
		outBitmap.clear();
		return;
	}

	// Get bounds
	Vec2i boundsMin(+10000, +10000);
	Vec2i boundsMax(-10000, -10000);
	for (const ExtendedTypeInfo& extendedTypeInfo : extendedTypeInfos)
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

	for (const ExtendedTypeInfo& extendedTypeInfo : extendedTypeInfos)
	{
		outBitmap.insertBlend(extendedTypeInfo.mDrawPosition.x - boundsMin.x, extendedTypeInfo.mDrawPosition.y - boundsMin.y, *extendedTypeInfo.mBitmap);
	}

	// Write inner rect
	outInnerRect.x = -boundsMin.x;
	outInnerRect.y = -boundsMin.y;
	outInnerRect.width = (int)(text.mLength - 1) * spacing;
	for (size_t i = 0; i < text.mLength; ++i)
	{
		const FontSource::GlyphInfo* info = fontSource->getGlyph(text[i]);
		if (nullptr != info)
		{
			outInnerRect.width += info->mAdvance;
		}
	}
	outInnerRect.height = fontSource->getHeight();
}

Vec2i Font::applyAlignment(const Recti& drawRect, const Recti& innerRect, int alignment)
{
	// Apply alignment
	Vec2i outPosition = drawRect.getPos() - innerRect.getPos();
	const int alignX = (alignment - 1) % 3;
	const int alignY = (alignment - 1) / 3;
	if (alignX > 0)
	{
		if (alignX == 1)
		{
			outPosition.x += (drawRect.width - innerRect.width) / 2;
		}
		else
		{
			outPosition.x += drawRect.width - innerRect.width;
		}
	}
	if (alignY > 0)
	{
		if (alignY == 1)
		{
			outPosition.y += (drawRect.height - innerRect.height) / 2;
		}
		else
		{
			outPosition.y += drawRect.height - innerRect.height;
		}
	}
	return outPosition;
}

void Font::invalidateFontSource()
{
	SAFE_DELETE(mFontSource);
	mFontSourceDirty = true;
	mCharacterMap.clear();
}

FontSource* Font::getFontSource()
{
	if (mFontSourceDirty)
	{
		mFontSourceDirty = false;
		RMX_ASSERT(nullptr == mFontSource, "Font source is expected to be a null pointer");
		for (IFontSourceFactory* factory : Font::mCodecs.mList)
		{
			mFontSource = factory->construct(mKey);
			if (nullptr != mFontSource)
				break;
		}
	}
	return mFontSource;
}
