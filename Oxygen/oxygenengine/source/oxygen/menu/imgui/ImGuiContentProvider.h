/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

class ImGuiContentProvider
{
public:
	virtual ~ImGuiContentProvider() {}
	virtual void buildImGuiContent() = 0;

	inline bool shouldRemoveContentProvider() const  { return mRemoveContentProvider; }

	virtual bool shouldBlockOtherProviders() const  { return false; }

protected:
	bool mRemoveContentProvider = false;	// Set to true as a signal that this instance should be removed
};

#endif
