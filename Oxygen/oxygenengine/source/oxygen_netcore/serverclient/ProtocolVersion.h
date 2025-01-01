/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace network
{
	// Protocol version compatibility policy:
	//  - Only the maximum version needs to be increased when introducing a change to previously existing packets
	//     (like adding a property or changing the serialization of a property). In that case, all affected packets
	//     need to differentiate between old and new version in their serialization method.
	//  - If a larger change is made that would break compatibility even with the extension of the packet serialization
	//     as described above, the minimum version needs to be set to that new version number as well.

	static const VersionRange<uint8> HIGHLEVEL_PROTOCOL_VERSION_RANGE { 1, 1 };
}
