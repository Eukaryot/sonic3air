/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/imgui/ImGuiIntegration.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/Application.h"
#include "oxygen/menu/imgui/ImGuiHelpers.h"

#include "imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"


namespace
{
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
	if (mEnabled == enable)
		return;

	mEnabled = enable;

	// If not inside a frame, shutdown can and should be performed right away
	if (!mInsideFrame && !mEnabled)
	{
		shutdown();
	}
}

void ImGuiIntegration::startup()
{
	RMX_ASSERT(!mRunning, "Startup called while already running");

	if (!mEnabled)
		return;

	// We can choose between the OpenGL renderer and a custom software renderer as fallback
	//  -> I also quickly tried out the SDLRenderer, but that didn't work correctly
	mUsingOpenGL = (FTX::Video->getVideoConfig().mRenderer == rmx::VideoConfig::Renderer::OPENGL);

	// Setup ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// Setup platform / renderer backends
	SDL_Window* window = FTX::Video->getMainWindow();
	if (mUsingOpenGL)
	{
		ImGui_ImplSDL2_InitForOpenGL(window, SDL_GL_GetCurrentContext());
		ImGui_ImplOpenGL3_Init();
	}
	else
	{
		ImGui_ImplSDL2_InitForOther(window);
		mImGuiSoftwareRenderer.initBackend();
	}

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

	mRunning = true;

	// Configure default styles
	loadFont("data/font/ttf/DroidSans.ttf", 15.0f, mDefaultFont);
	refreshImGuiStyle();

#if defined(PLATFORM_ANDROID)
	// Configure SDL to interpret finger taps as mouse clicks as well for mobile devices
	SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "1");
#endif
}

void ImGuiIntegration::shutdown()
{
	if (!mRunning)
		return;

	saveIniSettings();

	SDL_StopTextInput();

	if (mUsingOpenGL)
	{
		ImGui_ImplOpenGL3_Shutdown();
	}
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	mRunning = false;
}

void ImGuiIntegration::processSdlEvent(const SDL_Event& ev)
{
	if (!mRunning)
		return;

	// Apply global screen offset to mouse events
	if (mGlobalScreenOffset != Vec2i())
	{
		switch (ev.type)
		{
			case SDL_MOUSEMOTION:
			{
				SDL_Event newEvent = ev;
				newEvent.motion.x -= mGlobalScreenOffset.x;
				newEvent.motion.y -= mGlobalScreenOffset.y;
				ImGui_ImplSDL2_ProcessEvent(&newEvent);
				return;
			}

			case SDL_MOUSEBUTTONUP:
			case SDL_MOUSEBUTTONDOWN:
			{
				SDL_Event newEvent = ev;
				newEvent.button.x -= mGlobalScreenOffset.x;
				newEvent.button.y -= mGlobalScreenOffset.y;
				ImGui_ImplSDL2_ProcessEvent(&newEvent);
				return;
			}
		}
	}

	// Forward to ImGui backend
	ImGui_ImplSDL2_ProcessEvent(&ev);
}

void ImGuiIntegration::startFrame()
{
	RMX_ASSERT(!mInsideFrame, "Called startFrame while already inside a frame");
	mInsideFrame = true;

	if (!mRunning)
	{
		if (!mEnabled)
			return;

		// Startup now
		startup();
	}

	// Start the ImGui frame
	if (mUsingOpenGL)
	{
		ImGui_ImplOpenGL3_NewFrame();
	}
	else
	{
		mImGuiSoftwareRenderer.newFrame();
	}
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();

	ImGuiHelpers::resetForNextFrame();
}

