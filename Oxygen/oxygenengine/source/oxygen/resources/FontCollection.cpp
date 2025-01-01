/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/resources/FontCollection.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/rendering/RenderResources.h"

#include "lemon/compiler/parser/Parser.h"
#include "lemon/compiler/parser/ParserTokens.h"


namespace
{
	struct FontKeyParameter
	{
		lemon::IdentifierParserToken* mIdentifier = nullptr;
		std::vector<lemon::ConstantParserToken*> mArguments;

		template<typename T>
		inline T getIntArgument(size_t index, T defaultValue) const
		{
			if (index >= mArguments.size())
				return defaultValue;
			switch (mArguments[index]->mBaseType)
			{
				case lemon::BaseType::FLOAT:	return (T)mArguments[index]->mValue.get<float>();
				case lemon::BaseType::DOUBLE:	return (T)mArguments[index]->mValue.get<double>();
				default:						return mArguments[index]->mValue.get<T>();
			}
		}

		inline float getFloatArgument(size_t index, float defaultValue) const
		{
			if (index >= mArguments.size())
				return defaultValue;
			switch (mArguments[index]->mBaseType)
			{
				case lemon::BaseType::FLOAT:	return mArguments[index]->mValue.get<float>();
				case lemon::BaseType::DOUBLE:	return (float)mArguments[index]->mValue.get<double>();
				default:						return (float)mArguments[index]->mValue.get<int64>();
			}
		}
	};


	bool isOperator(lemon::ParserToken& token, lemon::Operator op)
	{
		return (token.isA<lemon::OperatorParserToken>() && token.as<lemon::OperatorParserToken>().mOperator == op);
	}

	void splitIntoParameters(lemon::ParserTokenList& tokenList, std::vector<std::pair<size_t, size_t>>& outTokensRangePerParameter)
	{
		// Split by colon operators
		size_t startIndex = 0;
		for (size_t index = 0; index < tokenList.size(); ++index)
		{
			if (isOperator(tokenList[index], lemon::Operator::COLON))
			{
				if (startIndex < index)
				{
					outTokensRangePerParameter.emplace_back(startIndex, index - startIndex);
				}
				startIndex = index + 1;
			}
		}
		if (startIndex < tokenList.size())
		{
			outTokensRangePerParameter.emplace_back(startIndex, tokenList.size() - startIndex);
		}
	}

	void collectFontkeyParameters(lemon::ParserTokenList& tokenList, std::string_view fontKey, std::vector<FontKeyParameter>& outParameters)
	{
		std::vector<std::pair<size_t, size_t>> tokensRangePerParameter;
		splitIntoParameters(tokenList, tokensRangePerParameter);

		outParameters.reserve(tokensRangePerParameter.size());

		for (const auto [startIndex, length] : tokensRangePerParameter)
		{
			if (!tokenList[startIndex].isA<lemon::IdentifierParserToken>())
				continue;

			FontKeyParameter& param = vectorAdd(outParameters);
			param.mIdentifier = &tokenList[startIndex].as<lemon::IdentifierParserToken>();

			if (length == 1)
				continue;

			const bool hasArgumentsInParentheses = (length >= 3 && isOperator(tokenList[startIndex + 1], lemon::Operator::PARENTHESIS_LEFT) && isOperator(tokenList[startIndex + length - 1], lemon::Operator::PARENTHESIS_RIGHT));
			RMX_CHECK(hasArgumentsInParentheses, "Syntax error in font key '" << fontKey << "'", break);

			const size_t firstIndex = startIndex + 2;
			const size_t lastIndex = startIndex + length - 2;
			for (size_t index = firstIndex; index <= lastIndex; ++index)
			{
				// Expecting a number constant here
				RMX_CHECK(tokenList[index].isA<lemon::ConstantParserToken>(), "Syntax error in font key '" << fontKey << "'", break);
				param.mArguments.push_back(&tokenList[index].as<lemon::ConstantParserToken>());
				++index;

				// And then either a comma, or the end
				RMX_CHECK(index > lastIndex || isOperator(tokenList[index], lemon::Operator::COMMA_SEPARATOR), "Syntax error in font key '" << fontKey << "'", break);
			}
		}
	}

