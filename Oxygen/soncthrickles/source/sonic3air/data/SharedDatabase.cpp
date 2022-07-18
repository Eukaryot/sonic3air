/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/data/SharedDatabase.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/resources/SpriteCache.h"


bool SharedDatabase::mIsInitialized = false;
std::vector<SharedDatabase::Zone> SharedDatabase::mAllZones;
std::vector<SharedDatabase::Zone> SharedDatabase::mAvailableZones;
std::unordered_map<uint32, SharedDatabase::Setting> SharedDatabase::mSettings;
std::vector<SharedDatabase::Achievement> SharedDatabase::mAchievements;
std::map<uint32, SharedDatabase::Achievement*> SharedDatabase::mAchievementMap;
std::vector<SharedDatabase::Secret> SharedDatabase::mSecrets;


void SharedDatabase::initialize()
{
	if (mIsInitialized)
		return;

	// Setup list of zones
	{
		mAllZones.emplace_back("aiz", "zone01_aiz", "Angel Island Zone",	0x00, 2, 2);
		mAllZones.emplace_back("hcz", "zone02_hcz", "Hydrocity Zone",		0x01, 2, 2);
		mAllZones.emplace_back("mgz", "zone03_mgz", "Marble Garden Zone",	0x02, 2, 2);
		mAllZones.emplace_back("cnz", "zone04_cnz", "Carnival Night Zone",	0x03, 2, 2);
		mAllZones.emplace_back("icz", "zone05_icz", "IceCap Zone",			0x05, 2, 2);
		mAllZones.emplace_back("lbz", "zone06_lbz", "Launch Base Zone",		0x06, 2, 2);
		mAllZones.emplace_back("mhz", "zone07_mhz", "Mushroom Hill Zone",	0x07, 2, 2);
		mAllZones.emplace_back("fbz", "zone08_fbz", "Flying Battery Zone",	0x04, 2, 2);
		mAllZones.emplace_back("soz", "zone09_soz", "Sandopolis Zone",		0x08, 2, 2);
		mAllZones.emplace_back("lrz", "zone10_lrz", "Lava Reef Zone",		0x09, 2, 2);
		mAllZones.emplace_back("hpz", "zone11_hpz", "Hidden Palace Zone",	0x16, 1, 0);	// Not for Time Attack
		mAllZones.emplace_back("ssz", "zone12_ssz", "Sky Sanctuary Zone",	0x0a, 1, 1);	// Only Act 1
		mAllZones.emplace_back("dez", "zone13_dez", "Death Egg Zone",		0x0b, 2, 2);
		mAllZones.emplace_back("ddz", "zone14_ddz", "Doomsday Zone",		0x0c, 1, 0);
	}

	// Setup gameplay settings
	{
		auto addSetting = [&](SharedDatabase::Setting::Type id, const char* identifier, SharedDatabase::Setting::SerializationType serializationType, bool enforceAllowInTimeAttack = false)
		{
			SharedDatabase::Setting& setting = mSettings[(uint32)id];
			setting.mSettingId = id;
			setting.mIdentifier = std::string(identifier).substr(9);
			setting.mValue = ((uint32)id & 0xff);
			setting.mDefaultValue = ((uint32)id & 0xff);
			setting.mSerializationType = serializationType;
			setting.mPurelyVisual = ((uint32)id & 0x80000000) != 0;
			setting.mAllowInTimeAttack = enforceAllowInTimeAttack || setting.mPurelyVisual;
		};

		#define ADD_SETTING(id) 			addSetting(id, #id, Setting::SerializationType::NONE);
		#define ADD_SETTING_HIDDEN(id) 		addSetting(id, #id, Setting::SerializationType::HIDDEN);
		#define ADD_SETTING_SERIALIZED(id) 	addSetting(id, #id, Setting::SerializationType::ALWAYS);
		#define ADD_SETTING_ALLOW_TA(id) 	addSetting(id, #id, Setting::SerializationType::ALWAYS, true);

		// These settings get saved in "settings.json" under their setting ID
		ADD_SETTING_SERIALIZED(Setting::SETTING_FIX_GLITCHES);
		ADD_SETTING_SERIALIZED(Setting::SETTING_NO_CONTROL_LOCK);
		ADD_SETTING_SERIALIZED(Setting::SETTING_TAILS_ASSIST_MODE);
		ADD_SETTING_SERIALIZED(Setting::SETTING_CANCEL_FLIGHT);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SUPER_CANCEL);
		ADD_SETTING_SERIALIZED(Setting::SETTING_INSTA_SHIELD);
		ADD_SETTING_SERIALIZED(Setting::SETTING_HYPER_TAILS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SHIELD_TYPES);

		ADD_SETTING_SERIALIZED(Setting::SETTING_AIZ_BLIMPSEQUENCE);
		ADD_SETTING_SERIALIZED(Setting::SETTING_LBZ_BIGARMS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_LRZ2_BOSS);

		ADD_SETTING_SERIALIZED(Setting::SETTING_EXTENDED_HUD);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SMOOTH_ROTATION);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SPEEDUP_AFTERIMGS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_BS_VISUAL_STYLE);

		ADD_SETTING_SERIALIZED(Setting::SETTING_INFINITE_LIVES);
		ADD_SETTING_SERIALIZED(Setting::SETTING_INFINITE_TIME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_RANDOM_MONITORS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_RANDOM_SPECIALSTAGES);
		ADD_SETTING_SERIALIZED(Setting::SETTING_BUBBLE_SHIELD_BOUNCE);
		ADD_SETTING_ALLOW_TA(Setting::SETTING_CAMERA_OUTRUN);			// Allowed in Time Attack, even though it's not purely visual (it has a minimal impact on gameplay simulation)
		ADD_SETTING_ALLOW_TA(Setting::SETTING_EXTENDED_CAMERA);			// Same here
		ADD_SETTING_SERIALIZED(Setting::SETTING_MAINTAIN_SHIELDS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_BS_REPEAT_ON_FAIL);
		ADD_SETTING_SERIALIZED(Setting::SETTING_DISABLE_GHOST_SPAWN);

		ADD_SETTING_SERIALIZED(Setting::SETTING_SUPERFAST_RUNANIM);
		ADD_SETTING_SERIALIZED(Setting::SETTING_MONITOR_STYLE);
		ADD_SETTING_SERIALIZED(Setting::SETTING_HYPER_DASH_CONTROLS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SUPER_SONIC_ABILITY);
		ADD_SETTING_SERIALIZED(Setting::SETTING_LIVES_DISPLAY);
		ADD_SETTING_SERIALIZED(Setting::SETTING_BS_COUNTDOWN_RINGS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_CONTINUE_MUSIC);
		ADD_SETTING_SERIALIZED(Setting::SETTING_UNDERWATER_AUDIO);
		ADD_SETTING_SERIALIZED(Setting::SETTING_ICZ_NIGHTTIME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_CNZ_PROTOTYPE_MUSIC);
		ADD_SETTING_SERIALIZED(Setting::SETTING_ICZ_PROTOTYPE_MUSIC);
		ADD_SETTING_SERIALIZED(Setting::SETTING_LBZ_PROTOTYPE_MUSIC);
		ADD_SETTING_SERIALIZED(Setting::SETTING_SSZ_BOSS_TRACKS);

		ADD_SETTING_ALLOW_TA(Setting::SETTING_GFX_ANTIFLICKER);			// Allowed in Time Attack
		ADD_SETTING_SERIALIZED(Setting::SETTING_LEVELLAYOUTS);
		ADD_SETTING_SERIALIZED(Setting::SETTING_REGION_CODE);
		ADD_SETTING_SERIALIZED(Setting::SETTING_TIME_ATTACK_GHOSTS);

		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_TITLE_THEME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_EXTRALIFE_JINGLE);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_INVINCIBILITY_THEME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_SUPER_THEME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_MINIBOSS_THEME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_KNUCKLES_THEME);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_HPZ_MUSIC);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_OUTRO);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_COMPETITION_MENU);
		ADD_SETTING_SERIALIZED(Setting::SETTING_AUDIO_CONTINUE_SCREEN);

		ADD_SETTING_HIDDEN(Setting::SETTING_DROPDASH);
		ADD_SETTING_HIDDEN(Setting::SETTING_SUPER_PEELOUT);
		ADD_SETTING_HIDDEN(Setting::SETTING_DEBUG_MODE);
		ADD_SETTING_HIDDEN(Setting::SETTING_TITLE_SCREEN);

		// These settings get saved either indirectly or not at all
		ADD_SETTING(Setting::SETTING_KNUCKLES_AND_TAILS);
	}

	// Setup achievements
	{
		mAchievements.reserve(10);

		auto addAchievement = [&](Achievement::Type type, const char* name, const char* description, const char* hint, const char* image)
		{
			Achievement& achievement = vectorAdd(mAchievements);
			achievement.mType = type;
			achievement.mName = name;
			achievement.mDescription = description;
			achievement.mHint = hint;
			achievement.mImage = image;
		};

		addAchievement(Achievement::ACHIEVEMENT_300_RINGS,				"Attracted to shiny things", "Collect 300 rings without losing them.", "", "rings");
		addAchievement(Achievement::ACHIEVEMENT_DOUBLE_INVINCIBILITY,	"Double dose of stars", "Open another invincibility monitor while still being invincible from the last one.", "", "invincibility");
		addAchievement(Achievement::ACHIEVEMENT_CONTINUES,				"Old-fashioned life insurance", "Have 5 continues in one game.", "", "continues");
		addAchievement(Achievement::ACHIEVEMENT_GOING_HYPER,			"Going Hyper", "Collect all 14 emeralds and transform to a Hyper form.", "", "hyperform");
		addAchievement(Achievement::ACHIEVEMENT_SCORE,					"Score millionaire", "Reach a score of 1,000,000 points.", "", "score");
		addAchievement(Achievement::ACHIEVEMENT_ELECTROCUTE,			"Electrofishing", "Defeat an underwater enemy by electrocution.", "", "electrocution");
		addAchievement(Achievement::ACHIEVEMENT_LONGPLAY,				"Longplay", "Beat the game with any character.", "", "gamebeaten");
		addAchievement(Achievement::ACHIEVEMENT_BS_PERFECT,				"Tidied up the place", "Complete a Blue Spheres stage with Perfect.", "", "perfect");
		addAchievement(Achievement::ACHIEVEMENT_GS_EXIT_TOP,			"Is there an exit up there", "Reach the top of the Glowing Spheres bonus stage.", "", "glowingspheres");
		addAchievement(Achievement::ACHIEVEMENT_SM_JACKPOT,				"Jackpot", "Win the Jackpot in the Slot Machines bonus stage.", "", "jackpot");
		addAchievement(Achievement::ACHIEVEMENT_AIZ_TIMEATTACK,			"Bursting through the jungle", "Finish Angel Island Zone Act 1 in Time Attack in under 45 seconds.", "", "timeattack_aiz1");
		addAchievement(Achievement::ACHIEVEMENT_MGZ_GIANTRINGS,			"Attracted to giant shiny things", "Enter or collect 6 giant rings in Marble Garden Zone Act 1 in a single run without dying in between.", "", "giantrings_mgz1");
		addAchievement(Achievement::ACHIEVEMENT_ICZ_SNOWBOARDING,		"Greedy snowboarder", "Collect all 50 rings in the snow boarding section of IceCap Zone Act 1.", "", "snowboarding");
		addAchievement(Achievement::ACHIEVEMENT_ICZ_KNUX_SUNRISE,		"Once see the sunrise", "Defeat the upper boss in IceCap Zone Act 1 with Knuckles (you may need a buddy for this).", "", "icecap1boss");
		addAchievement(Achievement::ACHIEVEMENT_LBZ_STAY_DRY,			"Fluffy fur must not get wet", "Get through Launch Base Zone Act 2 without touching any water (requires A.I.R. level layouts).", "", "staydry");
		addAchievement(Achievement::ACHIEVEMENT_MHZ_OPEN_MONITORS,		"Display smasher", "Open 18 monitors in Mushroom Hill Zone Act 1 with Knuckles.", "", "monitors");
		addAchievement(Achievement::ACHIEVEMENT_FBZ_FREE_ANIMALS,		"Squirrels on a plane", "Free 35 animals in Flying Battery Zone Act 1 before the boss.", "", "animals");
		addAchievement(Achievement::ACHIEVEMENT_SSZ_DECOYS,				"No touchy", "Fight the second boss in Sonic's Sky Sanctuary but pop at most one of the inflatable Mechas.", "", "decoys");

		for (Achievement& achievement : mAchievements)
		{
			mAchievementMap[achievement.mType] = &achievement;
		}
	}

	// Setup secrets
	{
		mSecrets.reserve(4);

		auto addSecret = [&](Secret::Type type, bool hiddenUntilUnlocked, bool shownInMenu, bool serialized, uint32 requiredAchievements, const char* name, const char* description, const char* image)
		{
			Secret& secret = vectorAdd(mSecrets);
			secret.mType = type;
			secret.mName = name;
			secret.mDescription = description;
			secret.mImage = image;
			secret.mRequiredAchievements = requiredAchievements;
			secret.mUnlockedByAchievements = (requiredAchievements > 0);
			secret.mHiddenUntilUnlocked = hiddenUntilUnlocked;
			secret.mShownInMenu = shownInMenu;
			secret.mSerialized = serialized;
		};

		addSecret(Secret::SECRET_COMPETITION_MODE,	false, true,  false,  0, "Competition Mode", "As known from original Sonic 3 (& Knuckles).", "competitionmode");
		addSecret(Secret::SECRET_DROPDASH,			false, true,  true,   3, "Sonic Drop Dash", "In the Options menu (in Controls), you can now activate Sonic's Drop Dash move for Normal Game and Act Select.", "dropdash");
		addSecret(Secret::SECRET_KNUX_AND_TAILS,	false, true,  true,   5, "Knuckles & Tails Mode", "Play as Knuckles & Tails character combination in Normal Game and Act Select.", "knuckles_tails");
		addSecret(Secret::SECRET_SUPER_PEELOUT,		false, true,  true,   7, "Sonic Super Peel-Out", "The Super Peel-Out move is available in the Options menu. This also unlocks \"Max Control\" Time Attack.", "superpeelout");
		addSecret(Secret::SECRET_DEBUGMODE,			false, true,  true,  10, "Debug Mode", "Debug Mode can be activated in the Options menu (in Tweaks), and is available in Normal Game and Act Select.", "debugmode");
		addSecret(Secret::SECRET_BLUE_SPHERE,		false, true,  true,  12, "Blue Sphere", "Adds the Blue Sphere game to Extras that is known from Sonic 1 locked on to Sonic & Knuckles.", "bluesphere");
		addSecret(Secret::SECRET_LEVELSELECT,		true,  true,  true,   0, "Level Select", "Adds the original Sonic 3 & Knuckles Level Select menu to Extras.", "levelselect");
		addSecret(Secret::SECRET_TITLE_SK,			true,  true,  true,   0, "Sonic & Knuckles Title", "You can now select the Sonic & Knuckles title screen in the Options menu.", "title_sk");
		addSecret(Secret::SECRET_GAME_SPEED,		true,  true,  true,   0, "Game Speed Setting", "Ready for a new challenge? Make the game faster (or slower) in the Options menu.", "gamespeed");
		addSecret(Secret::SECRET_DOOMSDAY_ZONE,		true,  false, true,   0, "Doomsday Zone", "", "");
	}

	mIsInitialized = true;
}

