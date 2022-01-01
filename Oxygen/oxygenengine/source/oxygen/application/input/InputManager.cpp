/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/input/InputManager.h"
#include "oxygen/application/overlays/TouchControlsOverlay.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/helper/Logging.h"
#include "oxygen/rendering/utils/RenderUtils.h"
#include "oxygen/simulation/LogDisplay.h"


namespace
{

	const char* getJoystickName(SDL_Joystick* joystick)
	{
		if (nullptr == joystick)
			return "Unnamed controller";

		const char* name = SDL_JoystickName(joystick);
		return (nullptr == name) ? "Unnamed controller" : name;
	}

	const char* getGameControllerName(SDL_GameController* gameController)
	{
		if (nullptr == gameController)
			return "";		// Empty string by intent, so this case can easily be checked afterwards

		const char* name = SDL_GameControllerName(gameController);
		return (nullptr == name) ? "Unnamed controller" : name;
	}

	bool isBlacklistedName(const std::string& controllerOrJoystickName)
	{
		static std::vector<uint64> blacklistedHashes;
		if (blacklistedHashes.empty())
		{
			const std::vector<std::string> blacklist =
			{
				"virtual-search", "IPControl_UPnP_RemoteService", "shield-ask-remote",	// Dummy controllers that Nvidia Shield seems to create
				"uinput-fpc"															// Some other device
			};
			for (const std::string& str : blacklist)
			{
				blacklistedHashes.push_back(rmx::getMurmur2_64(str));
			}
		}

		const uint64 nameHash = rmx::getMurmur2_64(controllerOrJoystickName);
		for (uint64 blacklistedHash : blacklistedHashes)
		{
			if (blacklistedHash == nameHash)
				return true;
		}
		return false;
	}

	void collectControls(InputManager::ControllerScheme& controller, std::vector<InputManager::Control*>& outControls)
	{
		// This must match the "InputDeviceDefinition::Button" enum
		outControls.push_back(&controller.Up);
		outControls.push_back(&controller.Down);
		outControls.push_back(&controller.Left);
		outControls.push_back(&controller.Right);
		outControls.push_back(&controller.A);
		outControls.push_back(&controller.B);
		outControls.push_back(&controller.X);
		outControls.push_back(&controller.Y);
		outControls.push_back(&controller.Start);
		outControls.push_back(&controller.Back);
	}

	bool getControlAssignmentBySDLBinding(InputConfig::Assignment& output, const SDL_GameControllerButtonBind& binding, int axisDirection)
	{
		switch (binding.bindType)
		{
			case SDL_CONTROLLER_BINDTYPE_NONE:
				return false;

			case SDL_CONTROLLER_BINDTYPE_AXIS:
				output.mType = InputConfig::Assignment::Type::AXIS;
				output.mIndex = binding.value.axis * 2 + axisDirection;
				return true;

			case SDL_CONTROLLER_BINDTYPE_BUTTON:
				output.mType = InputConfig::Assignment::Type::BUTTON;
				output.mIndex = binding.value.button;
				return true;

			case SDL_CONTROLLER_BINDTYPE_HAT:
				output.mType = InputConfig::Assignment::Type::POV;
				output.mIndex = (binding.value.hat.hat * 0x100) + binding.value.hat.hat_mask;
				return true;
		}
		return false;
	}

	void setupRealDeviceInputMapping(InputManager::RealDevice& device, const InputConfig::DeviceDefinition& inputDeviceDefinition)
	{
		device.mControlMappings.resize((size_t)InputConfig::DeviceDefinition::Button::_NUM);
		for (size_t controlIndex = 0; controlIndex < device.mControlMappings.size(); ++controlIndex)
		{
			device.mControlMappings[controlIndex] = inputDeviceDefinition.mMappings[controlIndex];
		}
	}

