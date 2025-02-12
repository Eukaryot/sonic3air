/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LogDisplay.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/Utils.h"

#include <lemon/compiler/Compiler.h>
#include <lemon/compiler/TokenHelper.h>
#include <lemon/compiler/TokenManager.h>
#include <lemon/program/GlobalsLookup.h>
#include <lemon/program/Module.h>
#include <lemon/program/Program.h>
#include <lemon/runtime/StandardLibrary.h>
#include <lemon/utility/PragmaSplitter.h>


struct ModuleAppendedInfo : public lemon::Module::AppendedInfo
{
	inline explicit ModuleAppendedInfo(const Mod* mod) : mMod(mod) {}

	const Mod* mMod = nullptr;
};


struct LemonScriptProgram::Internal
{
	lemon::Module mLemonCoreModule;
	lemon::Module mOxygenCoreModule;
	lemon::Module mScriptModule;
	std::vector<lemon::Module*> mModModules;
	std::vector<const Mod*> mLastModSelection;
	lemon::Program mProgram;
	LemonScriptBindings	mLemonScriptBindings;
	lemon::GlobalsLookup mGlobalsLookupCoreOnly;

	Hook mPreUpdateHook;
	Hook mPostUpdateHook;
	LinearLookupTable<Hook, 0x400000, 6, 1024> mAddressHooks;

	inline Internal() :
		mLemonCoreModule("LemonCore"),
		mOxygenCoreModule("OxygenCore"),
		mScriptModule("GameScript")
	{}
};


LemonScriptProgram::LemonScriptProgram() :
	mInternal(*new Internal())
{
	// Register game-specific nativized code
	EngineMain::getDelegate().registerNativizedCode(mInternal.mProgram);

	Configuration& config = Configuration::instance();
	mInternal.mLemonCoreModule.clear();
	mInternal.mOxygenCoreModule.clear();

	// Setup lemon core module -- containing lemonscript standard library
	{
		lemon::Module& module = mInternal.mLemonCoreModule;
		module.startCompiling(mInternal.mGlobalsLookupCoreOnly);
		lemon::StandardLibrary::registerBindings(module);

		// Set preprocessor definitions in core module
		for (const auto& pair : config.mPreprocessorDefinitions.getDefinitions())
		{
			module.addPreprocessorDefinition(pair.second.mIdentifier, pair.second.mValue);
		}
		mInternal.mGlobalsLookupCoreOnly.addDefinitionsFromModule(module);
	}

	// Setup oxygen core module -- with Oxygen Engine specific bindings
	{
		lemon::Module& module = mInternal.mOxygenCoreModule;
		module.startCompiling(mInternal.mGlobalsLookupCoreOnly);
		mInternal.mLemonScriptBindings.registerBindings(module);
		mInternal.mGlobalsLookupCoreOnly.addDefinitionsFromModule(module);
	}

	// Optionally dump the core module script bindings into a generated script file for reference
	if (!config.mDumpCppDefinitionsOutput.empty())
	{
		mInternal.mLemonCoreModule.dumpDefinitionsToScriptFile(config.mDumpCppDefinitionsOutput);
		mInternal.mOxygenCoreModule.dumpDefinitionsToScriptFile(config.mDumpCppDefinitionsOutput, true);
	}
}

LemonScriptProgram::~LemonScriptProgram()
{
	delete &mInternal;
}

LemonScriptBindings& LemonScriptProgram::getLemonScriptBindings()
{
	return mInternal.mLemonScriptBindings;
}

lemon::Program& LemonScriptProgram::getInternalLemonProgram()
{
	return mInternal.mProgram;
}

bool LemonScriptProgram::hasValidProgram() const
{
	return !mInternal.mProgram.getModules().empty();
}

