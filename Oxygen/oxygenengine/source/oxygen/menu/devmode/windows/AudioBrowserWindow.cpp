/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/AudioBrowserWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/menu/imgui/ImGuiHelpers.h"
#include "oxygen/application/EngineMain.h"
#include "oxygen/application/audio/AudioOutBase.h"


AudioBrowserWindow::AudioBrowserWindow() :
	DevModeWindowBase("Audio Browser", Category::MISC, ImGuiWindowFlags_AlwaysAutoResize)
{
}

void AudioBrowserWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(350.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(500.0f, 250.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	AudioOutBase& audioOut = EngineMain::instance().getAudioOut();
	AudioCollection& audioCollection = AudioCollection::instance();

	ImGui::SliderFloat("Master Volume", &Configuration::instance().mAudio.mMasterVolume, 0.0f, 1.0f, "%.2f");

	if (ImGui::Button("Reload Modded Audio"))
	{
		mPlayingAudio.stop();
		audioOut.reloadAudioCollection();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	// Refresh if there's any changes
	if (mLastAudioCollectionChangeCounter != audioCollection.getChangeCounter())
	{
		mAudioDefinitions.clear();
		const auto& audioDefinitions = audioCollection.getAudioDefinitions();
		for (const auto& [key, audioDefinition] : audioDefinitions)
		{
			mAudioDefinitions.emplace_back(&audioDefinition);
		}

		std::sort(mAudioDefinitions.begin(), mAudioDefinitions.end(),
			[](const AudioCollection::AudioDefinition* a, const AudioCollection::AudioDefinition* b) { return a->mKeyString < b->mKeyString; });

		mLastAudioCollectionChangeCounter = audioCollection.getChangeCounter();
	}

	// TODO: Cache filter results
	static ImGuiHelpers::FilterString filterString;
	filterString.draw();

	if (ImGui::BeginTable("Audio Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY, ImVec2(0.0f, 200.0f * uiScale)))
	{
		ImGui::TableSetupColumn("Identifier");
		ImGui::TableSetupColumn("Play");
		ImGui::TableSetupColumn("Type");

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (const AudioCollection::AudioDefinition* audioDefinition : mAudioDefinitions)
		{
			if (!filterString.shouldInclude(audioDefinition->mKeyString))
				continue;

			const ImVec4 textColor = (audioDefinition->mActiveSource->mPackage != AudioCollection::Package::MODDED) ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f) : ImVec4(0.5f, 1.0f, 1.0f, 1.0f);

			ImGui::PushID(audioDefinition);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::TextColored(textColor, "%s", audioDefinition->mKeyString.c_str());

			ImGui::TableSetColumnIndex(1);
			if (mPlayingAudio.valid() && mPlayingDefinition == audioDefinition)	// TODO: This does not work yet
			{
				if (ImGui::Button("Stop"))
				{
					mPlayingAudio.stop();
				}
			}
			else
			{
				if (ImGui::ArrowButton("Play", ImGuiDir_Right))
				{
					mPlayingAudio.stop();

					const bool isSound = (audioDefinition->mType != AudioCollection::AudioDefinition::Type::SOUND);
					if (!isSound)
					{
						audioOut.getAudioPlayer().stopAllSoundsByChannel(0);
					}

					//mPlayingAudio = ...
					audioOut.playAudioBase(audioDefinition->mKeyId, AudioOutBase::CONTEXT_MENU + (isSound? 0 : AudioOutBase::CONTEXT_MUSIC));
					mPlayingDefinition = audioDefinition;
				}
			}

			ImGui::TableSetColumnIndex(2);
			const char* typeString = "unknown";
			switch (audioDefinition->mActiveSource->mType)
			{
				case AudioCollection::SourceRegistration::Type::FILE:					typeString = "Ogg File";  break;
				case AudioCollection::SourceRegistration::Type::EMULATION_BUFFERED:		typeString = "Emulation (buffered)";  break;
				case AudioCollection::SourceRegistration::Type::EMULATION_CONTINUOUS:	typeString = "Emulation (continuous)";  break;
				case AudioCollection::SourceRegistration::Type::EMULATION_DIRECT:		typeString = "Emulation (unbuffered)";  break;
			}
			ImGui::TextColored(textColor, "%s", typeString);

			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

#endif
