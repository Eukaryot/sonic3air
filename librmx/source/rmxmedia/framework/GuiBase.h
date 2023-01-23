/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
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
	inline const std::list<GuiBase*>& getChildren() const  { return mChildren; }

	void addChild(GuiBase* child);
	void removeChild(GuiBase* child);
	void deleteChild(GuiBase* child);
	void deleteAllChildren();

	template<typename T> T* createChild()
	{
		T* child = new T();
		addChild(child);
		return child;
	}

	void moveToFront(GuiBase* child);
	void moveToBack(GuiBase* child);

	void setRect(const Rectf& rect)		{ mRect = rect; }
	void setRect(float x, float y, float w, float h) { setRect(Rectf(x, y, w, h)); }
	const Rectf& getRect() const		{ return mRect; }

	void setName(const String& value)	{ mName = value; }
	const String& getName() const		{ return mName; }

	virtual void setEnabled(bool value)	{ mEnabled = value; }
	bool isEnabled() const				{ return mEnabled && (mParent != nullptr); }

	virtual void setVisible(bool value)	{ mVisible = value; }
	bool isVisible() const				{ return mVisible && (mParent != nullptr); }

	void setAlpha(float alpha);
	float getAlpha() const  { return mAlpha; }

	virtual void initialize();
	virtual void deinitialize() {}

	virtual void sdlEvent(const SDL_Event& ev);
	virtual void mouse(const rmx::MouseEvent& ev);
	virtual void keyboard(const rmx::KeyboardEvent& ev);
	virtual void textinput(const rmx::TextInputEvent& ev);
	virtual void update(float timeElapsed);
	virtual void render();

protected:
	void updateRealAlpha();

private:
	void internalRemoveChild(GuiBase& child);
	void onIteratingChildrenDone();

protected:
	std::list<GuiBase*> mChildren;
	GuiBase* mParent = nullptr;

	Rectf mRect;
	String mName;

	bool mEnabled = true;
	bool mVisible = true;

	float mAlpha = 1.0f;
	float mRealAlpha = 1.0f;

private:
	std::vector<GuiBase*> mChildrenToRemove;
	bool mIteratingChildren = false;
};
