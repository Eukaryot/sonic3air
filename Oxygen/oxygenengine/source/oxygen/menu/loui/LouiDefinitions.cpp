/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/loui/LouiDefinitions.h"


namespace loui
{
	void BinaryInput::updateState(bool pressed, float deltaSeconds)
	{
		mPrevious = mPressed;
		mPressed = pressed;
		mRepeat = false;
		mConsumed = false;

		if (mPressed)
		{
			if (mPressed != mPrevious)
			{
				mRepeatTimeout = 0.4f;
			}
			else
			{
				mRepeatTimeout -= deltaSeconds;
				if (mRepeatTimeout <= 0.0f)
				{
					mRepeat = true;
					mRepeatTimeout = std::max(mRepeatTimeout + 0.125f, 0.05f);
				}
			}
		}
	}
}
