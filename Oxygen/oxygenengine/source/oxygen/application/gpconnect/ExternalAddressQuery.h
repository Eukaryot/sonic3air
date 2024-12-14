/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


class ExternalAddressQuery
{
public:
	std::string mOwnExternalIP;
	uint16 mOwnExternalPort = 0;

	uint64 mQueryID = 0;
};