	void setupRealDeviceInputMapping(InputManager::RealDevice& device, SDL_GameController& gameController)
	{
		using Button = InputConfig::DeviceDefinition::Button;
		std::vector<SDL_GameControllerButtonBind> bindings[(size_t)Button::_NUM];

		bindings[(size_t)Button::UP]   .emplace_back(SDL_GameControllerGetBindForAxis  (&gameController, SDL_CONTROLLER_AXIS_LEFTY));
		bindings[(size_t)Button::UP]   .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_DPAD_UP));
		bindings[(size_t)Button::DOWN] .emplace_back(SDL_GameControllerGetBindForAxis  (&gameController, SDL_CONTROLLER_AXIS_LEFTY));
		bindings[(size_t)Button::DOWN] .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_DPAD_DOWN));
		bindings[(size_t)Button::LEFT] .emplace_back(SDL_GameControllerGetBindForAxis  (&gameController, SDL_CONTROLLER_AXIS_LEFTX));
		bindings[(size_t)Button::LEFT] .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_DPAD_LEFT));
		bindings[(size_t)Button::RIGHT].emplace_back(SDL_GameControllerGetBindForAxis  (&gameController, SDL_CONTROLLER_AXIS_LEFTX));
		bindings[(size_t)Button::RIGHT].emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_DPAD_RIGHT));

		bindings[(size_t)Button::A]    .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_A));
		bindings[(size_t)Button::B]    .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_B));
		bindings[(size_t)Button::X]    .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_X));
		bindings[(size_t)Button::Y]    .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_Y));
		bindings[(size_t)Button::START].emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_START));
		bindings[(size_t)Button::START].emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_GUIDE));
		bindings[(size_t)Button::BACK] .emplace_back(SDL_GameControllerGetBindForButton(&gameController, SDL_CONTROLLER_BUTTON_BACK));

		device.mControlMappings.resize((size_t)Button::_NUM);
		for (size_t controlIndex = 0; controlIndex < device.mControlMappings.size(); ++controlIndex)
		{
			std::vector<InputConfig::Assignment>& assignments = device.mControlMappings[controlIndex].mAssignments;
			for (const SDL_GameControllerButtonBind& binding : bindings[controlIndex])
			{
				InputConfig::Assignment assignment;
				if (getControlAssignmentBySDLBinding(assignment, binding, controlIndex % 2))
				{
					assignments.emplace_back(assignment);
				}
			}
		}
	}

	void getMatchingInputDeviceDefinition(const InputManager::RealDevice& device, int index, const std::vector<InputConfig::DeviceDefinition>& definitions, const InputConfig::DeviceDefinition*& outMatchingDefinition, const InputConfig::DeviceDefinition*& outFallbackDefinition)
	{
		String joystickName = getJoystickName(device.mSDLJoystick);
		String controllerName = getGameControllerName(device.mSDLGameController);
		if (controllerName.nonEmpty() && controllerName != joystickName)
		{
			if (index >= 0)
				RMX_LOG_INFO("Controller #" << (index+1) << ": \"" << *joystickName << "\" (alternative name: \"" << *controllerName << "\")");
			controllerName.lowerCase();
		}
		else
		{
			if (index >= 0)
				RMX_LOG_INFO("Controller #" << (index+1) << ": \"" << *joystickName << "\"");
			controllerName.clear();
		}
		joystickName.lowerCase();
		const uint64 nameHashes[2] = { rmx::getMurmur2_64(joystickName), controllerName.empty() ? 0 : rmx::getMurmur2_64(controllerName) };
		static const uint64 WILDCARD_HASH = rmx::getMurmur2_64(String("*"));

		for (size_t k = 0; k < definitions.size(); ++k)
		{
			const InputConfig::DeviceDefinition& inputDeviceDefinition = definitions[k];
			if (!String(inputDeviceDefinition.mIdentifier).startsWith("Keyboard"))		// Ignore keyboard definitions here
			{
				for (const auto& pair : inputDeviceDefinition.mDeviceNames)
				{
					const uint64 deviceNameHash = pair.first;
					if (deviceNameHash == nameHashes[0] || (nameHashes[1] != 0 && deviceNameHash == nameHashes[1]))
					{
						outMatchingDefinition = &inputDeviceDefinition;
						return;
					}
					else if (deviceNameHash == WILDCARD_HASH)
					{
						outFallbackDefinition = &inputDeviceDefinition;
					}
				}
			}
		}
	}
}




InputFeeder::~InputFeeder()
{
	unregisterAtInputManager();
}

void InputFeeder::registerAtInputManager(InputManager& inputManager)
{
	if (mInputManager != &inputManager)
	{
		unregisterAtInputManager();
		mInputManager = &inputManager;
		mInputManager->registerInputFeeder(*this);
	}
}

