/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	AppFramework
*		Management of the application, incl. input and output.
*/

#pragma once


namespace rmx
{
	struct KeyboardEvent
	{
		int key = 0;
		uint32 scancode = 0;
		uint16 modifiers = 0;
		bool state = false;
		bool repeat = false;
	};

	struct TextInputEvent
	{
		WString text;
	};

	enum class MouseButton
	{
		Left = 0,
		Right,
		Middle,
		Button4,
		Button5
	};

	struct MouseEvent
	{
		MouseButton button = MouseButton::Left;
		bool state = false;
		Vec2i position;
	};


	struct VideoConfig
	{
		enum class Renderer
		{
			SOFTWARE,
			OPENGL
		};

		Recti rect;						// Window rect on screen
		Vec2i startPos;					// Upper left corner
		int bpp = 32;					// Fullscreen only: Color depth bits per pixel
		bool fullscreen = false;		// (Exclusive) fullscreen or window mode?
		bool positioning = false;		// Window mode only: Use "startPos"
		bool resizeable = true;			// Window mode only: Resizable window
		bool noframe = false;			// Window mode only: Hide window frame / borderless window
		bool vsync = true;				// Use vertical sync
		bool hidecursor = false;		// Show or hide mouse cursor
		int multisampling = 0;			// Multisampling setting, usually 0 to disable
		bool autoclearscreen = true;	// Automatically clear screen before rendering
		bool autoswapbuffers = true;	// Automatically swap buffers after rendering
		int iconResource = 0;			// Resource number of icon
		Bitmap iconBitmap;				// Bitmap of icon, as an alternative for "iconSource"
		String iconSource;				// Source file for icon (in PNG format)
		String caption;					// Caption text for window
		Renderer renderer = Renderer::OPENGL;

		VideoConfig();
		VideoConfig(bool fullscr, int wid, int hgt, const String& caption_ = "");
	};


	class InputContext
	{
	public:
		InputContext();
		~InputContext();

		void copy(const InputContext& source);

		void applyEvent(const KeyboardEvent& ev);
		void applyEvent(const MouseEvent& ev);

		inline bool getKeyState(int key) const	{ return (((mKeyState[(key & 0x01ff)/32] >> (key%32)) & 1) != 0); }
		inline bool getKeyChange(int key) const	{ return (((mKeyChange[(key & 0x01ff)/32] >> (key%32)) & 1) != 0); }

		bool getMouseState(int button) const;
		bool getMouseChange(int button) const;

	public:
		uint32 mKeyState[16];
		uint32 mKeyChange[16];
		Vec2i mMousePos;
		Vec2i mMouseRel;
		int   mMouseWheel;
		bool  mMouseState[5];
		bool  mMouseChange[5];
	};
}


namespace FTX
{
	// Video
	int   screenWidth();
	int   screenHeight();
	Vec2i screenSize();
	const Recti& screenRect();
	bool  reshaped();

	// Timing
	float getTimeDifference();
	float getTime();
	float getFramerate();
	int   getFrameCounter();

	// Keyboard
	bool keyState(int key);
	bool keyChange(int key);

	// Mouse
	const Vec2i& mousePos();
	const Vec2i& mouseRel();
	int  mouseWheel();
	bool mouseState(rmx::MouseButton button);
	bool mouseChange(rmx::MouseButton button);
	bool mouseIn(const Recti& rect);
}
