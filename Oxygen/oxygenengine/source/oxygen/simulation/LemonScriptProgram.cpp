/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/LemonScriptProgram.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/application/modding/ModManager.h"
#include "oxygen/helper/Utils.h"

#include <lemon/compiler/Compiler.h>
#include <lemon/compiler/TokenManager.h>
#include <lemon/program/GlobalsLookup.h>
#include <lemon/program/Module.h>
#include <lemon/program/Program.h>


namespace
{

	bool isOperatorToken(const lemon::Token& token, lemon::Operator op)
	{
		return (token.getType() == lemon::Token::Type::OPERATOR) && (token.as<lemon::OperatorToken>().mOperator == op);
	}


	// TODO: Move this into a helper!
	class PragmaSplitter
	{
	public:
		struct Entry
		{
			std::string mArgument;
			std::string mValue;
		};
		std::vector<Entry> mEntries;

	public:
		PragmaSplitter(const std::string& input)
		{
			size_t pos = 0;
			while (pos < input.length())
			{
				// Skip all spaces
				while (pos < input.length() && input[pos] == 32)
					++pos;

				if (pos < input.length())
				{
					const size_t startPos = pos;
					while (pos < input.length() && input[pos] != 32)
						++pos;

					mEntries.emplace_back();

					// Split part string into argument and value
					const std::string part = input.substr(startPos, pos - startPos);
					const size_t left = part.find_first_of('(');
					if (left != std::string::npos)
					{
						RMX_CHECK(part.back() == ')', "No matching parentheses in pragma found", continue);

						mEntries.back().mArgument = part.substr(0, left);
						mEntries.back().mValue = part.substr(left + 1, part.length() - left - 2);
					}
					else
					{
						mEntries.back().mArgument = part;
					}
				}
			}
		}
	};

}


struct LemonScriptProgram::Internal
{
	lemon::Module mCoreModule;
	lemon::Module mScriptModule;
	std::vector<lemon::Module*> mModModules;
	std::vector<const Mod*> mLastModSelection;
	lemon::Program mProgram;
	LemonScriptBindings	mLemonScriptBindings;

	Hook mPreUpdateHook;
	Hook mPostUpdateHook;
	LinearLookupTable<Hook, 0x400000, 6, 1024> mAddressHooks;

	inline Internal() :
		mCoreModule("OxygenCore"),
		mScriptModule("GameScript")
	{}
};


LemonScriptProgram::LemonScriptProgram() :
	mInternal(*new Internal())
{
	mInternal.mCoreModule.clear();

	// Register game-specific nativized code
	EngineMain::getDelegate().registerNativizedCode(mInternal.mProgram);
}

LemonScriptProgram::~LemonScriptProgram()
{
	delete &mInternal;
}