void InputFeeder::unregisterAtInputManager()
{
	if (nullptr != mInputManager)
	{
		mInputManager->unregisterInputFeeder(*this);
		mInputManager = nullptr;
	}
}



void InputManager::Control::clearInputs()
{
	mInputs.clear();
}

void InputManager::Control::addInput(RealDevice& device, InputConfig::Assignment::Type type, uint32 index)
{
	ControlInput& input = vectorAdd(mInputs);
	input.mDevice = &device;
	input.mType = type;
	input.mIndex = index;
}

void InputManager::Control::addInput(RealDevice& device, const String& mappingString)
{
	InputConfig::Assignment assignment;
	if (InputConfig::Assignment::setFromMappingString(assignment, mappingString, device.mType))
	{
		addInput(device, assignment.mType, assignment.mIndex);
	}
}



const char* InputManager::RealDevice::getName() const
{
	const char* controllerName = getGameControllerName(mSDLGameController);
	if (nullptr != controllerName && controllerName[0] != 0)
	{
		return controllerName;
	}
	else
	{
		return getJoystickName(mSDLJoystick);
	}
}



InputManager::InputManager()
{
	mKeyboards.reserve(2);
	mGamepads.reserve(8);	// That's quite a lot, but we have to make sure this is never exceeded by the actual number of devices

	// Register controls
	for (int i = 0; i < 2; ++i)
	{
		collectControls(mController[i], mAllControls);
	}
}

void InputManager::startup()
{
	// Initialize gamepad
	SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	rescanRealDevices();
}

void InputManager::updateInput(float timeElapsed)
{
#if 0
	if (!mGamepads.empty())
	{
		String str;
		SDL_Joystick* joystick = mGamepads[0].mSDLJoystick;
		for (int i = 0; i < 20; ++i)
			str << SDL_JoystickGetButton(joystick, i) << " ";
		str << "-- ";
		for (int i = 0; i < 8; ++i)
			str << SDL_JoystickGetAxis(joystick, i) << " ";
		str << "-- ";
		for (int i = 0; i < 12; ++i)
			str << SDL_JoystickGetHat(joystick, i) << " ";
		LogDisplay::instance().setLogDisplay(str);
	}
#endif

	mAnythingPressed = false;

	// Update touches
	{
		mActiveTouches.clear();
		const int touchDevices = SDL_GetNumTouchDevices();
		for (int k = 0; k < touchDevices; ++k)
		{
			const SDL_TouchID touchId = SDL_GetTouchDevice(k);
			const int numFingers = SDL_GetNumTouchFingers(touchId);
			for (int i = 0; i < numFingers; ++i)
			{
				const SDL_Finger* finger = SDL_GetTouchFinger(touchId, i);
				if (nullptr != finger)
				{
					vectorAdd(mActiveTouches).mPosition.set(finger->x, finger->y);
				}
			}
		}

		// Also consider left mouse click
		if (FTX::mouseState(rmx::MouseButton::Left))
		{
			vectorAdd(mActiveTouches).mPosition = Vec2f(FTX::mousePos()) / Vec2f(FTX::screenSize());
		}

		if (!mActiveTouches.empty())
		{
			mLastInputType = InputType::TOUCH;
			mAnythingPressed = true;
		}
	}

	// Update controls
	{
		// Update all controls internally (i.e. the part not processed by input feeders)
		for (Control* control : mAllControls)
		{
			control->mPrevState = control->mState;
			control->mState = isPressed(*control);
		}

		// Update input feeders
		for (InputFeeder* inputFeeder : mInputFeeders)
		{
			inputFeeder->updateControls();
		}

		// Finalize controls
		for (Control* control : mAllControls)
		{
			control->mChange = (control->mState != control->mPrevState);

			// Update repeat
			control->mRepeat = false;
			if (control->mState)
			{
				if (control->mChange)
				{
					control->mRepeatTimeout = 0.4f;
				}
				else
				{
					control->mRepeatTimeout -= timeElapsed;
					if (control->mRepeatTimeout <= 0.0f)
					{
						control->mRepeat = true;
						control->mRepeatTimeout = std::max(control->mRepeatTimeout + 0.125f, 0.05f);
					}
				}
				mAnythingPressed = true;
			}
		}
	}

	// Reset one-frame inputs
	mOneFrameKeyboardInputs.clear();

	// Update touch input mode specific behavior
	if (mTouchInputMode == TouchInputMode::FULLSCREEN_START && mWaitingForSingleInput != WaitInputState::NONE)
	{
		switch (mWaitingForSingleInput)
		{
			case WaitInputState::WAIT_FOR_RELEASE:
			{
				if (!mAnythingPressed)
				{
					mWaitingForSingleInput = WaitInputState::WAIT_FOR_PRESS;
				}
				mAnythingPressed = false;
				break;
			}

			case WaitInputState::WAIT_FOR_PRESS:
			{
				if (mAnythingPressed)
				{
					// Inject a Start button press
					mController[0].Start.mState = true;
					mController[0].Start.mChange = true;

					// Leave this touch input mode, and by default show controls now (this may be overwritten later in the frame again)
					setTouchInputMode(TouchInputMode::NORMAL_CONTROLS);
				}
				break;
			}

			default:
				break;
		}
	}
}

