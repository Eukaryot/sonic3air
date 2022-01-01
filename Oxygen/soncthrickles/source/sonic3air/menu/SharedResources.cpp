/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/menu/SharedResources.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/EngineMain.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/FileHelper.h"
#include "oxygen/helper/JsonHelper.h"
#include "oxygen/helper/PackageFileCrawler.h"
#include "oxygen/rendering/utils/PaletteBitmap.h"
#include "oxygen/resources/ResourcesCache.h"


namespace
{
	struct OutlineFontProcessor : public FontProcessor
	{
		inline explicit OutlineFontProcessor(uint32 outlineColor = 0xff000000) : mOutlineColor(outlineColor) {}

		virtual void process(FontProcessingData& data) override
		{
			const int outlineWidth = 1;

			const int oldBorderLeft   = data.mBorderLeft;
			const int oldBorderRight  = data.mBorderRight;
			const int oldBorderTop    = data.mBorderTop;
			const int oldBorderBottom = data.mBorderBottom;

			data.mBorderLeft   = oldBorderLeft   + outlineWidth;
			data.mBorderRight  = oldBorderRight  + outlineWidth;
			data.mBorderTop    = oldBorderTop    + outlineWidth;
			data.mBorderBottom = oldBorderBottom + outlineWidth;

			const int newWidth  = data.mBitmap.mWidth  + (data.mBorderLeft + data.mBorderRight) - (oldBorderLeft + oldBorderRight);
			const int newHeight = data.mBitmap.mHeight + (data.mBorderTop + data.mBorderBottom) - (oldBorderTop + oldBorderBottom);
			const int insetX = data.mBorderLeft - oldBorderLeft;
			const int insetY = data.mBorderTop - oldBorderTop;

			Bitmap bitmap;
			bitmap.create(newWidth, newHeight, 0);

			for (int y = 0; y < data.mBitmap.mHeight; ++y)
			{
				for (int x = 0; x < data.mBitmap.mWidth; ++x)
				{
					if (data.mBitmap.mData[x + y * data.mBitmap.mWidth] & 0xff000000)
					{
						bitmap.mData[(insetX + x - 1) + (insetY + y) * bitmap.mWidth] = mOutlineColor;
						bitmap.mData[(insetX + x + 1) + (insetY + y) * bitmap.mWidth] = mOutlineColor;
						bitmap.mData[(insetX + x) + (insetY + y - 1) * bitmap.mWidth] = mOutlineColor;
						bitmap.mData[(insetX + x) + (insetY + y + 1) * bitmap.mWidth] = mOutlineColor;
					}
				}
			}
			bitmap.insertBlend(insetX, insetY, data.mBitmap);
			const int pixels = bitmap.getPixelCount();
			for (int i = 0; i < pixels; ++i)
			{
				float colorIntensity = (float)(bitmap.mData[i] & 0xff) / 255.0f;
				colorIntensity *= interpolate(0.65f, 1.0f, saturate((float)(i / bitmap.mWidth) / (float)bitmap.mHeight * 2.0f));
				bitmap.mData[i] = (bitmap.mData[i] & 0xff000000) | ((int)(colorIntensity * 255.5f) * 0x10101);
			}
			data.mBitmap = bitmap;
		}

	private:
		uint32 mOutlineColor = 0xff000000;
	};

	OutlineFontProcessor gOutlineFontProcessor;
	OutlineFontProcessor gOutlineFontProcessorTransparent(0x80000000);
}


namespace global
{
	Font mFont3Pure;
	Font mFont3;
	Font mFont4;
	Font mFont5;
	Font mFont7;
	Font mFont10;
	Font mFont18;

	DrawerTexture mGameLogo;
	DrawerTexture mMainMenuBackgroundLeft;
	DrawerTexture mMainMenuBackgroundSeparator;
	DrawerTexture mDataSelectBackground;
	DrawerTexture mDataSelectAltBackground;
	DrawerTexture mLevelSelectBackground;
	DrawerTexture mPreviewBorder;
	DrawerTexture mOptionsTopBar;
	DrawerTexture mCharactersIcon[3];
	DrawerTexture mCharSelectionBox;
	DrawerTexture mAchievementsFrame;
	DrawerTexture mPauseScreenUpperBG;
	DrawerTexture mPauseScreenLowerBG;
	DrawerTexture mPauseScreenDialog2BG;
	DrawerTexture mPauseScreenDialog3BG;
	DrawerTexture mTimeAttackResultsBG;

	std::map<ZoneActPreviewKey, DrawerTexture> mZoneActPreview;
	std::map<uint32, DrawerTexture> mAchievementImage;
	std::map<uint32, DrawerTexture> mSecretImage;


