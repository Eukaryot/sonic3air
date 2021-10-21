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
