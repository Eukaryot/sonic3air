/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


template<typename T>
class IDProvider
{
public:
	inline T getNextID()
	{
		const T result = mNextID;
		++mNextID;
		if (mNextID == 0)	// Skip the zero after overflow
			++mNextID;
		return result;
	}

private:
	T mNextID = 1;
};
