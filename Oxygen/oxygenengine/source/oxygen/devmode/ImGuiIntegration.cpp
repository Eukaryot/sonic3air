/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/devmode/ImGuiIntegration.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/devmode/DevModeMainWindow.h"

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"


namespace
{
	DevModeMainWindow* mDevModeMainWindow = nullptr;
	ImFont* mDefaultFontx1 = nullptr;
	ImFont* mDefaultFontx2 = nullptr;
	std::wstring mIniFilePath;

	bool loadFont(const char* filename, float size, ImFont*& outFont)
	{
		outFont = nullptr;
		std::vector<uint8> content;
		if (!FTX::FileSystem->readFile(filename, content))
		{
			RMX_ERROR("Could not find font file: '" << filename << "'", );
		}
		else
		{
			uint8* buffer = new uint8[content.size()];
			memcpy(buffer, &content[0], (int)content.size());
			outFont = ImGui::GetIO().Fonts->AddFontFromMemoryTTF(buffer, (int)content.size(), size, nullptr, ImGui::GetIO().Fonts->GetGlyphRangesDefault());
		}
		return (nullptr != outFont);
	}
}


void ImGuiIntegration::setEnabled(bool enable)
{
	mEnabled = enable;
}

void ImGuiIntegration::startup()
{
	RMX_ASSERT(!mRunning, "Startup called while already running");

	if (!mEnabled)
		return;

	// Only OpenGL renderer is supported
	//  -> I also quickly tried out the SDLRenderer, but that didn't work correctly
	if (FTX::Video->getVideoConfig().mRenderer != rmx::VideoConfig::Renderer::OPENGL)
		return;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup platform / renderer backends
	SDL_Window* window = FTX::Video->getMainWindow();
	ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
	ImGui_ImplOpenGL3_Init();

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	// Configure paths
	{
		ImGui::GetIO().IniFilename = nullptr;	// Disable automatic load & save by ImGui, we're handling that ourselves

		const std::wstring iniBasePath = Configuration::instance().mAppDataPath + L"devmode/";
		mIniFilePath = iniBasePath + L"imgui.ini";

		if (!FTX::FileSystem->exists(iniBasePath))
		{
			FTX::FileSystem->createDirectory(iniBasePath);
		}
		else
		{
			std::vector<uint8> content;
			if (FTX::FileSystem->readFile(mIniFilePath, content) && !content.empty())
			{
				ImGui::LoadIniSettingsFromMemory((const char*)&content[0], content.size());
			}
		}
	}

	updateFontScale();

	mRunning = true;

	// Configure default styles
	loadFont("data/font/ttf/DroidSans.ttf", 15.0f, mDefaultFontx1);
	loadFont("data/font/ttf/DroidSans.ttf", 30.0f, mDefaultFontx2);
	refreshImGuiStyle();

	mDevModeMainWindow = new DevModeMainWindow();

#if defined(PLATFORM_ANDROID)
	// Configure SDL to interpret finger taps as mouse clicks as well for mobile devices
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#endif
}

void ImGuiIntegration::shutdown()
{
	if (!mRunning)
		return;

	SAFE_DELETE(mDevModeMainWindow);

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	mRunning = false;
}

