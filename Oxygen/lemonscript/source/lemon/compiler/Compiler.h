/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/compiler/Definitions.h"
#include "lemon/compiler/Errors.h"
#include "lemon/compiler/LineNumberTranslation.h"
#include "lemon/compiler/Preprocessor.h"
#include "lemon/compiler/frontend/TokenProcessing.h"


namespace lemon
{
	class Module;
	class GlobalsLookup;
	class FunctionNode;

	class API_EXPORT Compiler
	{
	public:
		struct ErrorMessage
		{
			std::string mMessage;
			const SourceFileInfo* mSourceFileInfo = nullptr;
			CompilerError mError;
		};

	public:
		static inline Compiler* getActiveInstance()  { return mActiveInstance; }

	public:
		Compiler(Module& module, GlobalsLookup& globalsLookup, const CompileOptions& compileOptions);
		~Compiler();

		bool loadScript(const std::wstring& path);

		void addWarning(CompilerWarning::Code warningCode, std::string_view warningMessage, uint32 lineNumber);

		inline const std::vector<ErrorMessage>& getErrors() const  { return mErrors; }

	private:
		bool loadCodeLines(std::vector<std::string_view>& outLines, const std::wstring& path);
		bool compileLines(const std::vector<std::string_view>& lines);

		bool loadScriptInternal(const std::wstring& basepath, const std::wstring& filename, std::vector<std::string_view>& outLines, std::unordered_set<uint64>& includedPathHashes);
		void runCompilerBackend(std::vector<FunctionNode*>& functionNodes);

	private:
		static inline Compiler* mActiveInstance = nullptr;

	private:
		Module& mModule;
		GlobalsLookup& mGlobalsLookup;
		CompileOptions mCompileOptions;
		LineNumberTranslation mLineNumberTranslation;

		TokenProcessing mTokenProcessing;
		Preprocessor mPreprocessor;

		struct ScriptFile
		{
			std::wstring mBasePath;
			std::wstring mFilename;
			String mContent;
			size_t mFirstLine = 0;
		};
		std::vector<ScriptFile*> mScriptFiles;
		ObjectPool<ScriptFile,64> mScriptFilesPool;

		std::vector<ErrorMessage> mErrors;
	};

}