const SharedDatabase::Zone* SharedDatabase::getZoneByInternalIndex(uint8 index)
{
	for (const SharedDatabase::Zone& zone : SharedDatabase::getAllZones())
	{
		if (zone.mInternalIndex == index)
		{
			return &zone;
		}
	}
	return nullptr;
}

uint64 SharedDatabase::setupCharacterSprite(uint8 character, uint16 animationSprite, bool superActive)
{
	if (animationSprite >= 0x100)
	{
		// Special handling required
		if (animationSprite >= 0x102)
		{
			return rmx::getMurmur2_64(String(0, "sonic_peelout_%d", animationSprite - 0x102));
		}
		else
		{
			return rmx::getMurmur2_64(String(0, "sonic_dropdash_%d", animationSprite - 0x100));
		}
	}
	else
	{
		uint32 sourceBase;
		uint32 tableAddress;
		uint32 mappingOffset;
		switch (character)
		{
			default:
			case 0:		// Sonic
				sourceBase    = (animationSprite >= 0xda) ? 0x140060 : 0x100000;
				tableAddress  = (superActive) ? 0x148378 : 0x148182;
				mappingOffset = (superActive) ? 0x146816 : 0x146620;
				break;

			case 1:		// Tails
				sourceBase    = (animationSprite >= 0xd1) ? 0x143d00 : 0x3200e0;
				tableAddress  = 0x14a08a;
				mappingOffset = 0x148eb8;
				break;

			case 2:		// Knuckles
				sourceBase    = 0x1200e0;
				tableAddress  = 0x14bd0a;
				mappingOffset = 0x14a8d6;
				break;
		}

		return SpriteCache::instance().setupSpriteFromROM(sourceBase, tableAddress, mappingOffset, (uint8)animationSprite, 0x00, SpriteCache::ENCODING_CHARACTER);
	}
}

