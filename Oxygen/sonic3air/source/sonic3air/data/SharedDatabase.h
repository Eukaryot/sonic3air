/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class EmulatorInterface;


class SharedDatabase
{
public:
	struct Zone
	{
		std::string mInitials;
		std::string mShortName;
		std::string mDisplayName;
		uint8 mInternalIndex = 0;
		int mActsNormal = 0;
		int mActsTimeAttack = 0;

		inline Zone(const std::string& initials, const std::string& shortName, const std::string& displayName, uint8 index, int actsNormal, int actsTimeAttack) :
			mInitials(initials), mShortName(shortName), mDisplayName(displayName), mInternalIndex(index), mActsNormal(actsNormal), mActsTimeAttack(actsTimeAttack) {}
	};

	struct Setting
	{
		enum Type		// Hold in sync with script (except for the INVALID entry)
		{
			INVALID								= 0,

			SETTING_FIX_GLITCHES				= 0x00000102,
			SETTING_NO_CONTROL_LOCK				= 0x00000201,
			SETTING_TAILS_ASSIST_MODE			= 0x00000401,
			SETTING_CANCEL_FLIGHT				= 0x00000501,
			SETTING_SUPER_CANCEL				= 0x00000601,
			SETTING_INSTA_SHIELD				= 0x00000701,
			SETTING_LEVELRESULT_SCORE			= 0x00001201,
			SETTING_HYPER_TAILS					= 0x00001301,
			SETTING_SHIELD_TYPES				= 0x00001401,

			SETTING_AIZ_BLIMPSEQUENCE			= 0x00010101,
			SETTING_AIZ_INTRO_KNUCKLES			= 0x00010301,
			SETTING_HCZ_WATERPIPE				= 0x00020101,
			SETTING_LBZ_TUBETRANSPORT			= 0x00060101,
			SETTING_LBZ_CUPELEVATOR				= 0x00060201,
			SETTING_LBZ_BIGARMS					= 0x00060301,
			SETTING_MHZ_ELEVATOR				= 0x00070101,
			SETTING_FBZ_ENTERCYLINDER			= 0x00080101,
			SETTING_FBZ_SCREWDOORS				= 0x00080201,
			SETTING_FASTER_PUSH					= 0x00090101,
			SETTING_SOZ_PYRAMID					= 0x00090201,
			SETTING_LRZ2_BOSS					= 0x000a0101,

			SETTING_EXTENDED_HUD				= 0x80000101,
			SETTING_SMOOTH_ROTATION				= 0x80000201,
			SETTING_SPEEDUP_AFTERIMGS			= 0x80000301,
			SETTING_PLAYER2_OFFSCREEN			= 0x80000401,
			SETTING_BS_VISUAL_STYLE				= 0x80009103,

			SETTING_KNUCKLES_AND_TAILS			= 0x00008100,
			SETTING_DROPDASH					= 0x00008200,
			SETTING_SUPER_PEELOUT				= 0x00008300,
			SETTING_DEBUG_MODE					= 0x00008400,
			SETTING_TITLE_SCREEN				= 0x00008500,

			SETTING_INFINITE_LIVES				= 0x00000800,
			SETTING_INFINITE_TIME				= 0x00000900,
			SETTING_RANDOM_MONITORS				= 0x00000c00,
			SETTING_RANDOM_SPECIALSTAGES		= 0x00000d00,
			SETTING_BUBBLE_SHIELD_BOUNCE		= 0x00001500,
			SETTING_CAMERA_OUTRUN				= 0x00001800,
			SETTING_EXTENDED_CAMERA				= 0x00001900,
			SETTING_MAINTAIN_SHIELDS			= 0x00002100,
			SETTING_MONITOR_STYLE				= 0x00002200,
			SETTING_HYPER_DASH_CONTROLS			= 0x00002300,
			SETTING_SUPER_SONIC_ABILITY			= 0x00002400,
			SETTING_MONITOR_BEHAVIOR			= 0x00002500,
			SETTING_BS_REPEAT_ON_FAIL			= 0x00009100,
			SETTING_DISABLE_GHOST_SPAWN			= 0x00090100,

			SETTING_HIDDEN_MONITOR_HINT			= 0x80000500,
			SETTING_SUPERFAST_RUNANIM			= 0x80000600,
			SETTING_LIVES_DISPLAY				= 0x80002300,
			SETTING_BS_COUNTDOWN_RINGS			= 0x80009000,
			SETTING_CONTINUE_MUSIC				= 0x8000a000,
			SETTING_UNDERWATER_AUDIO			= 0x8000a100,
			SETTING_ICZ_NIGHTTIME				= 0x80050200,
			SETTING_CNZ_PROTOTYPE_MUSIC			= 0x80041100,
			SETTING_ICZ_PROTOTYPE_MUSIC			= 0x80051100,
			SETTING_LBZ_PROTOTYPE_MUSIC			= 0x80061100,
			SETTING_FBZ2_MIDBOSS_TRACK			= 0x80080300,
			SETTING_SSZ_BOSS_TRACKS				= 0x800c0100,

			SETTING_GFX_ANTIFLICKER				= 0x00001a01,
			SETTING_LEVELLAYOUTS				= 0x00002002,
			SETTING_REGION_CODE					= 0x00003080,
			SETTING_TIME_ATTACK_GHOSTS			= 0x00003103,

