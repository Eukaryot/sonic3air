/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/application/input/InputConfig.h"
#include "oxygen/application/input/RumbleEffectQueue.h"


class InputManager;
struct _SDL_Joystick;
struct _SDL_GameController;
typedef struct _SDL_Joystick SDL_Joystick;
typedef struct _SDL_GameController SDL_GameController;


class InputFeeder
{
public:
	virtual ~InputFeeder();

	void registerAtInputManager(InputManager& inputManager);
	void unregisterAtInputManager();

	virtual void updateControls() = 0;

protected:
	InputManager* mInputManager = nullptr;
};


class InputManager : public SingleInstance<InputManager>
{
public:
	struct RealDevice;

	enum class InputType
	{
		NONE,
		KEYBOARD,
		GAMEPAD,
		TOUCH		// Includes mouse clicks
	};

	struct ControlInput : public InputConfig::Assignment
	{
		RealDevice* mDevice = nullptr;
	};

	struct Control
	{
		std::vector<ControlInput> mInputs;
		bool mState = false;
		bool mPrevState = false;
		bool mChange = false;
		bool mRepeat = false;
		float mRepeatTimeout = 0.0f;
		int mPlayerIndex = 0;

		inline bool isPressed() const	{ return mState; }
		inline bool hasChanged() const	{ return mChange; }
		inline bool justPressed() const	{ return mState && mChange; }
		inline bool justPressedOrRepeat() const	{ return justPressed() || mRepeat; }

		void clearInputs();
		void addInput(RealDevice& device, InputConfig::Assignment::Type type, uint32 index);
		void addInput(RealDevice& device, const String& mappingString);
	};

	struct ControllerScheme
	{
		Control Up;
		Control Down;
		Control Left;
		Control Right;
		Control A;
		Control B;
		Control X;
		Control Y;
		Control Start;
		Control Back;
		Control L;
		Control R;
	};

	struct Touch
	{
		Vec2f mPosition;	// Normalized position in 0.0f ... 1.0f
	};

	enum class TouchInputMode
	{
		HIDDEN_CONTROLS	 = 0,		// Hidden touch controls overlay, no touch input possible
		FULLSCREEN_START = 1,		// Hidden touch controls overlay, reacting to a released touch anywhere on the screen by injecting a Start button press
		NORMAL_CONTROLS	 = 2		// Normal touch input via the visible touch controls overlay
	};

	struct RealDevice
	{
		InputConfig::DeviceType mType = InputConfig::DeviceType::KEYBOARD;
		SDL_Joystick* mSDLJoystick = nullptr;
		SDL_GameController* mSDLGameController = nullptr;
		int32 mSDLJoystickInstanceId = 0;			// This changes every time the controller is reconnected, but stays fixed from there on
		bool mSupportsRumble = false;
		std::vector<InputConfig::ControlMapping> mControlMappings;
		int mAssignedPlayer = 0;
		bool mDirty = false;		// Only for temporary use

		const char* getName() const;
	};

	struct RescanResult
	{
		uint32 mGamepadsFound = 0;
		uint32 mPreviousGamepadsFound = 0;
	};

public:
	InputManager();

	void startup();
	void enableTouchInput(bool enable);

	void updateInput(float timeElapsed);
	void injectSDLInputEvent(const SDL_Event& ev);

	const ControllerScheme& getController(size_t index) const;
	ControllerScheme& accessController(size_t index);
	inline const std::vector<Control*>& getAllControls() const  { return mAllControls; }

	void getPressedGamepadInputs(std::vector<InputConfig::Assignment>& outInputs, const RealDevice& device);
	inline const std::vector<Touch>& getActiveTouches() const  { return mActiveTouches; }

	RescanResult rescanRealDevices();
	void updatePlayerGamepadAssignments();

	const std::vector<RealDevice>& getKeyboards() const	{ return mKeyboards; }
	const std::vector<RealDevice>& getGamepads() const	{ return mGamepads; }
	uint32 getGamepadsChangeCounter() const  { return mGamepadsChangeCounter; }
	const RealDevice* getGamepadByJoystickInstanceId(int32 joystickInstanceId) const;