void LemonScriptProgram::startup()
{
	Configuration& config = Configuration::instance();

	// Register script bindings (user-defined variables and functions)
	//  -> TODO: This has some dependency of the runtime, as the register variables directly access the emulator interface instance;
	//           and this in turn is the reason why there's a need for this separate startup function at all...
	mInternal.mLemonScriptBindings.registerBindings(mInternal.mCoreModule);

	// Set preprocessor definitions in core module
	for (const auto& pair : config.mPreprocessorDefinitions.getDefinitions())
	{
		mInternal.mCoreModule.addPreprocessorDefinition(pair.second.mIdentifier, pair.second.mValue);
	}

	// Optionally dump the core module script bindings into a generated script file for reference
	if (!config.mDumpCppDefinitionsOutput.empty())
	{
		mInternal.mCoreModule.dumpDefinitionsToScriptFile(config.mDumpCppDefinitionsOutput);
	}
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

bool LemonScriptProgram::loadScriptModule(lemon::Module& module, lemon::GlobalsLookup& globalsLookup, const std::wstring& filename)
{
	try
	{
		// Compile script source
		lemon::Compiler::CompileOptions options;
		//options.mOutputCombinedSource = L"combined_source.lemon";	// Just for debugging preprocessor issues
		//options.mOutputTranslatedSource = L"output.cpp";			// For testing translation
		lemon::Compiler compiler(module, globalsLookup, options);

		const bool compileSuccess = compiler.loadScript(filename);
		if (!compileSuccess && !compiler.getErrors().empty())
		{
			for (const lemon::Compiler::ErrorMessage& error : compiler.getErrors())
			{
				LogDisplay::instance().addLogError(String(0, "Compile error: %s (in '%s', line %d)", error.mMessage.c_str(), *WString(error.mFilename).toString(), error.mError.mLineNumber));
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
			if (error.mFilename.empty())
				text += "Caused in module " + module.getModuleName() + ".";
			else
				text += "Caused in file '" + WString(error.mFilename).toStdString() + "', line " + std::to_string(error.mError.mLineNumber) + ", of module '" + module.getModuleName() + "'.";
			RMX_ERROR(text, );
			return false;
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
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
	lemon::GlobalsLookup globalsLookup;
	globalsLookup.addDefinitionsFromModule(mInternal.mCoreModule);

	// Clear program here already - in case compilation fails, it would be broken otherwise
	mInternal.mProgram.clear();

	// Load project's main script module, but only if a full reload is needed
	if (mainScriptReloadNeeded)
	{
		mInternal.mScriptModule.clear();

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
					scriptsLoaded = mInternal.mScriptModule.serialize(serializer);
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
					RMX_CHECK(scriptsLoaded, "Failed to compile scripts from source at '" << filename << "'", );

					if (scriptsLoaded && !config.mCompiledScriptSavePath.empty())
					{
						// Save compiled scripts
						VectorBinarySerializer serializer(false, buffer);
						const bool success = mInternal.mScriptModule.serialize(serializer);
						RMX_CHECK(success, "Failed to serialize scripts", );
						FTX::FileSystem->saveFile(config.mCompiledScriptSavePath, buffer);	// In order to use these scripts, they have to be manually moved to the "data" folder
					}
				}
			}
		}

		// Deserialize from compiled scripts
		if (!scriptsLoaded && !config.mForceCompileScripts)
		{
			if (FTX::FileSystem->readFile(L"data/scripts.bin", buffer))
			{
				VectorBinarySerializer serializer(true, buffer);
				scriptsLoaded = mInternal.mScriptModule.serialize(serializer);
				RMX_CHECK(scriptsLoaded, "Failed to load 'scripts.bin'", );
			}
		}

		if (!scriptsLoaded)
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
				lemon::Module* module = new lemon::Module(mod->mName);
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
	mInternal.mProgram.addModule(mInternal.mCoreModule);
	mInternal.mProgram.addModule(mInternal.mScriptModule);
	for (lemon::Module* module : mInternal.mModModules)
	{
		mInternal.mProgram.addModule(*module);
	}

	mInternal.mProgram.setOptimizationLevel(config.mScriptOptimizationLevel);

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
		return (mInternal.mPostUpdateHook.mFunction == nullptr) ? nullptr : &mInternal.mPostUpdateHook;
	}
	else
	{
		return (mInternal.mPreUpdateHook.mFunction == nullptr) ? nullptr : &mInternal.mPreUpdateHook;
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

void LemonScriptProgram::evaluateFunctionPragmas()
{
	mInternal.mAddressHooks.clear();

	// Go through all functions and have a look at their pragmas
	for (const lemon::ScriptFunction* function : mInternal.mProgram.getScriptFunctions())
	{
		for (const std::string& pragma : function->mPragmas)
		{
			PragmaSplitter splitter(pragma);
			for (size_t index = 0; index < splitter.mEntries.size(); ++index)
			{
				const auto& entry = splitter.mEntries[index];
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
				else if (entry.mArgument == "address-hook" || entry.mArgument == "translated")
				{
					RMX_CHECK(!entry.mValue.empty(), "Address hook must have a value", continue);
					const uint32 address = (uint32)rmx::parseInteger(entry.mValue);

					// You can use "translated" to denote that some code was already put into script, but should not be an actual address hook
					if (entry.mArgument == "address-hook")
					{
						// Create address hook
						Hook& hook = addHook(Hook::Type::ADDRESS, address);
						hook.mFunction = function;
					}
				}
			}
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
			tokens[0].getType() == lemon::Token::Type::VARTYPE &&
			isOperatorToken(tokens[1], lemon::Operator::BRACKET_LEFT) &&
			isOperatorToken(tokens.back(), lemon::Operator::BRACKET_RIGHT))
		{
			// Check for define with fixed address memory access, e.g. "u16[0xffffb000]"
			if (tokens.size() == 4 &&
				tokens[2].getType() == lemon::Token::Type::CONSTANT)
			{
				const uint32 address = (uint32)tokens[2].as<lemon::ConstantToken>().mValue;
				const lemon::DataTypeDefinition& dataType = *tokens[0].as<lemon::VarTypeToken>().mDataType;
				if (dataType.mClass == lemon::DataTypeDefinition::Class::INTEGER)
				{
					GlobalDefine& var = vectorAdd(mGlobalDefines);
					var.mName = define->getName();
					var.mAddress = address;
					var.mBytes = (uint8)dataType.mBytes;
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

std::string_view LemonScriptProgram::getFunctionNameByHash(uint64 hash) const
{
	const auto& list = mInternal.mProgram.getFunctionsByName(hash);
	return list.empty() ? std::string_view() : list[0]->getName().getString();
}

lemon::Variable* LemonScriptProgram::getGlobalVariableByHash(uint64 hash) const
{
	return mInternal.mProgram.getGlobalVariableByName(hash);
}

void LemonScriptProgram::resolveLocation(uint32 functionId, uint32 programCounter, std::string& scriptFilename, uint32& lineNumber) const
{
	const lemon::Function* function = mInternal.mProgram.getFunctionByID(functionId);
	if (nullptr == function)
	{
		scriptFilename = *String(0, "<unknown function id %d>", functionId);
	}
	else
	{
		resolveLocation(*function, programCounter, scriptFilename, lineNumber);
	}
}

void LemonScriptProgram::resolveLocation(const lemon::Function& function, uint32 programCounter, std::string& scriptFilename, uint32& lineNumber)
{
	if (function.getType() == lemon::Function::Type::SCRIPT)
	{
		const lemon::ScriptFunction& scriptFunc = static_cast<const lemon::ScriptFunction&>(function);
		if (programCounter < scriptFunc.mOpcodes.size())
		{
			scriptFilename = *WString(scriptFunc.mSourceFileInfo->mFilename).toString();
			lineNumber = scriptFunc.mOpcodes[programCounter].mLineNumber - scriptFunc.mSourceBaseLineOffset + 1;
		}
		else
		{
			scriptFilename = *String(0, "<invalid program counter %d in function '%.*s'>", programCounter, function.getName().getString().length(), function.getName().getString().data());
		}
	}
	else
	{
		scriptFilename = *String(0, "<user-defined function '%.*s'>", function.getName().getString().length(), function.getName().getString().data());
	}
}