void InputManager::injectSDLInputEvent(const SDL_Event& ev)
{
	switch (ev.type)
	{
		case SDL_KEYDOWN:
		{
			if (ev.key.state == SDL_PRESSED)	// This check may be unnecessary
			{
				// Add as one-frame input
				//  -> This is done so that very short key pressed get registered for one frame even if there is no "updateInput" call between key down and key up
				mOneFrameKeyboardInputs.insert(ev.key.keysym.sym);
				mHasKeyboard = true;
			}
			break;
		}
	}
}

const InputManager::ControllerScheme& InputManager::getController(size_t index) const
{
	RMX_ASSERT(index < 2, "Invalid index");
	return mController[index];
}

InputManager::ControllerScheme& InputManager::accessController(size_t index)
{
	RMX_ASSERT(index < 2, "Invalid index");
	return mController[index];
}

void InputManager::getPressedGamepadInputs(std::vector<InputConfig::Assignment>& outInputs, const RealDevice& device)
{
	outInputs.clear();
	if (device.mType != InputConfig::DeviceType::GAMEPAD)
		return;

	for (int k = 0; k < SDL_JoystickNumButtons(device.mSDLJoystick); ++k)
	{
		if (SDL_JoystickGetButton(device.mSDLJoystick, k) != 0)
		{
			outInputs.emplace_back(InputConfig::Assignment::Type::BUTTON, k);
		}
	}

	for (int k = 0; k < SDL_JoystickNumAxes(device.mSDLJoystick); ++k)
	{
		const int16 value = SDL_JoystickGetAxis(device.mSDLJoystick, k);
		if (value < -0x6000)
		{
			outInputs.emplace_back(InputConfig::Assignment::Type::AXIS, k*2);
		}
		else if (value > 0x6000)
		{
			outInputs.emplace_back(InputConfig::Assignment::Type::AXIS, k*2+1);
		}
	}

	for (int k = 0; k < SDL_JoystickNumHats(device.mSDLJoystick); ++k)
	{
		uint8 value = SDL_JoystickGetHat(device.mSDLJoystick, k);
		if (value != 0)
		{
			for (int bit = 1; bit < 0x100; bit *= 2)
			{
				if (value & bit)
				{
					outInputs.emplace_back(InputConfig::Assignment::Type::POV, k * 0x100 + bit);
					break;
				}
			}
		}
	}}