	void loadSharedResources()
	{
		ResourcesCache& resourcesCache = ResourcesCache::instance();
		resourcesCache.registerFontSource("monofont");
		resourcesCache.registerFontSource("oxyfont_light");
		resourcesCache.registerFontSource("oxyfont_regular");
		resourcesCache.registerFontSource("oxyfont_small");
		resourcesCache.registerFontSource("oxyfont_tiny");
		resourcesCache.registerFontSource("oxyfont_tiny_narrow");
		resourcesCache.registerFontSource("smallfont");
		resourcesCache.registerFontSource("sonic3_fontB");
		resourcesCache.registerFontSource("sonic3_fontC");

		mFont3Pure.loadFromFile("data/font/smallfont.json");

		mFont3.loadFromFile("data/font/smallfont.json");
		mFont3.addFontProcessor(gOutlineFontProcessorTransparent);

		mFont4.loadFromFile("data/font/oxyfont_tiny.json");
		mFont4.addFontProcessor(gOutlineFontProcessor);
		mFont4.setShadow(true, Vec2f(0.5f, 0.5f), 0.6f);

		mFont5.loadFromFile("data/font/oxyfont_small.json");
		mFont5.addFontProcessor(gOutlineFontProcessor);
		mFont5.setShadow(true, Vec2f(0.5f, 0.5f), 0.6f);

		mFont7.loadFromFile("data/font/sonic3_fontB.json");
		mFont7.addFontProcessor(gOutlineFontProcessor);
		mFont7.setShadow(true, Vec2f(0.5f, 0.5f), 0.6f);

		mFont10.loadFromFile("data/font/oxyfont_regular.json");
		mFont10.addFontProcessor(gOutlineFontProcessor);
		mFont10.setShadow(true, Vec2f(0.5f, 0.5f), 0.6f);

		mFont18.loadFromFile("data/font/sonic3_fontC.json");
		mFont18.addFontProcessor(gOutlineFontProcessor);
		mFont18.setShadow(true, Vec2f(1.0f, 0.5f), 0.5f);

		FileHelper::loadTexture(mGameLogo, L"data/images/menu/sonic3air_logo.png");
		FileHelper::loadTexture(mMainMenuBackgroundLeft, L"data/images/menu/mainmenu_bg_left.png");
		FileHelper::loadTexture(mMainMenuBackgroundSeparator, L"data/images/menu/mainmenu_bg_separator.png");
		FileHelper::loadTexture(mDataSelectBackground, L"data/images/menu/dataselect_bg.png");
		FileHelper::loadTexture(mDataSelectAltBackground, L"data/images/menu/dataselect_dark_bg.png");
		FileHelper::loadTexture(mLevelSelectBackground, L"data/images/menu/levelselect_bg.png");
		FileHelper::loadTexture(mPreviewBorder, L"data/images/menu/preview_border.png");
		FileHelper::loadTexture(mOptionsTopBar, L"data/images/menu/options_topbar_bg.png");
		FileHelper::loadTexture(mCharactersIcon[0], L"data/images/menu/charselect_sonic.png");
		FileHelper::loadTexture(mCharactersIcon[1], L"data/images/menu/charselect_tails.png");
		FileHelper::loadTexture(mCharactersIcon[2], L"data/images/menu/charselect_knuckles.png");
		FileHelper::loadTexture(mCharSelectionBox, L"data/images/menu/charselectionbox.png");
		FileHelper::loadTexture(mAchievementsFrame, L"data/images/menu/achievements_frame.png");
		FileHelper::loadTexture(mPauseScreenUpperBG, L"data/images/menu/pause_screen_upper.png");
		FileHelper::loadTexture(mPauseScreenLowerBG, L"data/images/menu/pause_screen_lower.png");
		FileHelper::loadTexture(mPauseScreenDialog2BG, L"data/images/menu/pause_screen_dialog.png");
		FileHelper::loadTexture(mPauseScreenDialog3BG, L"data/images/menu/pause_screen_dialog3.png");
		FileHelper::loadTexture(mTimeAttackResultsBG, L"data/images/menu/timeattack_results_screen.png");

		const std::vector<SharedDatabase::Zone>& zones = SharedDatabase::getAllZones();
		for (const SharedDatabase::Zone& zone : zones)
		{
			const uint8 acts = std::max(zone.mActsNormal, zone.mActsTimeAttack);
			if (acts == 0)
				continue;

			ZoneActPreviewKey key;
			key.mZone = zone.mInternalIndex;
			for (uint8 act = 0; act < acts; ++act)
			{
				key.mAct = act;
				for (uint8 image = 0; image < 2; ++image)
				{
					key.mImage = image;
					const String filename(0, "data/images/zone_preview/%s_act%d%c.png", zone.mShortName.substr(0, 6).c_str(), act + 1, 'a' + image);
					FileHelper::loadTexture(mZoneActPreview[key], *filename.toWString());
				}
			}
		}

		for (const SharedDatabase::Achievement& achievement : SharedDatabase::getAchievements())
		{
			const String filename(0, "data/images/achievements/%s.png", achievement.mImage.c_str());
			Bitmap bitmap;
			if (FileHelper::loadBitmap(bitmap, *filename.toWString()))
			{
				{
					DrawerTexture& texture = mAchievementImage[achievement.mType];
					if (!texture.isValid())
					{
						EngineMain::instance().getDrawer().createTexture(texture);
					}
					texture.accessBitmap() = bitmap;
					texture.bitmapUpdated();
				}

				// Convert to grayscale
				const int pixels = bitmap.getPixelCount();
				for (int i = 0; i < pixels; ++i)
				{
					bitmap.mData[i] = roundToInt(Color::fromABGR32(bitmap.mData[i]).getGray() * 255.0f) * 0x10101 + 0xff000000;
				}

				{
					DrawerTexture& texture = mAchievementImage[achievement.mType | 0x80000000];
					if (!texture.isValid())
					{
						EngineMain::instance().getDrawer().createTexture(texture);
					}
					texture.accessBitmap() = bitmap;
					texture.bitmapUpdated();
				}
			}
		}

		for (const SharedDatabase::Secret& secret : SharedDatabase::getSecrets())
		{
			const String filename(0, "data/images/secrets/%s.png", secret.mImage.c_str());
			FileHelper::loadTexture(mSecretImage[secret.mType], *filename.toWString());

			const String filename2(0, "data/images/secrets/%s_locked.png", secret.mImage.c_str());
			FileHelper::loadTexture(mSecretImage[secret.mType | 0x80000000], *filename2.toWString());
		}
	}
}