LemonScriptProgram::LoadScriptsResult LemonScriptProgram::loadScripts(const std::string& filename, const LoadOptions& loadOptions)
{
	// Select script mods to load
	std::vector<const Mod*> modsToLoad;
	if (loadOptions.mModuleSelection == LoadOptions::ModuleSelection::ALL_MODS)
	{
		for (const Mod* mod : ModManager::instance().getActiveMods())
		{
			// Is it a script mod?
			const std::wstring mainScriptFilename = mod->mFullPath + L"scripts/main.lemon";
			if (FTX::FileSystem->exists(mainScriptFilename))
			{
				modsToLoad.push_back(mod);
			}
		}
	}

	// Check if there's anything to do at all
	const bool mainScriptReloadNeeded = (mInternal.mProgram.getModules().empty() || loadOptions.mEnforceFullReload);
	if (!mainScriptReloadNeeded)
	{
		if (modsToLoad == mInternal.mLastModSelection)
		{
			// No change
			return LoadScriptsResult::NO_CHANGE;
		}
	}

	Configuration& config = Configuration::instance();
	lemon::GlobalsLookup globalsLookup = mInternal.mGlobalsLookupCoreOnly;	// Copy the definitions from the two core modules

	// Clear program here already - in case compilation fails, it would be broken otherwise
	mInternal.mProgram.clear();

	// Load project's main script module, but only if a full reload is needed
	if (mainScriptReloadNeeded)
	{
		mInternal.mScriptModule.clear();
		const uint32 coreModuleDependencyHash = mInternal.mLemonCoreModule.buildDependencyHash() + mInternal.mOxygenCoreModule.buildDependencyHash();

		// Load scripts
		std::vector<uint8> buffer;
		bool scriptsLoaded = false;

		if (EngineMain::getDelegate().useDeveloperFeatures())
		{
		#ifdef DEBUG
			// Deserialize from cache (so that debug builds don't have to compile themselves, which is quite slow there)
			if (!config.mForceCompileScripts && !config.mCompiledScriptSavePath.empty())
			{
				if (FTX::FileSystem->readFile(config.mCompiledScriptSavePath, buffer))
				{
					VectorBinarySerializer serializer(true, buffer);
					scriptsLoaded = mInternal.mScriptModule.serialize(serializer, globalsLookup, coreModuleDependencyHash, loadOptions.mAppVersion);
					RMX_CHECK(scriptsLoaded, "Failed to deserialize scripts, possibly because the compiled script file '" << WString(config.mCompiledScriptSavePath).toStdString() << "' is using an older format", );
				}
			}
		#endif

			// Compile scripts from the sources, if they're present
			if (!scriptsLoaded)
			{
				if (FTX::FileSystem->exists(filename))
				{
					// Compile module
					scriptsLoaded = loadScriptModule(mInternal.mScriptModule, globalsLookup, *String(filename).toWString());

					// If there are no script functions at all, we consider that a failure
					scriptsLoaded = scriptsLoaded && !mInternal.mScriptModule.getScriptFunctions().empty();

					if (scriptsLoaded)
					{
						if (!config.mCompiledScriptSavePath.empty())
						{
							// Save compiled scripts
							buffer.clear();
							VectorBinarySerializer serializer(false, buffer);
							const bool success = mInternal.mScriptModule.serialize(serializer, globalsLookup, coreModuleDependencyHash, loadOptions.mAppVersion);
							RMX_CHECK(success, "Failed to serialize scripts", );
							FTX::FileSystem->saveFile(config.mCompiledScriptSavePath, buffer);	// In order to use these scripts, they have to be manually moved to the "data" folder
						}
					}
					else
					{
						mInternal.mScriptModule.clear();
					}
				}
			}
		}

		// Deserialize from compiled scripts
		if (!scriptsLoaded && !config.mForceCompileScripts)
		{
			bool loaded = FTX::FileSystem->readFile(L"data/scripts.bin", buffer);
			if (!loaded && !config.mCompiledScriptSavePath.empty())
				loaded = FTX::FileSystem->readFile(config.mCompiledScriptSavePath, buffer);

			if (loaded)
			{
				VectorBinarySerializer serializer(true, buffer);
				scriptsLoaded = mInternal.mScriptModule.serialize(serializer, globalsLookup, coreModuleDependencyHash, loadOptions.mAppVersion);
				RMX_CHECK(scriptsLoaded, "Failed to load 'scripts.bin'", );
			}
		}

		if (scriptsLoaded)
		{
			// If base path was not set yet at this point, scripts were likely loaded from a serialization, and we need to take a guess where the original sources are located
			if (mInternal.mScriptModule.getScriptBasePath().empty())
			{
				// TODO: Make this more generic, and especially less specific for S3AIR
				const std::vector<std::wstring> candidatePaths = { L"./scripts/", L"./bonus/sonic3air_dev/scripts/" };
				for (const std::wstring& candidate : candidatePaths)
				{
					if (FTX::FileSystem->exists(candidate + L"main.lemon"))
					{
						mInternal.mScriptModule.setScriptBasePath(candidate);
						break;
					}
				}
			}
		}
		else
		{
			// Failed to load scripts
			return LoadScriptsResult::FAILED;
		}
	}

	// Load mod script modules
	// TODO: There might be mod script modules already loaded that should stay loaded, i.e. no actual reload is needed for these
	{
		for (lemon::Module* module : mInternal.mModModules)
			delete module;
		mInternal.mModModules.clear();

		if (!modsToLoad.empty())
		{
			lemon::Module* previousModule = &mInternal.mScriptModule;
			for (const Mod* mod : modsToLoad)
			{
				if (nullptr != previousModule)
				{
					globalsLookup.addDefinitionsFromModule(*previousModule);
					previousModule = nullptr;
				}

				// Create and compile module
				lemon::Module* module = new lemon::Module(mod->mUniqueID, new ModuleAppendedInfo(mod));
				const std::wstring mainScriptFilename = mod->mFullPath + L"scripts/main.lemon";
				const bool success = loadScriptModule(*module, globalsLookup, mainScriptFilename);
				if (success)
				{
					mInternal.mModModules.push_back(module);
					previousModule = module;
				}
				else
				{
					delete module;
					break;
				}
			}
		}
	}

	// Build lemon script program from modules
	mInternal.mProgram.addModule(mInternal.mLemonCoreModule);
	mInternal.mProgram.addModule(mInternal.mOxygenCoreModule);
	mInternal.mProgram.addModule(mInternal.mScriptModule);
	for (lemon::Module* module : mInternal.mModModules)
	{
		mInternal.mProgram.addModule(*module);
	}

	// Set script optimization level
	{
		int scriptOptimizationLevel = clamp(config.mScriptOptimizationLevel, 0, 3);
		if (config.mScriptOptimizationLevel < 0)
		{
			// Auto-select script optimization level
			//  -> Use a reduced level for web version, as nativization seems to introduce a bug in S3AIR, when finishing the Blue Spheres special stage
			//  -> However, this only happens in the web version, and is not generally reproducible (it's consistent only for a few people, happens rarely or not at all for others)
		#if defined(PLATFORM_WEB)
			scriptOptimizationLevel = 1;
		#else
			scriptOptimizationLevel = 3;
		#endif
		}
		mInternal.mProgram.setOptimizationLevel(scriptOptimizationLevel);
	}

	// Optional code nativization
	if (config.mRunScriptNativization == 1 && !config.mScriptNativizationOutput.empty())
	{
		mInternal.mProgram.runNativization(mInternal.mScriptModule, config.mScriptNativizationOutput, EmulatorInterface::instance());
		config.mRunScriptNativization = 2;		// Mark as done
	}

	// Scan for function pragmas defining hooks
	evaluateFunctionPragmas();

	if (EngineMain::getDelegate().useDeveloperFeatures())
	{
		// Evaluate defines (only for debugging)
		evaluateDefines();
	}

	mInternal.mLastModSelection.swap(modsToLoad);
	return LoadScriptsResult::PROGRAM_CHANGED;
}