	void parseFontKey(std::string_view fontKey, std::string_view& outBaseFontKey, std::vector<std::shared_ptr<FontProcessor>>& outFontProcessors)
	{
		const size_t colonPosition = fontKey.find(':');
		if (colonPosition == std::string_view::npos)
		{
			outBaseFontKey = fontKey;
			return;
		}

		outBaseFontKey = fontKey.substr(0, colonPosition);

		// Use lemonscript's parser for the parameters
		lemon::ParserTokenList tokenList;
		const std::string_view parametersString = fontKey.substr(colonPosition + 1);
		lemon::Parser().splitLineIntoTokens(parametersString, 0, tokenList);

		// Collect parameters contents
		std::vector<FontKeyParameter> parameters;
		collectFontkeyParameters(tokenList, fontKey, parameters);

		// Build font processors from parameters
		for (const FontKeyParameter& param : parameters)
		{
			if (param.mIdentifier->mName == "shadow")
			{
				const Vec2i shadowOffset(param.getIntArgument<int8>(0, 1), param.getIntArgument<int8>(1, 1));
				const float shadowBlur = param.getFloatArgument(2, 0.0f);
				const float shadowAlpha = param.getFloatArgument(3, 1.0f);
				outFontProcessors.emplace_back(std::make_shared<ShadowFontProcessor>(shadowOffset, shadowBlur, shadowAlpha));
			}
			else if (param.mIdentifier->mName == "outline")
			{
				const Color outlineColor = Color::fromRGBA32(param.getIntArgument<uint32>(0, 0x000000ff));
				const int range = param.getIntArgument<int8>(1, 1);
				const bool rectangularOutline = (param.getIntArgument<bool>(2, false) != 0);
				outFontProcessors.emplace_back(std::make_shared<OutlineFontProcessor>(outlineColor, range, rectangularOutline));
			}
			else if (param.mIdentifier->mName == "gradient")
			{
				outFontProcessors.emplace_back(std::make_shared<GradientFontProcessor>());
			}
		}
	}
}


FontCollection::~FontCollection()
{
	clear();
}

Font* FontCollection::getFontByKey(uint64 keyHash)
{
	// Try to find in map
	Font** font = mapFind(mFontsByKeyHash, keyHash);
	return (nullptr != font) ? *font : nullptr;
}

Font* FontCollection::createFontByKey(std::string_view key)
{
	// First check if the font exists already
	const uint64 keyHash = rmx::getMurmur2_64(key);
	{
		Font* font = getFontByKey(keyHash);
		if (nullptr != font)
			return font;
	}

	// Split font key
	std::string_view collectedFontKey;
	std::vector<std::shared_ptr<FontProcessor>> fontProcessors;
	parseFontKey(key, collectedFontKey, fontProcessors);

	const uint64 collectedFontKeyHash = rmx::getMurmur2_64(collectedFontKey);
	CollectedFont* collectedFont = mapFind(mCollectedFonts, collectedFontKeyHash);
	if (nullptr == collectedFont)
		return nullptr;

	Font& font = mFontPool.createObject();
	for (std::shared_ptr<FontProcessor>& fontProcessor : fontProcessors)
	{
		font.addFontProcessor(fontProcessor);
	}

	registerManagedFontInternal(font, *collectedFont);
	mFontsByKeyHash[keyHash] = &font;
	return &font;
}

bool FontCollection::registerManagedFont(Font& font, std::string_view key)
{
	const uint64 keyHash = rmx::getMurmur2_64(key);
	CollectedFont* collectedFont = mapFind(mCollectedFonts, keyHash);
	if (nullptr == collectedFont)
		return false;

	registerManagedFontInternal(font, *collectedFont);

	// Add to list of managed fonts as well
	ManagedFont& managedFont = vectorAdd(mAllManagedFonts);
	managedFont.mFont = &font;
	managedFont.mKey = key;
	return true;
}

void FontCollection::clear()
{
	for (auto& [key, collectedFont] : mCollectedFonts)
	{
		for (Font* font : collectedFont.mManagedFonts)
		{
			font->injectFontSource(nullptr);
		}
		delete collectedFont.mFontSource;
	}

	mCollectedFonts.clear();
	mFontsByKeyHash.clear();
	mFontPool.clear();
}

void FontCollection::reloadAll()
{
	// Load main game fonts
	clear();
	loadDefinitionsFromPath(L"data/font/", nullptr);

	// Load mod fonts
	collectFromMods();

	// Re-register managed fonts
	{
		std::vector<ManagedFont> managedFonts;
		managedFonts.swap(mAllManagedFonts);
		for (const ManagedFont& managedFont : managedFonts)
		{
			registerManagedFont(*managedFont.mFont, managedFont.mKey);
		}
	}
}

