/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "sonic3air/menu/GameMenuBase.h"
#include "oxygen/application/input/InputManager.h"

class OptionsMenu;


class ControllerSetupMenu : public GuiBase
{
public:
	enum class State
	{
		INACTIVE,
		APPEAR,
		SHOW,
		FADE_OUT
	};

public:
	ControllerSetupMenu(OptionsMenu& optionsMenu);
	~ControllerSetupMenu();

	void fadeIn();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

	inline float getVisibility() const  { return mVisibility; }
	inline bool isVisible() const  { return (mVisibility > 0.0f); }

private:
	const InputManager::RealDevice* getSelectedDevice() const;
	void abortButtonBinding(const InputManager::RealDevice& device);
	void assignButtonBinding(const InputManager::RealDevice& device, const InputConfig::Assignment& newAssignment);
	void assignButtonBindings(const InputManager::RealDevice& device, const std::vector<InputConfig::Assignment>& newAssignments);
	void onAssignmentDone(const InputManager::RealDevice& device);

	void goBack();
	void refreshGamepadList(bool forceUpdate = false);

private:
	State mState = State::INACTIVE;
	OptionsMenu& mOptionsMenu;
	GameMenuEntries mMenuEntries;
	GameMenuEntry* mControllerSelectEntry = nullptr;
	GameMenuEntries mAssignmentType;
	GameMenuScrolling mScrolling;

	bool mUsingControlsLR = false;
	uint32 mLastGamepadsChangeCounter = 0;

	int mCurrentlyAssigningButtonIndex = -1;
	bool mAppendAssignment = false;
	bool mAssignAll = false;
	bool mControlsBlocked = false;		// Button assignment stays blocked until the key that triggered it gets released
	std::vector<InputConfig::Assignment> mBlockedInputs;
	std::vector<InputConfig::Assignment> mTemp;

	float mVisibility = 0.0f;
};