const LemonScriptProgram::Hook* LemonScriptProgram::checkForUpdateHook(bool post)
{
	if (post)
	{
		return (nullptr == mInternal.mPostUpdateHook.mFunction) ? nullptr : &mInternal.mPostUpdateHook;
	}
	else
	{
		return (nullptr == mInternal.mPreUpdateHook.mFunction) ? nullptr : &mInternal.mPreUpdateHook;
	}
}

const LemonScriptProgram::Hook* LemonScriptProgram::checkForAddressHook(uint32 address)
{
	return mInternal.mAddressHooks.find(address);
}

size_t LemonScriptProgram::getNumAddressHooks() const
{
	return mInternal.mAddressHooks.size();
}

std::string_view LemonScriptProgram::getFunctionNameByHash(uint64 hash) const
{
	const auto& list = mInternal.mProgram.getFunctionsByName(hash);
	return list.empty() ? std::string_view() : list[0]->getName().getString();
}

lemon::Variable* LemonScriptProgram::getGlobalVariableByHash(uint64 hash) const
{
	return mInternal.mProgram.getGlobalVariableByName(hash);
}

const Mod* LemonScriptProgram::getModByModule(const lemon::Module& module) const
{
	const ModuleAppendedInfo* appendedInfo = static_cast<ModuleAppendedInfo*>(module.getAppendedInfo());
	return (nullptr != appendedInfo) ? appendedInfo->mMod : nullptr;
}

