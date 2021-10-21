/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


#include <rmxmedia.h>


class MemoryHexView : public GuiBase
{
public:
	MemoryHexView();
	~MemoryHexView();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

private:
	uint32	mStartAddress;
	uint32	mLines;

	// Misc
	Font	mFont;
};
