/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/menu/devmode/windows/AudioPlaybackWindow.h"

#if defined(SUPPORT_IMGUI)

#include "oxygen/application/audio/AudioPlayer.h"
#include "oxygen/menu/imgui/ImGuiHelpers.h"


AudioPlaybackWindow::AudioPlaybackWindow() :
	DevModeWindowBase("Audio Playback", Category::MISC, 0)
{
}

void AudioPlaybackWindow::buildContent()
{
	ImGui::SetWindowPos(ImVec2(600.0f, 10.0f), ImGuiCond_FirstUseEver);
	ImGui::SetWindowSize(ImVec2(600.0f, 200.0f), ImGuiCond_FirstUseEver);

	const float uiScale = getUIScale();

	AudioPlayer& audioPlayer = AudioPlayer::instance();

	if (ImGui::CollapsingHeader("Rows"))
	{
		ImGuiHelpers::ScopedIndent si;

		ImGui::Checkbox("Position", &mShowPosition);
		ImGui::SameLine();
		ImGui::Checkbox("Channel", &mShowChannel);
		ImGui::SameLine();
		ImGui::Checkbox("Context", &mShowContext);

		ImGui::Checkbox("Volume", &mShowVolume);
		ImGui::SameLine();
		ImGui::Checkbox("Panning", &mShowPanning);
	}

	const int numRows = 2 + (int)mShowPosition + (int)mShowChannel + (int)mShowContext + (int)mShowVolume + (int)mShowPanning;

	if (ImGui::BeginTable("Audio Playback", numRows, ImGuiTableFlags_Borders))
	{
		ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40 * uiScale);
		ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthStretch, 2.0f);
		if (mShowPosition)
			ImGui::TableSetupColumn("Position");
		if (mShowChannel)
			ImGui::TableSetupColumn("Channel");
		if (mShowContext)
			ImGui::TableSetupColumn("Context");
		if (mShowVolume)
			ImGui::TableSetupColumn("Volume");
		if (mShowPanning)
			ImGui::TableSetupColumn("Panning");

		ImGui::TableSetupScrollFreeze(0, 1);
		ImGui::TableHeadersRow();

		for (size_t k = 0; k < audioPlayer.getNumPlayingSounds(); ++k)
		{
			AudioPlayer::PlayingSoundRef ref = audioPlayer.getPlayingSoundByIndex(k);
			if (!audioPlayer.isValidPlayingSound(ref))
				continue;

			ImGui::PushID(ref.mUniqueId);

			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::Text("%u", ref.mUniqueId);

			ImGui::TableSetColumnIndex(1);
			const std::string_view audioKey = audioPlayer.getPlayingSoundAudioKey(ref);
			ImGui::Text("%.*s", (int)audioKey.length(), audioKey.data());

			if (mShowPosition)
			{
				ImGui::TableNextColumn();
				ImGui::Text("%0.2f sec%s", audioPlayer.getPlayingSoundPosition(ref), audioPlayer.isPlayingSoundPaused(ref) ? " (paused)" : "");
			}

			if (mShowChannel)
			{
				ImGui::TableNextColumn();
				ImGui::Text("0x%02x", audioPlayer.getPlayingSoundChannel(ref));
			}

			if (mShowContext)
			{
				ImGui::TableNextColumn();
				ImGui::Text("0x%02x", audioPlayer.getPlayingSoundContext(ref));
			}

			if (mShowVolume)
			{
				ImGui::TableNextColumn();
				ImGui::Text("%0.3f", audioPlayer.getPlayingSoundVolume(ref));
			}

			if (mShowPanning)
			{
				ImGui::TableNextColumn();
				ImGui::Text("%0.2f", audioPlayer.getPlayingSoundPanning(ref));
			}

			ImGui::PopID();
		}
		ImGui::EndTable();
	}
}

#endif