const std::vector<const lemon::Module*>& LemonScriptProgram::getModules() const
{
	return mInternal.mProgram.getModules();
}

void LemonScriptProgram::resolveLocation(ResolvedLocation& outResolvedLocation, uint32 functionId, uint32 programCounter) const
{
	const lemon::Function* function = mInternal.mProgram.getFunctionByID(functionId);
	if (nullptr == function)
	{
		outResolvedLocation.mScriptFilename = *String(0, "<unknown function id %d>", functionId);
	}
	else
	{
		resolveLocation(outResolvedLocation, *function, programCounter);
	}
}

void LemonScriptProgram::resolveLocation(ResolvedLocation& outResolvedLocation, const lemon::Function& function, uint32 programCounter)
{
	if (function.getType() == lemon::Function::Type::SCRIPT)
	{
		const lemon::ScriptFunction& scriptFunc = static_cast<const lemon::ScriptFunction&>(function);
		if (programCounter < scriptFunc.mOpcodes.size())
		{
			outResolvedLocation.mSourceFileInfo = scriptFunc.mSourceFileInfo;
			outResolvedLocation.mScriptFilename = *WString(scriptFunc.mSourceFileInfo->mFilename).toString();
			outResolvedLocation.mLineNumber = scriptFunc.mOpcodes[programCounter].mLineNumber - scriptFunc.mSourceBaseLineOffset + 1;
		}
		else
		{
			outResolvedLocation.mScriptFilename = *String(0, "<invalid program counter %d in function '%.*s'>", programCounter, function.getName().getString().length(), function.getName().getString().data());
		}
	}
	else
	{
		outResolvedLocation.mScriptFilename = *String(0, "<native function '%.*s'>", function.getName().getString().length(), function.getName().getString().data());
	}
}

bool LemonScriptProgram::loadScriptModule(lemon::Module& module, lemon::GlobalsLookup& globalsLookup, const std::wstring& filename)
{
	try
	{
		while (true)
		{
			const LoadingResult result = tryLoadScriptModule(module, globalsLookup, filename);
			switch (result)
			{
				case LoadingResult::SUCCESS:
					return true;

				default:
				case LoadingResult::FAILED_CONTINUE:
					return false;

				case LoadingResult::FAILED_RETRY:
					break;		// Retry loading by continuing in the loop
			}
		}
	}
	catch (...)
	{
		RMX_ERROR("Script compilation failed due to an unhandled exception", );
	}
	return false;
}

