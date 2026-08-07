/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	class GlobalsLookup;
	class Module;

	class ModuleSerializer
	{
	public:
		static bool serialize(Module& module, VectorBinarySerializer& outerSerializer, const GlobalsLookup& globalsLookup, uint32 dependencyHash, uint32 appVersion);

	private:
		static void serializeFunctions(Module& module, VectorBinarySerializer& serializer, const GlobalsLookup& globalsLookup);
	};
}