void FontCollection::collectFromMods()
{
	// Remove all definitions previously collected from mods, but not the main game ones
	for (auto& [key, collectedFont] : mCollectedFonts)
	{
		for (int index = (int)collectedFont.mDefinitions.size()-1; index >= 0; --index)
		{
			if (nullptr != collectedFont.mDefinitions[index].mMod)
			{
				// Update the index of the loaded definition beforehand
				if (collectedFont.mLoadedDefinitionIndex >= index)
				{
					if (collectedFont.mLoadedDefinitionIndex == index)
						collectedFont.mLoadedDefinitionIndex = -1;
					else
						--collectedFont.mLoadedDefinitionIndex;
				}

				// Remove that definition
				collectedFont.mDefinitions.erase(collectedFont.mDefinitions.begin() + index);
				--index;
			}
		}
	}

	// Scan for font definitions in mods
	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		loadDefinitionsFromPath(mod->mFullPath + L"font/", mod);
	}

	// Load the fonts
	updateLoadedFonts();
}

void FontCollection::registerManagedFontInternal(Font& font, CollectedFont& collectedFont)
{
	// Register as new managed font there
	collectedFont.mManagedFonts.push_back(&font);

	// Update the font source in all font instances (note that it might also be a null pointer)
	// TODO: Is this even needed for all managed fonts, or not just the one we're adding...?
	for (Font* managedFont : collectedFont.mManagedFonts)
	{
		managedFont->injectFontSource(collectedFont.mFontSource);
	}
}

void FontCollection::loadDefinitionsFromPath(std::wstring_view path, const Mod* mod)
{
	if (!FTX::FileSystem->exists(path))
		return;

	std::vector<rmx::FileIO::FileEntry> fileEntries;
	fileEntries.reserve(8);
	FTX::FileSystem->listFilesByMask(std::wstring(path) + L"*.json", false, fileEntries);

	for (const rmx::FileIO::FileEntry& fileEntry : fileEntries)
	{
		const std::wstring pureFilename = fileEntry.mFilename.substr(0, fileEntry.mFilename.length() - 5);	// Remove ".json"
		const std::string keyString = WString(pureFilename).toStdString();
		const uint64 keyHash = rmx::getMurmur2_64(keyString);

		// Get or create the collected font for this key
		CollectedFont& collectedFont = mCollectedFonts[keyHash];
		if (collectedFont.mManagedFonts.empty())	// This is true of the map entry was just created
		{
			// The list of managed font must always include the unmodified font
			collectedFont.mManagedFonts.push_back(&collectedFont.mUnmodifiedFont);
			collectedFont.mKeyHash = keyHash;
			collectedFont.mKeyString = keyString;
			mFontsByKeyHash[keyHash] = &collectedFont.mUnmodifiedFont;
		}

		// Add new definition
		Definition& definition = vectorAdd(collectedFont.mDefinitions);
		definition.mDefinitionFile = fileEntry.mPath + fileEntry.mFilename;
		definition.mMod = mod;
	}
}

void FontCollection::updateLoadedFonts()
{
	std::vector<uint64> keysToRemove;
	for (auto& [key, collectedFont] : mCollectedFonts)
	{
		if (collectedFont.mDefinitions.empty() && collectedFont.mManagedFonts.size() <= 1)
		{
			// Font is unused and can be removed
			keysToRemove.push_back(key);
			continue;
		}

		// Nothing to do if the last definition (= the highest priority one) is already loaded
		if (collectedFont.mDefinitions.size() == (size_t)collectedFont.mLoadedDefinitionIndex + 1)
			continue;

		// Font source needs to be reloaded
		collectedFont.mLoadedDefinitionIndex = -1;
		SAFE_DELETE(collectedFont.mFontSource);

		// Start at the end of the definitions list, at those have the highest priority
		for (int index = (int)collectedFont.mDefinitions.size() - 1; index >= 0; --index)
		{
			const Definition& definition = collectedFont.mDefinitions[index];
			collectedFont.mFontSource = new FontSourceBitmap(definition.mDefinitionFile);
			if (collectedFont.mFontSource->isValid())
			{
				collectedFont.mLoadedDefinitionIndex = index;
				break;
			}

			// If loading failed, try the next definition
			SAFE_DELETE(collectedFont.mFontSource);
		}

		// Update the font source in all font instances (note that it might also be a null pointer)
		for (Font* font : collectedFont.mManagedFonts)
		{
			font->injectFontSource(collectedFont.mFontSource);
		}

		// If loading failed for all definitions, remove the collected font instance
		if (collectedFont.mLoadedDefinitionIndex == -1 && collectedFont.mManagedFonts.size() <= 1)
		{
			keysToRemove.push_back(key);
		}
	}

	for (uint64 key : keysToRemove)
	{
		mCollectedFonts.erase(key);
		mFontsByKeyHash.erase(key);
	}

	// Invalidate cached printed texts
	RenderResources::instance().mPrintedTextCache.clear();
}
