#include "oxygen/application/modding/ModManager.h"
#include "oxygen/scripting/JSBindings.h"
#include "oxygen/scripting/JSProgram.h"

duk_context* JSProgram::ctx = nullptr;

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
		JSBindings::Init();
	}
}

std::vector<std::string> splitString(const std::string& str, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream tokenStream(str);
	while (std::getline(tokenStream, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
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

void JSProgram::RunScripts(const char* path)
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

				String path;
				path.loadFile(mainScriptFilename);
				JSProgram::RunScript(path.toStdString().c_str());
			}
		}
	}
}
