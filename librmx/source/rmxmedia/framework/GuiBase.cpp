/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
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
	if (mChildren.front() == &child)	// Child behind all others is the first one in the child list
		return;

	internalRemoveChild(child);
	mChildren.insert(mChildren.begin(), &child);
}

void GuiBase::removeFromParent()
{
	if (nullptr != mParent)
		mParent->removeChild(*this);
}

void GuiBase::initialize()
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->initialize();
	}
	endIteratingChildren();
}

void GuiBase::deinitialize()
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->deinitialize();
	}
	endIteratingChildren();
}

void GuiBase::beginFrame()
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->beginFrame();
	}
	endIteratingChildren();
}

void GuiBase::endFrame()
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->endFrame();
	}
	endIteratingChildren();
}

void GuiBase::sdlEvent(const SDL_Event& ev)
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->sdlEvent(ev);
	}
	endIteratingChildren();
}

void GuiBase::mouse(const rmx::MouseEvent& ev)
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->mouse(ev);
	}
	endIteratingChildren();
}

void GuiBase::keyboard(const rmx::KeyboardEvent& ev)
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->keyboard(ev);
	}
	endIteratingChildren();
}

void GuiBase::textinput(const rmx::TextInputEvent& ev)
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->textinput(ev);
	}
	endIteratingChildren();
}

void GuiBase::update(float deltaSeconds)
{
	beginIteratingChildren();
	for (int k = (int)mChildren.size() - 1; k >= 0; --k)	// Iterate in reverse order
	{
		mChildren[k]->update(deltaSeconds);
	}
	endIteratingChildren();
}

void GuiBase::render()
{
	beginIteratingChildren();
	for (int k = 0; k < (int)mChildren.size(); ++k)			// Iterate in forward order
	{
		mChildren[k]->render();
	}
	endIteratingChildren();
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

void GuiBase::beginIteratingChildren()
{
	mIteratingChildren = true;
}

void GuiBase::endIteratingChildren()
{
	mIteratingChildren = false;

	for (GuiBase* child : mChildrenToRemove)
	{
		internalRemoveChild(*child);
	}
	mChildrenToRemove.clear();
}
