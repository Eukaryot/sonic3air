/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/DrawCollection.h"
#include "oxygen/drawing/DrawCommand.h"


DrawCollection::~DrawCollection()
{
	clear();
}

void DrawCollection::clear()
{
	for (DrawCommand* drawCommand : mDrawCommands)
	{
		DrawCommand::mFactory.destroy(*drawCommand);
	}
	mDrawCommands.clear();
}

void DrawCollection::addDrawCommand(DrawCommand& drawCommand)
{
	mDrawCommands.push_back(&drawCommand);
}

void DrawCollection::addDrawCommand(DrawCommand* drawCommand)
{
	mDrawCommands.push_back(drawCommand);
}