			SETTING_AUDIO_TITLE_THEME			= 0x80400100,
			SETTING_AUDIO_EXTRALIFE_JINGLE		= 0x80400201,
			SETTING_AUDIO_INVINCIBILITY_THEME	= 0x80400301,
			SETTING_AUDIO_SUPER_THEME			= 0x80400404,
			SETTING_AUDIO_MINIBOSS_THEME		= 0x80400501,
			SETTING_AUDIO_KNUCKLES_THEME		= 0x80400601,
			SETTING_AUDIO_HPZ_MUSIC				= 0x80400701,
			SETTING_AUDIO_OUTRO					= 0x80400801,
			SETTING_AUDIO_COMPETITION_MENU		= 0x80400900,
			SETTING_AUDIO_CONTINUE_SCREEN		= 0x80400a01
		};

		enum class SerializationType
		{
			NONE,		// Not serialized
			HIDDEN,		// Hidden when default, i.e. does not get written in that case
			ALWAYS		// Serialized
		};

		Type mSettingId;
		std::string mIdentifier;
		uint32 mDefaultValue = 0;
		bool mPurelyVisual = false;
		bool mAllowInTimeAttack = false;
		SerializationType mSerializationType = SerializationType::NONE;
	};

	struct Achievement
	{
		enum Type		// Hold in sync with script
		{
			ACHIEVEMENT_300_RINGS				= 0x0001,
			ACHIEVEMENT_DOUBLE_INVINCIBILITY	= 0x0002,
			ACHIEVEMENT_CONTINUES				= 0x0010,
			ACHIEVEMENT_GOING_HYPER				= 0x0011,
			ACHIEVEMENT_SCORE					= 0x0012,
			ACHIEVEMENT_ELECTROCUTE				= 0x0013,
			ACHIEVEMENT_LONGPLAY				= 0x0020,
			ACHIEVEMENT_AIZ_TIMEATTACK			= 0x0101,
			ACHIEVEMENT_MGZ_GIANTRINGS			= 0x0301,
			ACHIEVEMENT_ICZ_SNOWBOARDING		= 0x0501,
			ACHIEVEMENT_ICZ_KNUX_SUNRISE		= 0x0502,
			ACHIEVEMENT_LBZ_STAY_DRY			= 0x0601,
			ACHIEVEMENT_MHZ_OPEN_MONITORS		= 0x0701,
			ACHIEVEMENT_FBZ_FREE_ANIMALS		= 0x0801,
			ACHIEVEMENT_SSZ_DECOYS				= 0x0c01,
			ACHIEVEMENT_GS_EXIT_TOP				= 0x1401,
			ACHIEVEMENT_SM_JACKPOT				= 0x1501,
			ACHIEVEMENT_BS_PERFECT				= 0x1601
		};

		Type mType;
		std::string mName;
		std::string mDescription;
		std::string mHint;
		std::string mImage;
		int32 mValue = 0;	// Temporary value used by scripts for whatever they need to store
	};

	struct Secret
	{
		enum Type		// Hold in sync with script; serialized secrets must not use values >= 0x20
		{
			SECRET_COMPETITION_MODE = 0x80,		// Not serialized, only for display in the Extras menu

			SECRET_KNUX_AND_TAILS	= 0x00,
			SECRET_DROPDASH			= 0x01,
			SECRET_DEBUGMODE		= 0x02,
			SECRET_SUPER_PEELOUT	= 0x03,
			SECRET_BLUE_SPHERE		= 0x04,

			SECRET_LEVELSELECT		= 0x10,
			SECRET_TITLE_SK			= 0x11,
			SECRET_GAME_SPEED		= 0x12,

			SECRET_DOOMSDAY_ZONE	= 0x18
		};

		Type mType;
		std::string mName;
		std::string mDescription;
		std::string mImage;
		uint32 mRequiredAchievements = 0;
		bool mUnlockedByAchievements = false;
		bool mShownInMenu = false;
		bool mHiddenUntilUnlocked = false;
		bool mSerialized = false;
	};

public:
	static void initialize();

	static const std::vector<Zone>& getAllZones()  { return mAllZones; }
	static const Zone* getZoneByInternalIndex(uint8 index);

	static uint64 setupCharacterSprite(EmulatorInterface& emulatorInterface, uint8 character, uint16 animationSprite, bool superActive = false);
	static uint64 setupTailsTailsSprite(EmulatorInterface& emulatorInterface, uint8 animationSprite);
	static uint8 getTailsTailsAnimationSprite(uint8 characterAnimationSprite, uint32 globalTime);

	static inline const std::unordered_map<uint32, Setting>& getSettings()  { return mSettings; }
	static const Setting* getSetting(uint32 settingId);
	static uint32 getSettingValue(uint32 settingId);

	static Achievement* getAchievement(uint32 achievementId);
	static const std::vector<Achievement>& getAchievements();
	static void resetAchievementValues();

	static Secret* getSecret(uint32 secretId);
	static inline const std::vector<Secret>& getSecrets()  { return mSecrets; }

private:
	static Setting& addSetting(SharedDatabase::Setting::Type id, const char* identifier, SharedDatabase::Setting::SerializationType serializationType, bool enforceAllowInTimeAttack = false);
	static void setupSettings();

private:
	static inline bool mIsInitialized;
	static inline std::vector<Zone> mAllZones;
	static inline std::vector<Zone> mAvailableZones;
	static inline std::unordered_map<uint32, Setting> mSettings;
	static inline std::vector<Achievement> mAchievements;
	static inline std::map<uint32, Achievement*> mAchievementMap;
	static inline std::vector<Secret> mSecrets;
};
