/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class SecretUnlockedWindow : public GuiBase
{
public:
	enum class EntryType
	{
		ACHIEVEMENT,
		SECRET
	};

public:
	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	void show(EntryType entryType, const std::string& title, const std::string& content, uint8 soundId);

private:
	struct Entry
	{
		EntryType mEntryType;
		std::string mTitle;
		std::string mContent;
		uint8 mSoundId = 0;
	};

private:
	void showInternal(Entry& entry);

private:
	enum Phase
	{
		PHASE_NONE,
		PHASE_TITLE_APPEAR,
		PHASE_TITLE_SHOW,
		PHASE_CONTENT_APPEAR,
		PHASE_CONTENT_SHOW,
		PHASE_DISAPPEAR
	};

	Phase mPhase = PHASE_NONE;
	float mPhaseTimer = 0.0f;	// Between 0.0f and 1.0f

	std::list<Entry> mEnqueuedEntries;
	Entry mShownEntry;
	std::vector<std::string> mShownTextLines;
};
