/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class GameLoader : public SingleInstance<GameLoader>
{
public:
	enum class State
	{
		UNLOADED,
		WAITING_FOR_ROM,
		ROM_LOADED,
		READY,
		FAILED
	};

	enum class UpdateResult
	{
		CONTINUE,
		CONTINUE_IMMEDIATE,
		SUCCESS,
		FAILURE
	};

public:
	State getState() const  { return mState; }
	bool isLoading() const  { return mState != State::READY; }

	UpdateResult updateLoading();

private:
	State mState = State::UNLOADED;
};
