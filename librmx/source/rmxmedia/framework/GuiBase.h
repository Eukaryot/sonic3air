/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	GuiBase
*		Base class for GUI elements.
*/

#pragma once

namespace rmx
{
	struct KeyboardEvent;
	struct TextInputEvent;
	struct MouseEvent;
}


class API_EXPORT GuiBase
{
public:
	GuiBase();
	virtual ~GuiBase();

	inline GuiBase* getParent() const  { return mParent; }
	inline const std::vector<GuiBase*>& getChildren() const  { return mChildren; }

	void addChild(GuiBase& child);
	void removeChild(GuiBase& child);
	void deleteChild(GuiBase& child);

	void removeAllChildren();
	void deleteAllChildren();

	template<typename T> T& createChild()
	{
		T& child = *new T();
		addChild(child);
		return child;
	}

	void moveToFront(GuiBase& child);
	void moveToBack(GuiBase& child);

	void removeFromParent();

	void setRect(const Recti& rect)		{ mRect = rect; }
	const Recti& getRect() const		{ return mRect; }

	virtual void setEnabled(bool value)	{ mEnabled = value; }
	bool isEnabled() const				{ return mEnabled && (nullptr != mParent); }

	virtual void setVisible(bool value)	{ mVisible = value; }
	bool isVisible() const				{ return mVisible && (nullptr != mParent); }

	virtual void initialize() {}
	virtual void deinitialize() {}

	virtual void sdlEvent(const SDL_Event& ev);
	virtual void mouse(const rmx::MouseEvent& ev);
	virtual void keyboard(const rmx::KeyboardEvent& ev);
	virtual void textinput(const rmx::TextInputEvent& ev);
	virtual void update(float deltaSeconds);
	virtual void render();

private:
	void internalRemoveChild(GuiBase& child);

	void beginIteratingChildren();
	void endIteratingChildren();

protected:
	Recti mRect;

private:
	std::vector<GuiBase*> mChildren;	// Front children get rendered first, but updated last
	GuiBase* mParent = nullptr;

	std::vector<GuiBase*> mChildrenToRemove;
	bool mIteratingChildren = false;

	bool mEnabled = true;
	bool mVisible = true;
};
