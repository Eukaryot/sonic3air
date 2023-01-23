/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

#if defined(PLATFORM_ANDROID)

	class AndroidJavaInterface : public SingleInstance<AndroidJavaInterface>
	{
	public:
		enum class BinaryDialogResult
		{
			PENDING,	// No result yet
			FAILED,		// Dialog failed
			SUCCESS		// Dialog successful
		};

		struct RomFileInjection
		{
			BinaryDialogResult mDialogResult = BinaryDialogResult::PENDING;
			std::vector<uint8> mRomContent;
		};

	public:
		bool hasRomFileAlready();
		void openRomFileSelectionDialog();
		inline const RomFileInjection& getRomFileInjection() const  { return mRomFileInjection; }

		void onReceivedRomContent(const uint8* content, size_t bytes);
		void onRomContentSelectionFailed();

	private:
		//rmx::Mutex mMutex;	// TODO: Add this is
		RomFileInjection mRomFileInjection;
	};

#endif