InputManager::RescanResult InputManager::rescanRealDevices()
{
	RescanResult result;
	result.mPreviousGamepadsFound = (uint32)mGamepads.size();
	result.mGamepadsFound = result.mPreviousGamepadsFound;

	// Anything changed at all?
	const int joysticks = SDL_NumJoysticks();
	if (joysticks == mLastCheckJoysticks && !mKeyboards.empty())
		return result;

	mLastCheckJoysticks = joysticks;
	Configuration& config = Configuration::instance();
	++mGamepadsChangeCounter;

	if (mKeyboards.empty())
	{
		// First-time setup for keyboards
		//  -> Though only one physical keyboard is supported, these are two "real devices", to allow for two players using one keyboard together
		for (size_t i = 0; i < 2; ++i)
		{
			RealDevice& device = vectorAdd(mKeyboards);
			device.mType = InputConfig::DeviceType::KEYBOARD;
			device.mSDLJoystick = nullptr;
			device.mSDLGameController = nullptr;

			using Button = InputConfig::DeviceDefinition::Button;
			bool found = false;
			const std::string key = (i == 0) ? "Keyboard1" : "Keyboard2";
			InputConfig::DeviceDefinition* inputDeviceDefinition = getInputDeviceDefinitionByIdentifier(key);
			if (nullptr == inputDeviceDefinition)
			{
				// Fallback in case something went wrong in config for whatever reason...
				RMX_ASSERT(false, "This shouldn't happen for keyboards");

				inputDeviceDefinition = &vectorAdd(config.mInputDeviceDefinitions);
				inputDeviceDefinition->mIdentifier = key;
				inputDeviceDefinition->mDeviceType = InputConfig::DeviceType::KEYBOARD;
				InputConfig::setupDefaultKeyboardMappings(*inputDeviceDefinition, (int)i);
			}

			::setupRealDeviceInputMapping(device, *inputDeviceDefinition);
		}
	}

	for (RealDevice& gamepad : mGamepads)
	{
		gamepad.mDirty = true;
	}
	for (int i = 0; i < joysticks; ++i)
	{
		// Respect the fixed limit of gamepads
		if (mGamepads.size() >= mGamepads.capacity())
			break;

		// Is this gamepad already in our list?
		SDL_Joystick* joystick = SDL_JoystickOpen(i);
		const int32 joystickInstanceId = SDL_JoystickInstanceID(joystick);
		{
			RealDevice* existingGamepad = findGamepadBySDLJoystickInstanceId(joystickInstanceId);
			if (nullptr != existingGamepad)
			{
				// Mark as still existing, and go to the next
				existingGamepad->mDirty = false;
				continue;
			}
		}

		// It's a newly connected (or maybe reconnected) device
		SDL_GameController* controller = SDL_GameControllerOpen(i);
		const std::string joystickName = getJoystickName(joystick);
		const std::string controllerName = getGameControllerName(controller);

		// Skip it if it's blacklisted
		if (isBlacklistedName(joystickName) || isBlacklistedName(controllerName))
			continue;

		RealDevice& device = vectorAdd(mGamepads);
		device.mType = InputConfig::DeviceType::GAMEPAD;
		device.mSDLJoystick = joystick;
		device.mSDLGameController = controller;
		device.mSDLJoystickInstanceId = joystickInstanceId;

		// Try to find a matching device definition in configuration
		const InputConfig::DeviceDefinition* matchingInputDeviceDefinition = nullptr;
		const InputConfig::DeviceDefinition* fallbackInputDeviceDefinition = nullptr;
		::getMatchingInputDeviceDefinition(device, (uint32)i, config.mInputDeviceDefinitions, matchingInputDeviceDefinition, fallbackInputDeviceDefinition);

		if (nullptr != matchingInputDeviceDefinition)
		{
			// Use device definition from config
			::setupRealDeviceInputMapping(device, *matchingInputDeviceDefinition);
		}
		else if (nullptr != device.mSDLGameController)
		{
			// Use SDL2 game controller lookup
			::setupRealDeviceInputMapping(device, *device.mSDLGameController);
		}
		else if (nullptr != fallbackInputDeviceDefinition)
		{
			// Use fallback from config
			::setupRealDeviceInputMapping(device, *fallbackInputDeviceDefinition);
		}
		else
		{
			continue;
		}

		// Log input mapping as JSON
		{
			static const String mappingKeys[12] = { "Up", "Down", "Left", "Right", "A", "B", "X", "Y", "Start", "Back" };
			RMX_LOG_INFO("{");
			for (size_t controlIndex = 0; controlIndex < device.mControlMappings.size(); ++controlIndex)
			{
				String line = String("\t\"") + mappingKeys[controlIndex] + "\": [ ";
				const std::vector<InputConfig::Assignment>& assignments = device.mControlMappings[controlIndex].mAssignments;
				bool first = true;
				for (const InputConfig::Assignment& assignment : assignments)
				{
					if (first)
						first = false;
					else
						line << ", ";

					switch (assignment.mType)
					{
						case InputConfig::Assignment::Type::AXIS:
						{
							line << "\"Axis" << assignment.mIndex << '"';
							break;
						}
						case InputConfig::Assignment::Type::BUTTON:
						{
							line << "\"Button" << assignment.mIndex << '"';
							break;
						}
						case InputConfig::Assignment::Type::POV:
						{
							line << "\"Pov" << rmx::log2(assignment.mIndex & 0xff) << '"';
							break;
						}
					}
				}
				line << " ]";
				if (controlIndex < device.mControlMappings.size() - 1)
					line << ",";
				RMX_LOG_INFO(*line);
			}
			RMX_LOG_INFO("}");
			RMX_LOG_INFO("");
		}

		// Add to device definitions
		if (nullptr == matchingInputDeviceDefinition)
		{
			InputConfig::DeviceDefinition& inputDeviceDefinition = vectorAdd(config.mInputDeviceDefinitions);
			inputDeviceDefinition.mIdentifier = controllerName.empty() ? joystickName : controllerName;
			inputDeviceDefinition.mDeviceType = InputConfig::DeviceType::GAMEPAD;

			inputDeviceDefinition.mDeviceNames[rmx::getMurmur2_64(joystickName)] = joystickName;
			if (!controllerName.empty() && controllerName != joystickName)
			{
				inputDeviceDefinition.mDeviceNames[rmx::getMurmur2_64(controllerName)] = controllerName;
			}

			for (size_t controlIndex = 0; controlIndex < device.mControlMappings.size(); ++controlIndex)
			{
				inputDeviceDefinition.mMappings[controlIndex].mAssignments = device.mControlMappings[controlIndex].mAssignments;
			}
		}

		// Check if this is a preferred gamepad for one of the players
		for (int playerIndex = 0; playerIndex < 2; ++playerIndex)
		{
			// This is only relevant if the player does not have a preferred gamepad already
			if (mPreferredGamepadByPlayer[playerIndex].mSDLJoystickInstanceId < 0)
			{
				const std::string& preferredGamepad = Configuration::instance().mPreferredGamepad[playerIndex];
				if (!preferredGamepad.empty() && (preferredGamepad == joystickName || preferredGamepad == controllerName))
				{
					mPreferredGamepadByPlayer[playerIndex].mSDLJoystickInstanceId = joystickInstanceId;
				}
			}
		}
	}

	// Remove all gamepads still marked as dirty, those got disconnected
	for (size_t i = 0; i < mGamepads.size(); ++i)
	{
		if (mGamepads[i].mDirty)
		{
			mGamepads.erase(mGamepads.begin() + i);
			--i;
		}
	}

	updatePlayerGamepadAssignments();

	result.mGamepadsFound = (uint32)mGamepads.size();
	return result;
}

