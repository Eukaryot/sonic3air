/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
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
#include "oxygen/rendering/utils/PaletteBitmap.h"
#include "oxygen/resources/FontCollection.h"


namespace global
{
	Font mFont3Pure;
	Font mFont3;
	Font mFont4;
	Font mFont5;
	Font mFont7;
	Font mFont10;
	Font mFont18;

	DrawerTexture mMainMenuBackgroundSeparator;
	DrawerTexture mDataSelectBackground;
	DrawerTexture mDataSelectAltBackground;
	DrawerTexture mLevelSelectBackground;
	DrawerTexture mPreviewBorder;
	DrawerTexture mOptionsTopBar;
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
		std::shared_ptr<ShadowFontProcessor> shadowFontProcessor  = std::make_shared<ShadowFontProcessor>(Vec2i(1, 1), 0.5f, 0.8f);
		std::shared_ptr<ShadowFontProcessor> shadowFontProcessor2 = std::make_shared<ShadowFontProcessor>(Vec2i(1, 1), 0.5f, 1.0f);
		std::shared_ptr<ShadowFontProcessor> shadowFontProcessor3 = std::make_shared<ShadowFontProcessor>(Vec2i(1, 1), 0.5f, 0.6f);

		std::shared_ptr<OutlineFontProcessor> outlineFontProcessor = std::make_shared<OutlineFontProcessor>();
		std::shared_ptr<OutlineFontProcessor> outlineFontProcessorTransparent = std::make_shared<OutlineFontProcessor>(Color(0.0f, 0.0f, 0.0f, 0.5f));

		std::shared_ptr<GradientFontProcessor> gradientFontProcessor = std::make_shared<GradientFontProcessor>();

		FontCollection& fontCollection = FontCollection::instance();

		fontCollection.registerManagedFont(mFont3Pure, "smallfont");

		fontCollection.registerManagedFont(mFont3, "smallfont");
		mFont3.addFontProcessor(outlineFontProcessorTransparent);

		fontCollection.registerManagedFont(mFont4, "oxyfont_tiny");
		mFont4.addFontProcessor(outlineFontProcessor);
		mFont4.addFontProcessor(gradientFontProcessor);
		mFont4.addFontProcessor(shadowFontProcessor3);

		fontCollection.registerManagedFont(mFont5, "oxyfont_small");
		mFont5.addFontProcessor(outlineFontProcessor);
		mFont5.addFontProcessor(gradientFontProcessor);
		mFont5.addFontProcessor(shadowFontProcessor);

		fontCollection.registerManagedFont(mFont7, "sonic3_fontB");
		mFont7.addFontProcessor(outlineFontProcessor);
		mFont7.addFontProcessor(gradientFontProcessor);
		mFont7.addFontProcessor(shadowFontProcessor);

		fontCollection.registerManagedFont(mFont10, "oxyfont_regular");
		mFont10.addFontProcessor(outlineFontProcessor);
		mFont10.addFontProcessor(gradientFontProcessor);
		mFont10.addFontProcessor(shadowFontProcessor);

		fontCollection.registerManagedFont(mFont18, "sonic3_fontC");
		mFont18.addFontProcessor(outlineFontProcessor);
		mFont18.addFontProcessor(gradientFontProcessor);
		mFont18.addFontProcessor(shadowFontProcessor2);

		FileHelper::loadTexture(mMainMenuBackgroundSeparator, L"data/images/menu/mainmenu_bg_separator.png");
		FileHelper::loadTexture(mDataSelectBackground, L"data/images/menu/dataselect_bg.png");
		FileHelper::loadTexture(mDataSelectAltBackground, L"data/images/menu/dataselect_dark_bg.png");
		FileHelper::loadTexture(mLevelSelectBackground, L"data/images/menu/levelselect_bg.png");
		FileHelper::loadTexture(mPreviewBorder, L"data/images/menu/preview_border.png");
		FileHelper::loadTexture(mOptionsTopBar, L"data/images/menu/options_topbar_bg.png");
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
				uint32* data = bitmap.getData();
				const int pixels = bitmap.getPixelCount();
				for (int i = 0; i < pixels; ++i)
				{
					data[i] = roundToInt(Color::fromABGR32(data[i]).getGray() * 255.0f) * 0x10101 + 0xff000000;
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
			if (!secret.mImage.empty())
			{
				const String filename(0, "data/images/secrets/%s.png", secret.mImage.c_str());
				FileHelper::loadTexture(mSecretImage[secret.mType], *filename.toWString());

				const String filename2(0, "data/images/secrets/%s_locked.png", secret.mImage.c_str());
				FileHelper::loadTexture(mSecretImage[secret.mType | 0x80000000], *filename2.toWString(), false);	// This is okay to fail for some secrets
			}
		}
	}
}