uint64 SharedDatabase::setupTailsTailsSprite(uint8 animationSprite)
{
	return SpriteCache::instance().setupSpriteFromROM(0x336620, 0x344d74, 0x344bb8, animationSprite, 0x00, SpriteCache::ENCODING_CHARACTER);
}

uint8 SharedDatabase::getTailsTailsAnimationSprite(uint8 characterAnimationSprite, uint32 globalTime)
{
	// Translate main character sprite into tails sprite
	//  -> Note that is only an estimation and does not represent the actual calculation by game
	if (characterAnimationSprite >= 0x86 && characterAnimationSprite <= 0x88)
	{
		// Spindash
		return 0x01 + (globalTime / 3) % 4;
	}
	else if (characterAnimationSprite >= 0x96 && characterAnimationSprite <= 0x98)
	{
		// Rolling
		return 0x05 + (globalTime / 4) % 4;
	}
	else if (characterAnimationSprite == 0x99 || (characterAnimationSprite >= 0xad && characterAnimationSprite <= 0xb4))
	{
		// Standing -- including idle anim, looking up/down
		return 0x22 + (globalTime / 8) % 5;
	}
	else if (characterAnimationSprite == 0xa0)
	{
		// Flying
		// TODO: When flying down, this is updated only every two frames, not every frame
		return 0x27 + (globalTime) % 2;
	}
	return 0;
}

const SharedDatabase::Setting* SharedDatabase::getSetting(uint32 settingId)
{
	const auto it = mSettings.find(settingId);
	return (it == mSettings.end()) ? nullptr : &it->second;
}

uint32 SharedDatabase::getSettingValue(uint32 settingId)
{
	const SharedDatabase::Setting* setting = getSetting(settingId);
	if (nullptr != setting)
	{
		return setting->mValue;
	}

	// Use default value
	return (settingId & 0xff);
}

SharedDatabase::Achievement* SharedDatabase::getAchievement(uint32 achievementId)
{
	return mAchievementMap[achievementId];
}

const std::vector<SharedDatabase::Achievement>& SharedDatabase::getAchievements()
{
	return mAchievements;
}

void SharedDatabase::resetAchievementValues()
{
	for (Achievement& achievement : mAchievements)
	{
		achievement.mValue = 0;
	}
}

SharedDatabase::Secret* SharedDatabase::getSecret(uint32 secretId)
{
	// No additional std::map used to optimize this, as the number of secrets is very low
	for (Secret& secret : mSecrets)
	{
		if (secret.mType == secretId)
			return &secret;
	}
	return nullptr;
}
