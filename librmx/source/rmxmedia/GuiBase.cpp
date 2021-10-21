/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxmedia.h"


GuiBase::GuiBase()
{
}

GuiBase::~GuiBase()
{
	deleteAllChildren();
	if (nullptr != mParent)
		mParent->removeChild(this);
}

void GuiBase::initialize()
{
	updateRealAlpha();
}

void GuiBase::addChild(GuiBase* child)
{
	assert(child);

	GuiBase* oldParent = child->mParent;
	if (oldParent)
	{
		if (oldParent == this)
			return;
		oldParent->removeChild(child);
	}

	mChildren.push_back(child);
	child->mParent = this;
	child->initialize();
}

void GuiBase::removeChild(GuiBase* child)
{
	assert(child);
	if (child->mParent != this)
		return;

	child->deinitialize();
	child->mParent = nullptr;

	if (mIteratingChildren)
	{
		mChildrenToRemove.push_back(child);
	}
	else
	{
		internalRemoveChild(*child);
	}
}

void GuiBase::deleteChild(GuiBase* child)
{
	removeChild(child);
	delete child;
}

void GuiBase::deleteAllChildren()
{
	for (GuiBase* child : mChildren)
	{
		child->mParent = nullptr;
		delete child;
	}
	mChildren.clear();
}

void GuiBase::moveToFront(GuiBase* child)
{
	assert(child);
	assert(child->mParent == this);

	if (mChildren.back() == child)		// Child in front of all others is the last one in the child list
		return;

	mChildren.remove(child);
	mChildren.push_back(child);
}

void GuiBase::moveToBack(GuiBase* child)
{
	assert(child);
	assert(child->mParent == this);

	if (mChildren.front() != child)		// Child ehind all others is the first one in the child list
		return;

	mChildren.remove(child);
	mChildren.push_front(child);
}

void GuiBase::setAlpha(float alpha)
{
	mAlpha = clamp(alpha, 0.0f, 1.0f);
	updateRealAlpha();
}

void GuiBase::updateRealAlpha()
{
	mRealAlpha = mAlpha;
	if (nullptr != mParent)
		mRealAlpha *= mParent->mRealAlpha;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->updateRealAlpha();
	}
	onIteratingChildrenDone();
}


void GuiBase::sdlEvent(const SDL_Event& ev)
{
	if (!mEnabled)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->sdlEvent(ev);
	}
	onIteratingChildrenDone();
}

void GuiBase::mouse(const rmx::MouseEvent& ev)
{
	if (!mEnabled)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->mouse(ev);
	}
	onIteratingChildrenDone();
}

void GuiBase::keyboard(const rmx::KeyboardEvent& ev)
{
	if (!mEnabled)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->keyboard(ev);
	}
	onIteratingChildrenDone();
}

void GuiBase::textinput(const rmx::TextInputEvent& ev)
{
	if (!mEnabled)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->textinput(ev);
	}
	onIteratingChildrenDone();
}

void GuiBase::update(float timeElapsed)
{
	if (!mEnabled || !mVisible)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->update(timeElapsed);
	}
	onIteratingChildrenDone();
}

void GuiBase::render()
{
	if (!mVisible)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->render();
	}
	onIteratingChildrenDone();
}


void GuiBase::internalRemoveChild(GuiBase& child)
{
	for (auto it = mChildren.begin(); it != mChildren.end(); ++it)
	{
		if (*it == &child)
		{
			mChildren.erase(it);
			break;
		}
	}
}

void GuiBase::onIteratingChildrenDone()
{
	mIteratingChildren = false;
	for (GuiBase* child : mChildrenToRemove)
	{
		internalRemoveChild(*child);
	}
	mChildrenToRemove.clear();
}
