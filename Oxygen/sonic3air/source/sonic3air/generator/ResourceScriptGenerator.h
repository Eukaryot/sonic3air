/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>

class CodeExec;


struct ResourceScriptGenerator
{
	static void generateLevelObjectTableScript(CodeExec& codeExec);
	static void generateLevelRingsTableScript(CodeExec& codeExec);

	static void convertLevelObjectsBinToScript(const std::wstring& inputFilename, const std::wstring& outputFilename, int objectTableNumber = 1);
	static void convertLevelRingsBinToScript(const std::wstring& inputFilename, const std::wstring& outputFilename);
};