void ImGuiIntegration::processSdlEvent(const SDL_Event& ev)
{
	if (!mRunning)
		return;

	// Forward to ImGui backend
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

void ImGuiIntegration::startFrame()
{
	if (!mRunning)
		return;

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
}

void ImGuiIntegration::showDebugWindow()
{
	if (!mRunning)
		return;

	// Some checks
	IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing Dear ImGui context. Refer to examples app!");
	IMGUI_CHECKVERSION();

	ImFont* font = (Configuration::instance().mDevMode.mUIScale <= 1.0f) ? mDefaultFontx1 : mDefaultFontx2;
	ImGui::PushFont(font);
	mDevModeMainWindow->buildWindow();
	ImGui::PopFont();
}

void ImGuiIntegration::endFrame()
{
	if (!mRunning)
		return;

	// ImGui rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Save ini if there was a change
	if (ImGui::GetIO().WantSaveIniSettings)
	{
		ImGui::GetIO().WantSaveIniSettings = false;
		size_t contentSize = 0;
		const char* content = ImGui::SaveIniSettingsToMemory(&contentSize);
		FTX::FileSystem->saveFile(mIniFilePath, content, contentSize);
	}
}

void ImGuiIntegration::onWindowRecreated(bool useOpenGL)
{
	if (!mEnabled)
		return;

	if (mRunning && !useOpenGL)
	{
		// OpenGL got deactivated, we need to shut down without it
		shutdown();
	}
	else if (!mRunning && useOpenGL)
	{
		// Start again now that OpenGL is available
		startup();
	}
}

bool ImGuiIntegration::isCapturingMouse()
{
	return mRunning && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiIntegration::isCapturingKeyboard()
{
	return mRunning && ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiIntegration::refreshImGuiStyle()
{
	if (!mRunning)
		return;

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;
	style.FrameRounding = 3.0f;
	style.ItemSpacing.y = 5.0f;

	Color accentColor = Configuration::instance().mDevMode.mUIAccentColor;
	const auto GetAccentColorMix = [&](float accent, float saturation = 1.0f, float grayValue = 0.3f)
	{
		return ImVec4(interpolate(grayValue, accentColor.x * accent, saturation), interpolate(grayValue, accentColor.y * accent, saturation), interpolate(grayValue, accentColor.z * accent, saturation), 1.0f);
	};

	style.Colors[ImGuiCol_Text]					= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_WindowBg]				= ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
	style.Colors[ImGuiCol_Border]				= ImVec4(0.8f, 0.8f, 0.8f, 0.15f);

	style.Colors[ImGuiCol_TitleBg]				= GetAccentColorMix(0.5f, 0.6f, 0.2f);
	style.Colors[ImGuiCol_TitleBgActive]		= GetAccentColorMix(0.6f, 1.0f, 0.3f);
	style.Colors[ImGuiCol_TitleBgCollapsed]		= GetAccentColorMix(0.2f, 0.0f, 0.2f);

	style.Colors[ImGuiCol_FrameBg]				= GetAccentColorMix(0.3f);
	style.Colors[ImGuiCol_FrameBgActive]		= GetAccentColorMix(0.8f);
	style.Colors[ImGuiCol_FrameBgHovered]		= GetAccentColorMix(1.0f);

	style.Colors[ImGuiCol_CheckMark]			= GetAccentColorMix(1.0f, 0.3f, 1.0f);

	style.Colors[ImGuiCol_Button]				= GetAccentColorMix(0.5f, 0.9f, 0.3f);
	style.Colors[ImGuiCol_ButtonActive]			= GetAccentColorMix(0.8f, 0.9f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered]		= GetAccentColorMix(1.0f, 0.9f, 0.5f);

	style.Colors[ImGuiCol_Header]				= GetAccentColorMix(0.5f, 0.4f, 0.3f);
	style.Colors[ImGuiCol_HeaderActive]			= GetAccentColorMix(0.5f, 0.4f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered]		= GetAccentColorMix(0.5f, 0.4f, 0.5f);

	style.Colors[ImGuiCol_ResizeGrip]			= ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive]		= ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered]	= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	style.Colors[ImGuiCol_SeparatorActive]		= ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered]		= ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

	style.Colors[ImGuiCol_Tab]					= GetAccentColorMix(0.5f, 0.5f, 0.2f);
	style.Colors[ImGuiCol_TabHovered]			= GetAccentColorMix(0.8f, 0.9f, 0.4f);
	style.Colors[ImGuiCol_TabSelected]			= GetAccentColorMix(1.0f, 0.9f, 0.5f);
	style.Colors[ImGuiCol_TabSelectedOverline]	= GetAccentColorMix(1.0f, 0.9f, 0.5f);
}

void ImGuiIntegration::updateFontScale()
{
	const float uiScale = Configuration::instance().mDevMode.mUIScale;
	const float fontSize = (uiScale <= 1.0f) ? 1.0f : 2.0f;
	ImGui::GetIO().FontGlobalScale = uiScale / fontSize;
}

void ImGuiIntegration::toggleMainWindow()
{
	if (mRunning && nullptr != mDevModeMainWindow)
	{
		mDevModeMainWindow->setIsWindowOpen(!mDevModeMainWindow->getIsWindowOpen());
	}
}

#else

void ImGuiIntegration::setEnabled(bool enable)  {}
void ImGuiIntegration::startup()  {}
void ImGuiIntegration::shutdown()  {}
void ImGuiIntegration::processSdlEvent(const SDL_Event& ev)  {}
void ImGuiIntegration::startFrame()  {}
void ImGuiIntegration::showDebugWindow()  {}
void ImGuiIntegration::endFrame()  {}
void ImGuiIntegration::onWindowRecreated(bool useOpenGL)  {}
bool ImGuiIntegration::isCapturingMouse()  { return false; }
bool ImGuiIntegration::isCapturingKeyboard()  { return false; }
void ImGuiIntegration::refreshImGuiStyle()  {}
void ImGuiIntegration::toggleMainWindow()  {}

#endif
