/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"


namespace lemon
{
	struct DataTypeDefinition;

	struct CompileOptions
	{
		// Options to be set before compilation
		const DataTypeDefinition* mExternalAddressType = &PredefinedDataTypes::UINT_64;
		std::wstring mOutputCombinedSource;
		std::wstring mOutputNativizedSource;
		std::wstring mOutputTranslatedSource;
		bool mConsumeProcessedPragmas = true;

		// Set during compilation
		uint32 mScriptFeatureLevel = 1;
	};

	enum class Keyword : uint8
	{
		_INVALID = 0,
		BLOCK_BEGIN,
		BLOCK_END,
		FUNCTION,
		GLOBAL,
		CONSTANT,
		DEFINE,
		DECLARE,
		RETURN,
		CALL,
		JUMP,
		BREAK,
		CONTINUE,
		IF,
		ELSE,
		WHILE,
		FOR,
		ADDRESSOF
	};

}
