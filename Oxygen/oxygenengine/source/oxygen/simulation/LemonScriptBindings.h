/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/DebuggingInterfaces.h"

namespace lemon
{
	class Module;
	class Runtime;
}


class LemonScriptBindings
{
public:
	void registerBindings(lemon::Module& module);
	void setDebugNotificationInterface(DebugNotificationInterface* bebugNotificationInterface);
};
