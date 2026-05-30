#include "lemon/program/Program.h"
#include "lemon/runtime/provider/NativizedOpcodeProvider.h"
#include "lemon/runtime/OpcodeExecUtils.h"
#include "lemon/runtime/RuntimeOpcodeContext.h"


namespace lemon
{
	#include "NativizedCode.inc"

#ifndef NATIVIZED_CODE_AVAILABLE
	void createNativizedCodeLookup(Nativizer::LookupDictionary& dict) {}
#endif
}
