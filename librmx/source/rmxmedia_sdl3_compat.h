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
	#undef SDL_FreeSurface

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
	#define SDL_FreeSurface		SDL_DestroySurface

	static inline void SDL_ShowCursor(bool show)
	{
		if (show)
			SDL_ShowCursor();
		else
			SDL_HideCursor();
	}
#endif