LemonScriptProgram::LoadingResult LemonScriptProgram::tryLoadScriptModule(lemon::Module& module, lemon::GlobalsLookup& globalsLookup, const std::wstring& filename)
{
	// Compile script source
	lemon::CompileOptions options;
	//options.mOutputCombinedSource = L"combined_source.lemon";	// Just for debugging preprocessor issues
	//options.mOutputTranslatedSource = L"output.cpp";			// For testing translation
	lemon::Compiler compiler(module, globalsLookup, options);

	const bool compileSuccess = compiler.loadScript(filename);
	if (compileSuccess || compiler.getErrors().empty())
		return LoadingResult::SUCCESS;

	for (const lemon::Compiler::ErrorMessage& error : compiler.getErrors())
	{
		String errorMessage;
		if (nullptr != error.mSourceFileInfo)
			errorMessage.format("Script error in file '%s', line %d: %s", *WString(error.mSourceFileInfo->mFilename).toString(), error.mError.mLineNumber, error.mMessage.c_str());
		else
			errorMessage.format("Script error: %s", error.mMessage.c_str());
		LogDisplay::instance().addLogError(errorMessage);
	}

	const lemon::Compiler::ErrorMessage& error = compiler.getErrors().front();
	std::string text = error.mMessage + ".";	// Because error messages usually don't end with a dot
	switch (error.mError.mCode)
	{
		case lemon::CompilerError::Code::SCRIPT_FEATURE_LEVEL_TOO_HIGH:
		{
			text += " It's possible the module requires a newer game version.";
			break;
		}
		default:
			break;
	}
	text = "Script compile error:\n" + text + "\n\n";
	if (nullptr == error.mSourceFileInfo ||error.mSourceFileInfo->mFilename.empty())
		text += "Caused in module " + module.getModuleName() + ".";
	else
		text += "Caused in file '" + WString(error.mSourceFileInfo->mFilename).toStdString() + "', line " + std::to_string(error.mError.mLineNumber) + ", of module '" + module.getModuleName() + "'.";

	// Don't show an assert break message box again during debugging, because LEMON_DEBUG_BREAK already brought one up
	if (!rmx::ErrorHandling::isDebuggerAttached())
	{
		// If dev mode is active, use a custom message box to allow for a retry of script loading
		if (!Configuration::instance().mDevMode.mEnabled || nullptr != rmx::ErrorHandling::mMessageBoxImplementation)
		{
			const uint64 hash = 0xb69b7a0fd2ab6355;		// Just some random number instead of calculating an actual hash
			if (!rmx::ErrorHandling::isIgnoringAssertsWithHash(hash))
			{
				text += "\n\n> Retry compilation right away?";
				const rmx::ErrorHandling::MessageBoxInterface::Result result = rmx::ErrorHandling::mMessageBoxImplementation->showMessageBox(rmx::ErrorHandling::MessageBoxInterface::DialogType::YES_NO_CANCEL, rmx::ErrorSeverity::ERROR, text, nullptr, 0);
				switch (result)
				{
					case rmx::ErrorHandling::MessageBoxInterface::Result::ACCEPT:
					{
						// Retry loading (supposed after the user made some fixes before pressing the button)
						return LoadingResult::FAILED_RETRY;
					}

					case rmx::ErrorHandling::MessageBoxInterface::Result::IGNORE:
					{
						// Suppress this message box from now on
						rmx::ErrorHandling::setIgnoreAssertsWithHash(hash, true);
						break;
					}

					default:
						break;
				}
			}
		}
		else
		{
			RMX_ERROR(text, );
		}
	}

	return LoadingResult::FAILED_CONTINUE;
}

