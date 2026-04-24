/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class Drawer;


namespace loui
{
	class Container;
	class FontWrapper;
	class Widget;


	struct BinaryInput
	{
		bool mPressed = false;
		bool mPrevious = false;
		bool mConsumed = false;

		inline bool isPressed() const	  { return mPressed; }
		inline bool hasChanged() const	  { return mPressed != mPrevious; }
		inline bool justPressed() const   { return mPressed && !mPrevious; }
		inline bool justReleased() const  { return !mPressed && mPrevious; }

		inline void updateState(bool pressed)  { mPrevious = mPressed; mPressed = pressed; mConsumed = false; }
		inline void consume()  { mPressed = false; mConsumed = true; }
	};


	struct UpdateInfo
	{
		float mDeltaSeconds = 0.0f;

		// Mouse related input data (also used for touch input)
		BinaryInput mLeftMouseButton;
		Vec2i mMousePosition;
		int mMouseWheel = 0;
		bool mMousePosConsumed = false;
		bool mMouseWheelConsumed = false;

		// Keyboard / controller related input data
		BinaryInput mButtonUp;
		BinaryInput mButtonDown;
		BinaryInput mButtonLeft;
		BinaryInput mButtonRight;
		BinaryInput mButtonA;
		BinaryInput mButtonB;
		BinaryInput mButtonX;
		BinaryInput mButtonY;
	};


	struct RenderInfo
	{
		Drawer& mDrawer;
		bool mIsVisible = true;
		bool mIsInteractable = true;
		float mOpacity = 1.0f;
	};


	struct Borders
	{
		int mTop = 0;
		int mBottom = 0;
		int mLeft = 0;
		int mRight = 0;
	};
}