	int32 getPreferredGamepadByJoystickInstanceId(int playerIndex) const;
	void setPreferredGamepad(int playerIndex, const RealDevice* gamepad);

	InputConfig::DeviceDefinition* getDeviceDefinition(const RealDevice& device) const;
	const std::vector<InputConfig::Assignment>* getControlMapping(const RealDevice& device, InputConfig::DeviceDefinition::Button button) const;
	void redefineControlMapping(const RealDevice& device, InputConfig::DeviceDefinition::Button button, const std::vector<InputConfig::Assignment>& newAssignments);
	uint32 getMappingsChangeCounter() const  { return mMappingsChangeCounter; }

	void registerInputFeeder(InputFeeder& inputFeeder);
	void unregisterInputFeeder(InputFeeder& inputFeeder);

	inline InputType getLastInputType() const			{ return mLastInputType; }
	inline void setLastInputType(InputType inputType)	{ mLastInputType = inputType; }
	inline bool hasKeyboard() const						{ return mHasKeyboard; }
	inline bool anythingPressed() const					{ return mAnythingPressed; }

	void setControlState(Control& control, bool pressed);
	void setTouchInputMode(TouchInputMode mode);

	void resetControllerRumbleForPlayer(int playerIndex);
	void setControllerRumbleForPlayer(int playerIndex, float lowFrequencyRumble, float highFrequencyRumble, uint32 milliseconds);

	void setControllerLEDsForPlayer(int playerIndex, const Color& color) const;

	inline bool isUsingControlsLR() const  { return mUsingControlsLR; }
	void handleActiveModsChanged();

private:
	enum class WaitInputState
	{
		NONE,
		WAIT_FOR_RELEASE,
		WAIT_FOR_PRESS
	};

	struct PreferredGamepad
	{
		int32 mSDLJoystickInstanceId = -1;
		// The joystick / game controller name is stored in Configuration instead
	};

	struct Player
	{
		ControllerScheme mController;
		std::vector<Control*> mPlayerControls;
		PreferredGamepad mPreferredGamepad;
		RumbleEffectQueue mRumbleEffectQueue;
		RealDevice* mLastInputDevice = nullptr;
		RealDevice* mRumblingDevice = nullptr;
	};

private:
	RealDevice* findGamepadBySDLJoystickInstanceId(int32 joystickInstanceId);
	InputConfig::DeviceDefinition* getInputDeviceDefinitionByIdentifier(std::string_view identifier) const;

	bool isPressed(const Control& control);
	bool isPressed(const ControlInput& input);
	bool isPressed(SDL_Joystick* joystick, const ControlInput& input);

	void reapplyControllerRumble(int playerIndex);
	void stopControllerRumbleForDevice(RealDevice& device);

private:
	static const constexpr size_t NUM_PLAYERS = 2;
	Player mPlayers[NUM_PLAYERS];

	std::vector<Control*> mAllControls;
	std::vector<RealDevice> mKeyboards;
	std::vector<RealDevice> mGamepads;
	int mLastCheckJoysticks = 0;
	uint32 mGamepadsChangeCounter = 0;		// Gets increased whenever a gamepad is connected or unplugged
	uint32 mMappingsChangeCounter = 0;		// Gets increased whenever the buttons mappings get changed

	std::set<InputFeeder*> mInputFeeders;

	std::set<SDL_Keycode> mOneFrameKeyboardInputs;

	bool mTouchInputEnabled = false;
	std::vector<Touch> mActiveTouches;

	// Device type of last input -- for keyboard and gamepads, this is affected only by assigned controls, not by any pressed keys/buttons
	InputType mLastInputType = InputType::NONE;

	bool mHasKeyboard = false;			// Set if any key was ever pressed on a keyboard
	bool mAnythingPressed = false;		// Tracks whether there's any input at all at the moment

	TouchInputMode mTouchInputMode = TouchInputMode::HIDDEN_CONTROLS;
	WaitInputState mWaitingForSingleInput = WaitInputState::NONE;

	bool mUsingControlsLR = false;
};