void InputManager::updatePlayerGamepadAssignments()
{
	// Try to map real devices to players
	std::vector<RealDevice*> devicesByPlayer[2];
	for (size_t i = 0; i < mKeyboards.size(); ++i)
	{
		devicesByPlayer[i % 2].push_back(&mKeyboards[i]);
		mKeyboards[i].mAssignedPlayer = (int)i;
	}

	for (RealDevice& gamepad : mGamepads)
	{
		gamepad.mAssignedPlayer = Configuration::instance().mAutoAssignGamepadPlayerIndex;
	}
	for (int playerIndex = 1; playerIndex >= 0; --playerIndex)	// Reverse order to make sure player 1 overwrites player 2
	{
		RealDevice* gamepad = findGamepadBySDLJoystickInstanceId(mPreferredGamepadByPlayer[playerIndex].mSDLJoystickInstanceId);
		if (nullptr != gamepad)
		{
			gamepad->mAssignedPlayer = playerIndex;
		}
	}
	for (RealDevice& gamepad : mGamepads)
	{
		if (gamepad.mAssignedPlayer >= 0)
		{
			devicesByPlayer[gamepad.mAssignedPlayer].push_back(&gamepad);
		}
	}

	// Re-assign controls for the controller schemes
	for (int playerIndex = 0; playerIndex < 2; ++playerIndex)
	{
		std::vector<Control*> controls;
		collectControls(accessController(playerIndex), controls);
		RMX_ASSERT(controls.size() == (size_t)InputConfig::DeviceDefinition::Button::_NUM, "Collect controls did not get all buttons");

		for (size_t controlIndex = 0; controlIndex < controls.size(); ++controlIndex)
		{
			Control& control = *controls[controlIndex];
			control.clearInputs();

			for (RealDevice* device : devicesByPlayer[playerIndex])
			{
				if (!device->mControlMappings.empty())
				{
					std::vector<InputConfig::Assignment>& assignments = device->mControlMappings[controlIndex].mAssignments;
					for (InputConfig::Assignment& assignment : assignments)
					{
						control.addInput(*device, assignment.mType, assignment.mIndex);
					}
				}
			}
		}
	}

#ifdef PLATFORM_ANDROID
	// Explicitly add the Android back button
	accessController(0).Back.addInput(mKeyboards[0], InputConfig::Assignment::Type::BUTTON, SDLK_AC_BACK);
#endif
}

