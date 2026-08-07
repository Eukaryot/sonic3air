/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


class GameApp : public GuiBase, public SingleInstance<GameApp>
{
public:
	GameApp();
	~GameApp();

	virtual void initialize() override;
	virtual void deinitialize() override;
	virtual void mouse(const rmx::MouseEvent& ev) override;
	virtual void keyboard(const rmx::KeyboardEvent& ev) override;
	virtual void update(float timeElapsed) override;
	virtual void render() override;
};
