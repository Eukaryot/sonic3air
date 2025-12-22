/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/menu/imgui/ImGuiContentProvider.h"

#if defined(SUPPORT_IMGUI)

class ImGuiManager : public SingleInstance<ImGuiManager>
{
public:
	~ImGuiManager();

	void clearProviders();
	void addImGuiContentProvider(uint64 key, int priority, ImGuiContentProvider& provider);
	ImGuiContentProvider* getImGuiContentProvider(uint64 key) const;

	template<typename T>
	T& getOrAddImGuiContentProvider(int priority)
	{
		T* provider = static_cast<T*>(getImGuiContentProvider(T::PROVIDER_KEY));
		if (nullptr == provider)
		{
			provider = new T();
			addImGuiContentProvider(T::PROVIDER_KEY, priority, *provider);
		}
		return *provider;
	}

	template<typename T>
	T* getImGuiContentProvider() const
	{
		return static_cast<T*>(getImGuiContentProvider(T::PROVIDER_KEY));
	}

	void buildAllImGuiContent();

private:
	struct ProviderRegistration
	{
		uint64 mKey = 0;
		int mPriority = 0;
		ImGuiContentProvider* mProvider = nullptr;	// Note that ImGuiManager is considered the owner of the provider and will eventually destroy the provider instance
	};

private:
	std::vector<ProviderRegistration> mProviders;
};

#endif
