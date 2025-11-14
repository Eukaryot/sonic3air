/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/devmode/ImGuiDefinitions.h"

#if defined(SUPPORT_IMGUI)

namespace ImGuiHelpers
{

	static const ImVec4 COLOR_WHITE		  (1.0f, 1.0f, 1.0f, 1.0f);
	static const ImVec4 COLOR_GRAY80	  (0.8f, 0.8f, 0.8f, 1.0f);
	static const ImVec4 COLOR_GRAY60	  (0.6f, 0.6f, 0.6f, 1.0f);
	static const ImVec4 COLOR_GRAY40	  (0.4f, 0.4f, 0.4f, 1.0f);
	static const ImVec4 COLOR_GRAY30	  (0.3f, 0.3f, 0.3f, 1.0f);
	static const ImVec4 COLOR_BLACK		  (0.0f, 0.0f, 0.0f, 1.0f);
	static const ImVec4 COLOR_TRANSPARENT (0.0f, 0.0f, 0.0f, 0.0f);

	static const ImVec4 COLOR_RED		  (1.0f, 0.0f, 0.0f, 1.0f);
	static const ImVec4 COLOR_YELLOW	  (1.0f, 1.0f, 0.0f, 1.0f);
	static const ImVec4 COLOR_LIGHT_YELLOW(1.0f, 1.0f, 0.5f, 1.0f);
	static const ImVec4 COLOR_GREEN		  (0.0f, 1.0f, 0.0f, 1.0f);
	static const ImVec4 COLOR_CYAN		  (0.0f, 1.0f, 1.0f, 1.0f);
	static const ImVec4 COLOR_LIGHT_CYAN  (0.5f, 1.0f, 0.0f, 1.0f);
	static const ImVec4 COLOR_BLUE		  (0.0f, 0.0f, 1.0f, 1.0f);
	static const ImVec4 COLOR_MAGENTA	  (1.0f, 0.0f, 1.0f, 1.0f);


	extern ImTextureRef getTextureRef(DrawerTexture& drawerTexture);


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

}

#endif
