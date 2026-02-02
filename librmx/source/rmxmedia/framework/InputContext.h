/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
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
		std::wstring text;
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


	class InputContext
	{
	friend class SystemManager;

	public:
		InputContext();
		~InputContext();

		inline size_t getBitIndex(int key) const { return (key & 0x01ff) + ((key & SDLK_SCANCODE_MASK) >> 21); }
		inline bool getKeyState(int key) const	 { return mKeyState.isBitSet(getBitIndex(key)); }
		inline bool getKeyChange(int key) const	 { return mKeyChange.isBitSet(getBitIndex(key)); }
		inline bool getKeyRepeat(int key) const	 { return mKeyRepeat.isBitSet(getBitIndex(key)); }

		bool getMouseState(int button) const;
		bool getMouseChange(int button) const;
		inline const Vec2i& getMousePos() const  { return mMousePos; }
		inline const Vec2i& getMouseRel() const  { return mMouseRel; }
		inline int getMouseWheel() const		 { return mMouseWheel; }

	private:
		void copy(const InputContext& source);

		void backupState();
		void refreshChangeFlags();

		void applyEvent(const KeyboardEvent& ev);
		void applyEvent(const MouseEvent& ev);
		void applyMousePos(int x, int y);
		void applyMouseWheel(int diff);

	private:
		BitArray<0x400> mKeyState;
		BitArray<0x400> mKeyOldState;
		BitArray<0x400> mKeyChange;
		BitArray<0x400> mKeyRepeat;
		Vec2i mMousePos;
		Vec2i mMouseOldPos;
		Vec2i mMouseRel;
		int mMouseWheel = 0;
		bool mMouseButtonState[5]    = { false };
		bool mMouseButtonOldState[5] = { false };
		bool mMouseButtonChange[5]   = { false };
	};
}
