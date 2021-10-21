/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"

#include <lemon/program/Program.h>
#include <lemon/runtime/provider/NativizedOpcodeProvider.h>
#include <lemon/runtime/OpcodeExecUtils.h>
#include <lemon/runtime/RuntimeOpcodeContext.h>


namespace lemon
{
	#include "NativizedCode.inc"

#ifndef NATIVIZED_CODE_AVAILABLE
	void createNativizedCodeLookup(Nativizer::LookupDictionary& dict) {}
#endif
}
