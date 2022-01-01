/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class DrawCommand;

class DrawCollection
{
public:
	~DrawCollection();

	inline const std::vector<DrawCommand*>& getDrawCommands() const  { return mDrawCommands; }

	void clear();
	void addDrawCommand(DrawCommand& drawCommand);
	void addDrawCommand(DrawCommand* drawCommand);

private:
	std::vector<DrawCommand*> mDrawCommands;
};