void ImGuiIntegration::endFrame()
{
	RMX_ASSERT(mInsideFrame, "Called endFrame while not inside a frame");
	mInsideFrame = false;

	if (!mRunning)
		return;

	mGlobalScreenOffset.set(0, 0);

#if defined(PLATFORM_ANDROID)
	// Move all windows up for active text input, if needed
	if (ImGui::GetIO().WantTextInput && ImGuiHelpers::mActiveInputRect != Recti())
	{
		const int maxY = FTX::screenHeight() * 2/5;
		const int diffY = ImGuiHelpers::mActiveInputRect.getEndPos().y - maxY;
		if (diffY > 0)
			mGlobalScreenOffset.y -= diffY;
	}
#endif

	// ImGui rendering
	ImGui::Render();

	if (mUsingOpenGL)
	{
		ImDrawData* drawData = ImGui::GetDrawData();
		drawData->DisplayPos.x -= mGlobalScreenOffset.x;
		drawData->DisplayPos.y -= mGlobalScreenOffset.y;
		ImGui_ImplOpenGL3_RenderDrawData(drawData);
	}
	else
	{
		mImGuiSoftwareRenderer.renderDrawData(mGlobalScreenOffset);
	}

	// Show or hide virtual keyboard on platforms that support it (especially Android)
	if (ImGui::GetIO().WantTextInput)
	{
		Application::instance().requestActiveTextInput();
	}

	// Save ini if there was a change
	if (ImGui::GetIO().WantSaveIniSettings)
	{
		ImGui::GetIO().WantSaveIniSettings = false;
		saveIniSettings();
	}

	if (!mEnabled)
	{
		// Shutdown now
		shutdown();
	}
}

void ImGuiIntegration::onWindowRecreated(bool useOpenGL)
{
	if (!mEnabled)
		return;

	if (mUsingOpenGL != useOpenGL)
	{
		// Shutdown and restart
		shutdown();
		mUsingOpenGL = useOpenGL;
		startup();
	}
}

void ImGuiIntegration::buildContents()
{
	if (!mRunning)
		return;

	// Some checks
	IM_ASSERT(ImGui::GetCurrentContext() != nullptr && "Missing ImGui context. Refer to examples app!");
	IMGUI_CHECKVERSION();

	ImGui::PushFont(mDefaultFont, 0.0f);
	mImGuiManager.buildAllImGuiContent();
	ImGui::PopFont();
}

