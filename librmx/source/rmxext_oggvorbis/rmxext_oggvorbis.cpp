/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "../rmxext_oggvorbis.h"


void rmxext_oggvorbis::initialize()
{
	// Initialize audio load callback
	AudioBuffer::LoadCallbackList callbacks;
	callbacks.push_back(OggLoader::staticLoadVorbis);

	AudioBuffer::LoadCallbackList& cblist = AudioBuffer::mStaticLoadCallbacks;
	for (const AudioBuffer::LoadCallbackType callback : callbacks)
	{
		if (!containsElement(cblist, callback))
		{
			cblist.push_back(callback);
		}
	}
}
