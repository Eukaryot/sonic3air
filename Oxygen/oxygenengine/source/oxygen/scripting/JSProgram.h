#pragma once

#include "oxygen/scripting/duk/duktape.h"

struct JSProgram
{
	static void Init();
	static void RunScript(const char* script);
	static void RunScripts();

	static duk_context* ctx;
};