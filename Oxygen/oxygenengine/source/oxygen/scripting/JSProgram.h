#pragma once

#include "oxygen/scripting/duk/duktape.h"

struct JSProgram
{
	static void Init();
	static void RunScript(const char* script);
	static void RunScripts(const char* path);

	static duk_context* ctx;
};