bool ImGuiIntegration::isCapturingMouse()
{
	return mRunning && ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiIntegration::isCapturingKeyboard()
{
	return mRunning && ImGui::GetIO().WantCaptureKeyboard;
}

bool ImGuiIntegration::hasBlockingImGuiWindow() const
{
	return mRunning && mImGuiManager.hasBlockingProvider();
}

void ImGuiIntegration::refreshImGuiStyle()
{
	if (!mRunning)
		return;

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameBorderSize = 1.0f;

	const float uiScale = Configuration::instance().mDevMode.mUIScale;
	style.FontScaleMain = uiScale;
	style.WindowPadding.x = uiScale * 8.0f;
	style.WindowPadding.y = uiScale * 8.0f;
	style.FramePadding.x = uiScale * 4.0f;
	style.FramePadding.y = uiScale * 3.0f;
	style.FrameRounding = uiScale * 3.0f;
	style.ItemSpacing.x = uiScale * 8.0f;
	style.ItemSpacing.y = uiScale * 5.0f;
	style.ItemInnerSpacing.x = uiScale * 4.0f;
	style.ItemInnerSpacing.y = uiScale * 4.0f;
	style.GrabMinSize = uiScale * 15.0f;
	style.ScrollbarSize = uiScale * 16.0f;
	style.ScrollbarRounding = uiScale * 10.0f;
	style.TabBarBorderSize = std::max(uiScale, 1.0f);
	style.TabRounding = uiScale * 5.0f;
	style.CellPadding.x = uiScale * 4.0f;
	style.CellPadding.y = uiScale * 2.0f;

	// On mobile, increase touch-sensitive areas for buttons and other interactable objects
#if defined(PLATFORM_ANDROID) || defined(PLATFORM_WEB)
	style.TouchExtraPadding.x = uiScale * 4.0f;
	style.TouchExtraPadding.y = uiScale * 4.0f;
#else
	style.TouchExtraPadding.x = 0.0f;
	style.TouchExtraPadding.y = 0.0f;
#endif

	style.Colors[ImGuiCol_Text]					= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_WindowBg]				= ImVec4(0.1f, 0.1f, 0.1f, 0.95f);
	style.Colors[ImGuiCol_Border]				= ImVec4(0.8f, 0.8f, 0.8f, 0.15f);

	style.Colors[ImGuiCol_TitleBg]				= ImGuiHelpers::getAccentColorMix(0.5f, 0.6f, 0.2f);
	style.Colors[ImGuiCol_TitleBgActive]		= ImGuiHelpers::getAccentColorMix(0.6f, 1.0f, 0.3f);
	style.Colors[ImGuiCol_TitleBgCollapsed]		= ImGuiHelpers::getAccentColorMix(0.2f, 0.0f, 0.2f);

	style.Colors[ImGuiCol_FrameBg]				= ImGuiHelpers::getAccentColorMix(0.3f);
	style.Colors[ImGuiCol_FrameBgActive]		= ImGuiHelpers::getAccentColorMix(0.8f);
	style.Colors[ImGuiCol_FrameBgHovered]		= ImGuiHelpers::getAccentColorMix(1.0f);

	style.Colors[ImGuiCol_CheckMark]			= ImGuiHelpers::getAccentColorMix(1.0f, 0.3f, 1.0f);

	style.Colors[ImGuiCol_Button]				= ImGuiHelpers::getAccentColorMix(0.5f, 0.9f, 0.3f);
	style.Colors[ImGuiCol_ButtonActive]			= ImGuiHelpers::getAccentColorMix(0.8f, 0.9f, 0.4f);
	style.Colors[ImGuiCol_ButtonHovered]		= ImGuiHelpers::getAccentColorMix(1.0f, 0.9f, 0.5f);

	style.Colors[ImGuiCol_Header]				= ImGuiHelpers::getAccentColorMix(0.5f, 0.4f, 0.3f);
	style.Colors[ImGuiCol_HeaderActive]			= ImGuiHelpers::getAccentColorMix(0.5f, 0.4f, 0.4f);
	style.Colors[ImGuiCol_HeaderHovered]		= ImGuiHelpers::getAccentColorMix(0.5f, 0.4f, 0.5f);

	style.Colors[ImGuiCol_ResizeGrip]			= ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive]		= ImVec4(0.8f, 0.8f, 0.8f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered]	= ImVec4(1.0f, 1.0f, 1.0f, 1.0f);

	style.Colors[ImGuiCol_SeparatorActive]		= ImVec4(0.6f, 0.6f, 0.6f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered]		= ImVec4(0.8f, 0.8f, 0.8f, 1.0f);

	style.Colors[ImGuiCol_Tab]					= ImGuiHelpers::getAccentColorMix(0.5f, 0.5f, 0.2f);
	style.Colors[ImGuiCol_TabHovered]			= ImGuiHelpers::getAccentColorMix(0.8f, 0.9f, 0.4f);
	style.Colors[ImGuiCol_TabSelected]			= ImGuiHelpers::getAccentColorMix(1.0f, 0.9f, 0.5f);
	style.Colors[ImGuiCol_TabSelectedOverline]	= ImGuiHelpers::getAccentColorMix(1.0f, 0.9f, 0.5f);
}

void ImGuiIntegration::saveIniSettings()
{
	size_t contentSize = 0;
	const char* content = ImGui::SaveIniSettingsToMemory(&contentSize);
	FTX::FileSystem->saveFile(mIniFilePath, content, contentSize);
}

#else

void ImGuiIntegration::setEnabled(bool enable)  {}
void ImGuiIntegration::startup()  {}
void ImGuiIntegration::shutdown()  {}
void ImGuiIntegration::processSdlEvent(const SDL_Event& ev)  {}
void ImGuiIntegration::startFrame()  {}
void ImGuiIntegration::endFrame()  {}
void ImGuiIntegration::onWindowRecreated(bool useOpenGL)  {}
void ImGuiIntegration::buildContents()  {}
bool ImGuiIntegration::isCapturingMouse()  { return false; }
bool ImGuiIntegration::isCapturingKeyboard()  { return false; }
bool ImGuiIntegration::hasBlockingImGuiWindow()  { return false; }
void ImGuiIntegration::refreshImGuiStyle()  {}
void ImGuiIntegration::saveIniSettings()  {}

#endif