const InputManager::RealDevice* InputManager::getGamepadByJoystickInstanceId(int32 joystickInstanceId) const
{
	return const_cast<InputManager*>(this)->findGamepadBySDLJoystickInstanceId(joystickInstanceId);
}

int32 InputManager::getPreferredGamepadByJoystickInstanceId(int playerIndex) const
{
	RMX_ASSERT(playerIndex >= 0 && playerIndex < 2, "Invalid player index " << playerIndex);
	return mPreferredGamepadByPlayer[playerIndex].mSDLJoystickInstanceId;
}

void InputManager::setPreferredGamepad(int playerIndex, const RealDevice* gamepad)
{
	RMX_ASSERT(playerIndex >= 0 && playerIndex < 2, "Invalid player index " << playerIndex);
	PreferredGamepad& preferredGamepad = mPreferredGamepadByPlayer[playerIndex];
	if (nullptr != gamepad)
	{
		preferredGamepad.mSDLJoystickInstanceId = gamepad->mSDLJoystickInstanceId;
		Configuration::instance().mPreferredGamepad[playerIndex] = gamepad->getName();
	}
	else
	{
		preferredGamepad.mSDLJoystickInstanceId = -1;
		Configuration::instance().mPreferredGamepad[playerIndex].clear();
	}
	updatePlayerGamepadAssignments();
}

InputConfig::DeviceDefinition* InputManager::getDeviceDefinition(const RealDevice& device) const
{
	if (device.mType == InputConfig::DeviceType::KEYBOARD)
	{
		const int keyboardIndex = (&device == &mKeyboards[1]) ? 1 : 0;
		const std::string key = (keyboardIndex == 0) ? "Keyboard1" : "Keyboard2";
		return getInputDeviceDefinitionByIdentifier(key);
	}
	else
	{
		const InputConfig::DeviceDefinition* matchingInputDeviceDefinition = nullptr;
		const InputConfig::DeviceDefinition* fallbackInputDeviceDefinition = nullptr;
		::getMatchingInputDeviceDefinition(device, -1, Configuration::instance().mInputDeviceDefinitions, matchingInputDeviceDefinition, fallbackInputDeviceDefinition);
		return const_cast<InputConfig::DeviceDefinition*>(matchingInputDeviceDefinition);
	}
}

const std::vector<InputConfig::Assignment>* InputManager::getControlMapping(const RealDevice& device, InputConfig::DeviceDefinition::Button button) const
{
	const InputConfig::DeviceDefinition* inputDeviceDefinition = getDeviceDefinition(device);
	return (nullptr == inputDeviceDefinition) ? nullptr : &inputDeviceDefinition->mMappings[(size_t)button].mAssignments;
}

void InputManager::redefineControlMapping(const RealDevice& device, InputConfig::DeviceDefinition::Button button, const std::vector<InputConfig::Assignment>& newAssignments)
{
	InputConfig::DeviceDefinition* inputDeviceDefinition = getDeviceDefinition(device);
	if (nullptr != inputDeviceDefinition)
	{
		InputConfig::setAssignments(*inputDeviceDefinition, (size_t)button, newAssignments, true);

		// Now also reload mappings from config and apply them
		::setupRealDeviceInputMapping(const_cast<RealDevice&>(device), *inputDeviceDefinition);
		updatePlayerGamepadAssignments();
	}
}

void InputManager::registerInputFeeder(InputFeeder& inputFeeder)
{
	mInputFeeders.insert(&inputFeeder);
}

void InputManager::unregisterInputFeeder(InputFeeder& inputFeeder)
{
	mInputFeeders.erase(&inputFeeder);
}

void InputManager::setControlState(Control& control, bool pressed)
{
	control.mChange = (pressed != control.mState);
	control.mState = pressed;
}

