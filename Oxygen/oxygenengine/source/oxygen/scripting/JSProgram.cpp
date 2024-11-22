#include <filesystem>

#include "oxygen/application/modding/ModManager.h"
#include "oxygen/scripting/JSBindings.h"
#include "oxygen/scripting/JSProgram.h"

duk_context* JSProgram::ctx = nullptr;

duk_ret_t include(duk_context* ctx)
{
	const char* filename = duk_require_string(ctx, 0);

	duk_push_global_stash(ctx);
	duk_get_prop_string(ctx, -1, "base_path");
	const char* base_path = duk_get_string(ctx, -1);
	duk_pop_2(ctx);

	std::filesystem::path base_path_fs(base_path);
	std::filesystem::path full_path = base_path_fs / ("scripts/" + std::string(filename) + ".js");

	std::wstring full_path_wstr = full_path.wstring();

	String script;
	if (!script.loadFile(full_path_wstr))
	{
		std::string errFMT;
		errFMT.append("Could not open script (")
			.append(full_path.string())
			.append(")");

		return duk_error(ctx, DUK_ERR_ERROR, errFMT.c_str());
	}

	JSProgram::RunScript(script.toStdString().c_str());

	return 1;
}


void JSProgram::Init()
{
	if (JSProgram::ctx)
	{
		duk_destroy_heap(JSProgram::ctx);
		JSProgram::ctx = nullptr;
	}

	JSProgram::ctx = duk_create_heap_default();
	if (!JSProgram::ctx)
	{
		RMX_ERROR("ERROR: Failed to initialize interpreter!");
		abort();
	} 
	else
	{
		RMX_LOG_INFO("Created interpreter successfully!");

		duk_push_global_object(JSProgram::ctx);

		// exposed name, native name, arg count
		const duk_function_list_entry global_list[] = {
			{ "include", include, 1 },

			{ NULL, NULL, 0 }
		};

		duk_put_function_list(JSProgram::ctx, -1, global_list);

		JSBindings::Init();
	}
}

void JSProgram::RunScript(const char* script)
{
	if (JSProgram::ctx)
	{
		if (duk_peval_string(JSProgram::ctx, script) != DUK_EXEC_SUCCESS)
		{
			const char* err = duk_safe_to_string(JSProgram::ctx, -1);

			RMX_ERROR(err);
			duk_pop(JSProgram::ctx);
		}
	}
	else
	{
		RMX_ERROR("ERROR: Interpreter context not initialized");
	}
}

std::string wstring_to_string(const std::wstring& wstr)
{
	std::string str;
	str.reserve(wstr.size());

	for (wchar_t wc : wstr)
		str.push_back(static_cast<char>(wc));

	return str;
}

void JSProgram::RunScripts()
{
	JSProgram::Init();

	std::vector<const Mod*> modsToLoad;
	std::vector<uint8> buffer;

	for (const Mod* mod : ModManager::instance().getActiveMods())
	{
		// Is it a script mod?
		const std::wstring mainScriptFilename = mod->mFullPath + L"scripts/main.js";
		if (FTX::FileSystem->exists(mainScriptFilename))
		{
			modsToLoad.push_back(mod);
		}
	}

	for (const Mod* mod : modsToLoad)
	{
		if (!modsToLoad.empty())
		{
			for (const Mod* mod : modsToLoad)
			{
				const std::wstring mainScriptFilename = mod->mFullPath + L"scripts/main.js";
				std::vector<rmx::FileIO::FileEntry> entries;

				duk_push_global_stash(JSProgram::ctx); 
				duk_push_string(JSProgram::ctx, wstring_to_string(mod->mFullPath).c_str());
				duk_put_prop_string(JSProgram::ctx, -2, "base_path"); 
				duk_pop(JSProgram::ctx);

				String script;
				script.loadFile(mainScriptFilename);
				JSProgram::RunScript(script.toStdString().c_str());
			}
		}
	}
}