void LemonScriptProgram::evaluateFunctionPragmas()
{
	mInternal.mAddressHooks.clear();

	// Go through all functions and have a look at their pragmas
	for (const lemon::ScriptFunction* function : mInternal.mProgram.getScriptFunctions())
	{
		for (const std::string& pragma : function->mPragmas)
		{
			lemon::PragmaSplitter pragmaSplitter(pragma);
			for (const lemon::PragmaSplitter::Entry& entry : pragmaSplitter.mEntries)
			{
				if (entry.mArgument == "update-hook" || entry.mArgument == "pre-update-hook")
				{
					RMX_CHECK(entry.mValue.empty(), "Update hook must not have any value", continue);

					// Create update hook
					Hook& hook = addHook(Hook::Type::PRE_UPDATE, 0);
					hook.mFunction = function;
				}
				else if (entry.mArgument == "post-update-hook")
				{
					RMX_CHECK(entry.mValue.empty(), "Update hook must not have any value", continue);

					// Create update hook
					Hook& hook = addHook(Hook::Type::POST_UPDATE, 0);
					hook.mFunction = function;
				}
			}
		}

		// Create address hooks
		for (uint32 addressHook : function->mAddressHooks)
		{
			Hook& hook = addHook(Hook::Type::ADDRESS, addressHook);
			hook.mFunction = function;
		}
	}
}

void LemonScriptProgram::evaluateDefines()
{
	mGlobalDefines.clear();
	mGlobalDefines.reserve(mInternal.mProgram.getDefines().size() / 2);	// Just a rough guess

	for (const lemon::Define* define : mInternal.mProgram.getDefines())
	{
		const auto& tokens = define->mContent;

		// Check for define with single memory access
		if (tokens.size() >= 4 &&
			tokens[0].isA<lemon::VarTypeToken>() &&
			lemon::isOperator(tokens[1], lemon::Operator::BRACKET_LEFT) &&
			lemon::isOperator(tokens.back(), lemon::Operator::BRACKET_RIGHT))
		{
			// Check for define with fixed address memory access, e.g. "u16[0xffffb000]"
			if (tokens.size() == 4 &&
				tokens[2].isA<lemon::ConstantToken>())
			{
				const uint32 address = (uint32)tokens[2].as<lemon::ConstantToken>().mValue.get<uint64>();
				const lemon::DataTypeDefinition& dataType = *tokens[0].as<lemon::VarTypeToken>().mDataType;
				if (dataType.getClass() == lemon::DataTypeDefinition::Class::INTEGER)
				{
					GlobalDefine& var = vectorAdd(mGlobalDefines);
					var.mName = define->getName();
					var.mAddress = address;
					var.mBytes = (uint8)dataType.getBytes();
					var.mSigned = dataType.as<lemon::IntegerDataType>().mIsSigned;

					const int pos = String(var.mName.getString()).findChar('.', 0, 1);	// Position of the dot
					var.mCategoryHash = (pos == -1) ? 0 : rmx::getMurmur2_64(var.mName.getString().substr(0, pos));
				}
			}
		}
	}

	// Sort alphabetically
	std::sort(mGlobalDefines.begin(), mGlobalDefines.end(), [](const GlobalDefine& a, const GlobalDefine& b) { return a.mName.getString() < b.mName.getString(); } );
}

LemonScriptProgram::Hook& LemonScriptProgram::addHook(Hook::Type type, uint32 address)
{
	Hook* hook = nullptr;
	if (type == Hook::Type::ADDRESS)
	{
		hook = mInternal.mAddressHooks.add(address);
		if (nullptr == hook)
		{
			RMX_ERROR("Invalid address for hook: " << rmx::hexString(address, 6), );
			static Hook dummy;
			return dummy;
		}
		hook->mIndex = (uint32)mInternal.mAddressHooks.size();
	}
	else
	{
		hook = (type == Hook::Type::PRE_UPDATE) ? &mInternal.mPreUpdateHook : &mInternal.mPostUpdateHook;
	}

	hook->mType = type;
	hook->mAddress = address;
	return *hook;
}
