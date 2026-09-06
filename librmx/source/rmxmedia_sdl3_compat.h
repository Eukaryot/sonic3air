/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#ifdef RMX_USE_SDL3
	// Translate renamed identifiers
	//  -> This is not a complete list, only what's used and can easily be replaced without further adjustments
	#undef SDL_TRUE
	#undef SDL_FALSE
	#undef SDL_mutex
	#undef SDL_cond
	#undef SDL_CreateCond
	#undef SDL_DestroyCond
	#undef SDL_CondSignal
	#undef SDL_CondWaitTimeout
	#undef SDL_RWops
	#undef SDL_RWFromFile
	#undef SDL_RWclose
	#undef SDL_RWsize
	#undef SDL_RWtell
	#undef SDL_RWseek
	#undef RW_SEEK_SET
	#undef SDL_NUM_SCANCODES
	#undef SDL_QUIT
	#undef SDL_KEYDOWN
	#undef SDL_KEYUP
	#undef SDL_TEXTINPUT
	#undef SDL_MOUSEBUTTONDOWN
	#undef SDL_MOUSEBUTTONUP
	#undef SDL_MOUSEWHEEL
	#undef SDL_MOUSEMOTION
	#undef SDL_APP_WILLENTERBACKGROUND
	#undef SDL_JOYDEVICEADDED
	#undef SDL_JOYDEVICEREMOVED
	#undef SDL_GetWindowDisplayIndex
	#undef SDL_FreeSurface
	#undef SDL_JoystickName
	#undef SDL_JoystickNumButtons
	#undef SDL_JoystickGetButton
	#undef SDL_JoystickNumAxes
	#undef SDL_JoystickGetAxis
	#undef SDL_JoystickNumHats
	#undef SDL_JoystickGetHat
	#undef SDL_JoystickOpen
	#undef SDL_JoystickInstanceID
	#undef SDL_JoystickRumble
	#undef SDL_GameController
	#undef SDL_GameControllerOpen
	#undef SDL_GameControllerSetLED
	#undef SDL_GameControllerName
	#undef SDL_GameControllerButtonBind
	#undef SDL_CONTROLLER_BINDTYPE_NONE
	#undef SDL_CONTROLLER_BINDTYPE_AXIS
	#undef SDL_CONTROLLER_BINDTYPE_BUTTON
	#undef SDL_CONTROLLER_BINDTYPE_HAT
	#undef SDL_CONTROLLER_AXIS_LEFTX
	#undef SDL_CONTROLLER_AXIS_LEFTY
	#undef SDL_CONTROLLER_BUTTON_DPAD_UP
	#undef SDL_CONTROLLER_BUTTON_DPAD_DOWN
	#undef SDL_CONTROLLER_BUTTON_DPAD_LEFT
	#undef SDL_CONTROLLER_BUTTON_DPAD_RIGHT
	#undef SDL_CONTROLLER_BUTTON_A
	#undef SDL_CONTROLLER_BUTTON_B
	#undef SDL_CONTROLLER_BUTTON_X
	#undef SDL_CONTROLLER_BUTTON_Y
	#undef SDL_CONTROLLER_BUTTON_START
	#undef SDL_CONTROLLER_BUTTON_GUIDE
	#undef SDL_CONTROLLER_BUTTON_BACK
	#undef SDL_CONTROLLER_BUTTON_LEFTSHOULDER
	#undef SDL_CONTROLLER_BUTTON_RIGHTSHOULDER
	#undef KMOD_SHIFT
	#undef KMOD_CTRL
	#undef KMOD_ALT
	#undef KMOD_LSHIFT
	#undef KMOD_LCTRL
	#undef KMOD_LALT
	#undef SDLK_a
	#undef SDLK_b
	#undef SDLK_c
	#undef SDLK_d
	#undef SDLK_e
	#undef SDLK_f
	#undef SDLK_g
	#undef SDLK_h
	#undef SDLK_i
	#undef SDLK_j
	#undef SDLK_k
	#undef SDLK_l
	#undef SDLK_m
	#undef SDLK_n
	#undef SDLK_o
	#undef SDLK_p
	#undef SDLK_q
	#undef SDLK_r
	#undef SDLK_s
	#undef SDLK_t
	#undef SDLK_u
	#undef SDLK_v
	#undef SDLK_w
	#undef SDLK_x
	#undef SDLK_y
	#undef SDLK_z
	#undef SDLK_BACKQUOTE
	#undef SDLK_QUOTE
	#undef SDLK_QUOTEDBL

	#define SDL_TRUE			true
	#define SDL_FALSE			false
	#define SDL_mutex			SDL_Mutex
	#define SDL_cond			SDL_Condition
	#define SDL_CreateCond		SDL_CreateCondition
	#define SDL_DestroyCond		SDL_DestroyCondition
	#define SDL_CondSignal		SDL_SignalCondition
	#define SDL_CondWaitTimeout	SDL_WaitConditionTimeout
	#define SDL_RWops			SDL_IOStream
	#define SDL_RWFromFile		SDL_IOFromFile
	#define SDL_RWclose			SDL_CloseIO
	#define SDL_RWsize			SDL_GetIOSize
	#define SDL_RWtell			SDL_TellIO
	#define SDL_RWseek			SDL_SeekIO
	#define RW_SEEK_SET			SDL_IO_SEEK_SET
	#define SDL_NUM_SCANCODES	SDL_SCANCODE_COUNT
	#define SDL_QUIT			SDL_EVENT_QUIT
	#define SDL_KEYDOWN			SDL_EVENT_KEY_DOWN
	#define SDL_KEYUP			SDL_EVENT_KEY_UP
	#define SDL_TEXTINPUT		SDL_EVENT_TEXT_INPUT
	#define SDL_MOUSEBUTTONDOWN	SDL_EVENT_MOUSE_BUTTON_DOWN
	#define SDL_MOUSEBUTTONUP	SDL_EVENT_MOUSE_BUTTON_UP
	#define SDL_MOUSEWHEEL		SDL_EVENT_MOUSE_WHEEL
	#define SDL_MOUSEMOTION		SDL_EVENT_MOUSE_MOTION
	#define SDL_APP_WILLENTERBACKGROUND SDL_EVENT_WILL_ENTER_BACKGROUND
	#define SDL_JOYDEVICEADDED	SDL_EVENT_JOYSTICK_ADDED
	#define SDL_JOYDEVICEREMOVED SDL_EVENT_JOYSTICK_REMOVED
	#define SDL_GetWindowDisplayIndex SDL_GetDisplayForWindow
	#define SDL_FreeSurface		SDL_DestroySurface
	#define SDL_JoystickName	SDL_GetJoystickName
	#define SDL_JoystickNumButtons SDL_GetNumJoystickButtons
	#define SDL_JoystickGetButton SDL_GetJoystickButton
	#define SDL_JoystickNumAxes	SDL_GetNumJoystickAxes
	#define SDL_JoystickGetAxis	SDL_GetJoystickAxis
	#define SDL_JoystickNumHats	SDL_GetNumJoystickHats
	#define SDL_JoystickGetHat	SDL_GetJoystickHat
	#define SDL_JoystickOpen	SDL_OpenJoystick
	#define SDL_JoystickInstanceID SDL_GetJoystickID
	#define SDL_JoystickRumble	SDL_RumbleJoystick
	#define SDL_GameController	SDL_Gamepad
	#define SDL_GameControllerOpen SDL_OpenGamepad
	#define SDL_GameControllerSetLED SDL_SetGamepadLED
	#define SDL_GameControllerName SDL_GetGamepadName
	#define SDL_GameControllerButtonBind SDL_GamepadBinding
	#define SDL_CONTROLLER_BINDTYPE_NONE SDL_GAMEPAD_BINDTYPE_NONE
	#define SDL_CONTROLLER_BINDTYPE_AXIS SDL_GAMEPAD_BINDTYPE_AXIS
	#define SDL_CONTROLLER_BINDTYPE_BUTTON SDL_GAMEPAD_BINDTYPE_BUTTON
	#define SDL_CONTROLLER_BINDTYPE_HAT SDL_GAMEPAD_BINDTYPE_HAT
	#define SDL_CONTROLLER_AXIS_LEFTX SDL_GAMEPAD_AXIS_LEFTX
	#define SDL_CONTROLLER_AXIS_LEFTY SDL_GAMEPAD_AXIS_LEFTY
	#define SDL_CONTROLLER_BUTTON_DPAD_UP SDL_GAMEPAD_BUTTON_DPAD_UP
	#define SDL_CONTROLLER_BUTTON_DPAD_DOWN SDL_GAMEPAD_BUTTON_DPAD_DOWN
	#define SDL_CONTROLLER_BUTTON_DPAD_LEFT SDL_GAMEPAD_BUTTON_DPAD_LEFT
	#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT SDL_GAMEPAD_BUTTON_DPAD_RIGHT
	#define SDL_CONTROLLER_BUTTON_A SDL_GAMEPAD_BUTTON_SOUTH
	#define SDL_CONTROLLER_BUTTON_B SDL_GAMEPAD_BUTTON_EAST
	#define SDL_CONTROLLER_BUTTON_X SDL_GAMEPAD_BUTTON_WEST
	#define SDL_CONTROLLER_BUTTON_Y SDL_GAMEPAD_BUTTON_NORTH
	#define SDL_CONTROLLER_BUTTON_START SDL_GAMEPAD_BUTTON_START
	#define SDL_CONTROLLER_BUTTON_GUIDE SDL_GAMEPAD_BUTTON_GUIDE
	#define SDL_CONTROLLER_BUTTON_BACK SDL_GAMEPAD_BUTTON_BACK
	#define SDL_CONTROLLER_BUTTON_LEFTSHOULDER SDL_GAMEPAD_BUTTON_LEFT_SHOULDER
	#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
	#define KMOD_SHIFT			SDL_KMOD_SHIFT
	#define KMOD_CTRL			SDL_KMOD_CTRL
	#define KMOD_ALT			SDL_KMOD_ALT
	#define KMOD_LSHIFT			SDL_KMOD_LSHIFT
	#define KMOD_LCTRL			SDL_KMOD_LCTRL
	#define KMOD_LALT			SDL_KMOD_LALT
	#define SDLK_a				SDLK_A
	#define SDLK_b				SDLK_B
	#define SDLK_c				SDLK_C
	#define SDLK_d				SDLK_D
	#define SDLK_e				SDLK_E
	#define SDLK_f				SDLK_F
	#define SDLK_g				SDLK_G
	#define SDLK_h				SDLK_H
	#define SDLK_i				SDLK_I
	#define SDLK_j				SDLK_J
	#define SDLK_k				SDLK_K
	#define SDLK_l				SDLK_L
	#define SDLK_m				SDLK_M
	#define SDLK_n				SDLK_N
	#define SDLK_o				SDLK_O
	#define SDLK_p				SDLK_P
	#define SDLK_q				SDLK_Q
	#define SDLK_r				SDLK_R
	#define SDLK_s				SDLK_S
	#define SDLK_t				SDLK_T
	#define SDLK_u				SDLK_U
	#define SDLK_v				SDLK_V
	#define SDLK_w				SDLK_W
	#define SDLK_x				SDLK_X
	#define SDLK_y				SDLK_Y
	#define SDLK_z				SDLK_Z
	#define SDLK_BACKQUOTE		SDLK_GRAVE
	#define SDLK_QUOTE			SDLK_APOSTROPHE
	#define SDLK_QUOTEDBL		SDLK_DBLAPOSTROPHE
	#define SDL_TicksType		uint64

	static inline void SDL_ShowCursor(bool show)
	{
		if (show)
			SDL_ShowCursor();
		else
			SDL_HideCursor();
	}

#else
	#define SDL_TicksType		uint32
#endif