void InputManager::setTouchInputMode(TouchInputMode mode)
{
	if (mTouchInputMode != mode)
	{
		const bool enableSingleInput = (mode == TouchInputMode::FULLSCREEN_START);
		const bool showOverlay = (mode == TouchInputMode::NORMAL_CONTROLS);
		mWaitingForSingleInput = enableSingleInput ? WaitInputState::WAIT_FOR_RELEASE : WaitInputState::NONE;
		if (TouchControlsOverlay::hasInstance())
		{
			TouchControlsOverlay::instance().setForceHidden(!showOverlay);
		}
		mTouchInputMode = mode;
	}
}

void InputManager::setControllerLEDsForPlayer(int playerIndex, const Color& color)
{
#if SDL_VERSION_ATLEAST(2, 0, 14)
	// TODO: Remove some of these exclusions where possible
#if !defined(PLATFORM_WEB) && !defined(PLATFORM_SWITCH) && !(defined(PLATFORM_WINDOWS) && defined(__GNUC__))
	for (size_t i = 0; i < mGamepads.size(); ++i)
	{
		if (mGamepads[i].mAssignedPlayer == playerIndex && nullptr != mGamepads[i].mSDLGameController)
		{
			SDL_GameControllerSetLED(mGamepads[i].mSDLGameController, (uint8)roundToInt(color.r * 255.0f), (uint8)roundToInt(color.g * 255.0f), (uint8)roundToInt(color.b * 255.0f));
		}
	}
#endif
#endif
}

InputManager::RealDevice* InputManager::findGamepadBySDLJoystickInstanceId(int32 joystickInstanceId)
{
	for (RealDevice& gamepad : mGamepads)
	{
		if (gamepad.mSDLJoystickInstanceId == joystickInstanceId)
			return &gamepad;
	}
	return nullptr;
}

InputConfig::DeviceDefinition* InputManager::getInputDeviceDefinitionByIdentifier(const std::string& identifier) const
{
	Configuration& config = Configuration::instance();
	for (size_t k = 0; k < config.mInputDeviceDefinitions.size(); ++k)
	{
		if (config.mInputDeviceDefinitions[k].mIdentifier == identifier)
		{
			return &config.mInputDeviceDefinitions[k];
		}
	}
	return nullptr;
}

bool InputManager::isPressed(const Control& control)
{
	// Handle key and button assignments
	for (const ControlInput& input : control.mInputs)
	{
		if (isPressed(input))
		{
			mLastInputType = (input.mDevice->mType == InputConfig::DeviceType::GAMEPAD) ? InputType::GAMEPAD : InputType::KEYBOARD;
			return true;
		}
	}
	return false;
}

bool InputManager::isPressed(const ControlInput& input)
{
	if (nullptr == input.mDevice)
		return false;

	switch (input.mDevice->mType)
	{
		case InputConfig::DeviceType::KEYBOARD:
		{
			if (FTX::keyState(input.mIndex) || mOneFrameKeyboardInputs.count(input.mIndex) > 0)
			{
				// Ignore key presses while Alt is down
				if (!FTX::keyState(SDLK_LALT) && !FTX::keyState(SDLK_RALT))
					return true;
			}
			break;
		}

		case InputConfig::DeviceType::GAMEPAD:
		{
			if (isPressed(input.mDevice->mSDLJoystick, input))
				return true;
			break;
		}
	}
	return false;
}

bool InputManager::isPressed(SDL_Joystick* joystick, const ControlInput& input)
{
	if (nullptr != joystick)
	{
		switch (input.mType)
		{
			case InputConfig::Assignment::Type::AXIS:
			{
				// Use even number for negative axis direction, odd number for positive axis direction
				const float value = (float)SDL_JoystickGetAxis(joystick, input.mIndex / 2) / 32767.0f;
				if ((input.mIndex % 2) == 0)
				{
					return (value < -0.25f);
				}
				else
				{
					return (value > 0.25f);
				}
			}

			case InputConfig::Assignment::Type::BUTTON:
			{
				return (SDL_JoystickGetButton(joystick, input.mIndex) > 0);
			}

			case InputConfig::Assignment::Type::POV:
			{
				const uint8 hatMask = SDL_JoystickGetHat(joystick, input.mIndex >> 8);
				return (hatMask & input.mIndex) != 0;
			}
		}
	}
	return false;
}
