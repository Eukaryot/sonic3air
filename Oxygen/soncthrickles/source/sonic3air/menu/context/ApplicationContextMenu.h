/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxmedia.h>


class ApplicationContextMenu : public GuiBase
{
public:
	ApplicationContextMenu();
	~ApplicationContextMenu();

	virtual void initialize() override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;

private:
	bool mActive = false;
	Vec2i mContextMenuPosition = Vec2i(-1, -1);
	Vec2i mContextMenuClick = Vec2i(-1, -1);
	Vec2i mBaseOuterSize;
	Recti mBaseInnerRect;

	struct Item
	{
		enum class Function
		{
			OPEN_SAVED_DATA_DIRECTORY,
			OPEN_MODS_DIRECTORY,
			OPEN_LOGFILE,
		};

		std::string mText;
		Function mFunction;

		//inline Item(const std::string& text, Function function)
	};
	std::vector<Item> mItems;
};
