/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/imgui/ImGuiManager.h"
#include "oxygen/menu/imgui/ImGuiIntegration.h"

#if defined(SUPPORT_IMGUI)

ImGuiManager::~ImGuiManager()
{
	clearProviders();
}

void ImGuiManager::clearProviders()
{
	for (ProviderRegistration& reg : mProviders)
	{
		delete reg.mProvider;
	}
	mProviders.clear();
	mHasBlockingProvider = false;

	// ImGui is not needed any more now
	ImGuiIntegration::instance().setEnabled(false);
}

void ImGuiManager::addImGuiContentProvider(uint64 key, int priority, ImGuiContentProvider& provider)
{
	RMX_ASSERT(nullptr == getImGuiContentProvider(key), "ImGui content provider key " << rmx::hexString(key, 16) << " is already in use");

	ProviderRegistration& reg = vectorAdd(mProviders);
	reg.mKey = key;
	reg.mPriority = priority;
	reg.mProvider = &provider;

	// Sort to ensure that higher priorities go first
	std::sort(mProviders.begin(), mProviders.end(), [](const ProviderRegistration& a, const ProviderRegistration& b) { return a.mPriority > b.mPriority; } );

	// Make sure that ImGui will get started, if it's not already running
	ImGuiIntegration::instance().setEnabled(true);
}

ImGuiContentProvider* ImGuiManager::getImGuiContentProvider(uint64 key) const
{
	for (const ProviderRegistration& reg : mProviders)
	{
		if (reg.mKey == key)
			return reg.mProvider;
	}
	return nullptr;
}

void ImGuiManager::buildAllImGuiContent()
{
	mHasBlockingProvider = false;

	for (size_t index = 0; index < mProviders.size(); ++index)
	{
		ProviderRegistration& reg = mProviders[index];

		// Build content
		reg.mProvider->buildImGuiContent();

		mHasBlockingProvider = reg.mProvider->shouldBlockOtherProviders();

		// Handle the case that the provider wants to be removed
		if (reg.mProvider->shouldRemoveContentProvider())
		{
			delete reg.mProvider;
			mProviders.erase(mProviders.begin() + index);
			--index;

			if (mProviders.empty())
			{
				ImGuiIntegration::instance().setEnabled(false);
			}
		}

		if (mHasBlockingProvider)
			break;
	}
}

#endif
