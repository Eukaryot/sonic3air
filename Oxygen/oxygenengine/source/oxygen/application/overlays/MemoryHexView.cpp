/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/application/overlays/MemoryHexView.h"
#include "oxygen/application/video/VideoOut.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/simulation/EmulatorInterface.h"


MemoryHexView::MemoryHexView() :
	mStartAddress(0xffffb000),
	mLines(0)
{
}

MemoryHexView::~MemoryHexView()
{
}

void MemoryHexView::initialize()
{
	setRect(5, 480, 720, 100);

	// Debug output font
	mFont.setSize(15.0f);
	mFont.setShadow(true);
}

void MemoryHexView::deinitialize()
{
}

void MemoryHexView::keyboard(const rmx::KeyboardEvent& ev)
{
	GuiBase::keyboard(ev);

	if (ev.state)
	{
		switch (ev.key)
		{
			case 'i':
			{
				mLines = (mLines == 0) ? 2 : (mLines < 16) ? (mLines * 2) : 0;
				mRect.height = (float)(20 + 20 * mLines);
				break;
			}

			case 'o':
			case 'l':
			{
				const int step = (ev.modifiers & KMOD_LCTRL) ? ((ev.modifiers & KMOD_LSHIFT) ? 0x10000 : 0x1000) : ((ev.modifiers & KMOD_LSHIFT) ? 0x100 : 0x20);
				if (ev.key == 'o')
					mStartAddress -= step;
				else
					mStartAddress += step;
				mStartAddress &= 0xffffff;
				break;
			}
		}
	}
}

void MemoryHexView::update(float timeElapsed)
{
}

void MemoryHexView::render()
{
	GuiBase::render();

	if (mLines > 1)
	{
		Drawer& drawer = EngineMain::instance().getDrawer();

		drawer.drawRect(mRect, Color(0.0f, 0.0f, 0.0f, 0.6f));

		Rectf rect = getRect();
		rect.y += 10;
		for (uint32 y = 0; y < mLines; ++y)
		{
			uint32 address = (mStartAddress + y*16) & 0xffffff;
			if (address >= 0xf00000)
				address |= 0xff000000;

			rect.x = getRect().x + 10;
			drawer.printText(mFont, rect, String(0, "%08x", address));
			rect.x = getRect().x + 132;

			for (uint32 x = 0; x < 16; ++x)
			{
				float brightness = 1.0f - (address % 4) * 0.15f;
				Color color(brightness, brightness, brightness * ((address % 32 < 16) ? 1.0f : 0.5f));

				const uint8 memoryBank = (address >> 16) & 0xff;
				const bool isReadableAddress = (memoryBank < 0x40 || (memoryBank & 0xf0) == 0x80 || memoryBank >= 0xff);
				if (isReadableAddress)
				{
					drawer.printText(mFont, rect, String(0, "%02x", EmulatorInterface::instance().readMemory8(address)), 1, color);
				}
				else
				{
					drawer.printText(mFont, rect, "--", 1, color);
				}

				rect.x += (x % 2) ? 40 : 32;
				++address;
			}

			rect.y += 18 + (y % 2) * 4;
		}

		drawer.performRendering();
	}
}
