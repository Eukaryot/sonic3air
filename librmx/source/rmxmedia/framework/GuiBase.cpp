/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


GuiBase::GuiBase()
{
}

GuiBase::~GuiBase()
{
	deleteAllChildren();
	if (nullptr != mParent)
		mParent->removeChild(*this);
}

void GuiBase::addChild(GuiBase& child)
{
	GuiBase* oldParent = child.mParent;
	if (oldParent)
	{
		if (oldParent == this)
			return;
		oldParent->removeChild(child);
	}

	mChildren.push_back(&child);
	child.mParent = this;
	child.initialize();
}

void GuiBase::removeChild(GuiBase& child)
{
	if (child.mParent != this)
		return;

	child.deinitialize();
	child.mParent = nullptr;

	if (mIteratingChildren)
	{
		mChildrenToRemove.push_back(&child);
	}
	else
	{
		internalRemoveChild(child);
	}
}

void GuiBase::deleteChild(GuiBase& child)
{
	removeChild(child);
	delete &child;
}

void GuiBase::removeAllChildren()
{
	for (GuiBase* child : mChildren)
	{
		child->deinitialize();
		child->mParent = nullptr;
	}
	mChildren.clear();
}

void GuiBase::deleteAllChildren()
{
	for (GuiBase* child : mChildren)
	{
		child->deinitialize();
		child->mParent = nullptr;
		delete child;
	}
	mChildren.clear();
}

void GuiBase::moveToFront(GuiBase& child)
{
	RMX_ASSERT(child.mParent == this, "Element to move to front is not a child");
	if (mChildren.back() == &child)		// Child in front of all others is the last one in the child list
		return;

	internalRemoveChild(child);
	mChildren.push_back(&child);
}

void GuiBase::moveToBack(GuiBase& child)
{
	RMX_ASSERT(child.mParent == this, "Element to move to back is not a child");
	if (mChildren.front() != &child)	// Child behind all others is the first one in the child list
		return;

	internalRemoveChild(child);
	mChildren.insert(mChildren.begin(), &child);
}

void GuiBase::removeFromParent()
{
	if (nullptr != mParent)
		mParent->removeChild(*this);
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

void GuiBase::update(float deltaSeconds)
{
	if (!mEnabled || !mVisible)
		return;

	mIteratingChildren = true;
	for (GuiBase* child : mChildren)
	{
		child->update(deltaSeconds);
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
