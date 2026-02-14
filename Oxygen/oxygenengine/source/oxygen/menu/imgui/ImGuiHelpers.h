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

#include "oxygen/menu/imgui/ImGuiExtensions.h"

struct ImGuiHelpers
{

	static inline const ImVec4 COLOR_WHITE			= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	static inline const ImVec4 COLOR_GRAY80			= ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	static inline const ImVec4 COLOR_GRAY60			= ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	static inline const ImVec4 COLOR_GRAY40			= ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	static inline const ImVec4 COLOR_GRAY30			= ImVec4(0.3f, 0.3f, 0.3f, 1.0f);
	static inline const ImVec4 COLOR_BLACK			= ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	static inline const ImVec4 COLOR_TRANSPARENT	= ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	static inline const ImVec4 COLOR_RED			= ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
	static inline const ImVec4 COLOR_YELLOW	 		= ImVec4(1.0f, 1.0f, 0.0f, 1.0f);
	static inline const ImVec4 COLOR_LIGHT_YELLOW	= ImVec4(1.0f, 1.0f, 0.5f, 1.0f);
	static inline const ImVec4 COLOR_GREEN			= ImVec4(0.0f, 1.0f, 0.0f, 1.0f);
	static inline const ImVec4 COLOR_CYAN			= ImVec4(0.0f, 1.0f, 1.0f, 1.0f);
	static inline const ImVec4 COLOR_LIGHT_CYAN		= ImVec4(0.5f, 1.0f, 0.0f, 1.0f);
	static inline const ImVec4 COLOR_BLUE			= ImVec4(0.0f, 0.0f, 1.0f, 1.0f);
	static inline const ImVec4 COLOR_MAGENTA		= ImVec4(1.0f, 0.0f, 1.0f, 1.0f);


	static void resetForNextFrame();

	static ImTextureRef getTextureRef(DrawerTexture& drawerTexture);

	static ImVec4 getAccentColorMix(float accent, float saturation = 1.0f, float grayValue = 0.3f);


	struct ScopedIndent
	{
	public:
		inline ScopedIndent() :
			mIndent(12.0f * Configuration::instance().mDevMode.mUIScale)
		{
			ImGui::Indent(mIndent);
		}

		inline explicit ScopedIndent(float indent, bool applyUIScale = true) :
			mIndent(indent)
		{
			if (applyUIScale)
				mIndent *= Configuration::instance().mDevMode.mUIScale;
			ImGui::Indent(mIndent);
		}

		inline ~ScopedIndent()
		{
			ImGui::Unindent(mIndent);
			ImGui::Spacing();
		}

	private:
		float mIndent;
	};


	struct InputString
	{
		char mInternal[256] = { 0 };

		InputString() = default;
		InputString(std::string_view str) { set(str); };

		inline bool isEmpty() const  { return (mInternal[0] == 0); }
		inline std::string_view get() const  { return std::string_view(mInternal); }
		void set(std::string_view str);
	};


	struct WideInputString
	{
		WString mWideString;
		char mInternalUTF8[256] = { 0 };

		inline bool isEmpty() const  { return mWideString.empty(); }
		inline const WString& get() const  { return mWideString; }
		void set(std::wstring_view str);
		void refreshFromInternal();
	};


	static inline Recti mActiveInputRect = Recti();

	static bool InputText(const char* label, char* buf, size_t buf_size, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	static bool InputText(const char* label, ImGuiHelpers::InputString& inputString, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);
	static bool InputText(const char* label, ImGuiHelpers::WideInputString& inputString, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void* user_data = nullptr);


	struct FilterString
	{
		char mString[256] = { 0 };

		bool draw();
		bool shouldInclude(std::string_view str) const;
	};


	struct OpenCodeLocation
	{
		static bool drawButton();
		static bool drawButton(const std::wstring& path, int lineNumber);
		static bool open(const std::wstring& path, int lineNumber);
	};
};

#endif
