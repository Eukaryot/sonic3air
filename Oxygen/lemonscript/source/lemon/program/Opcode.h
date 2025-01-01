/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/DataType.h"


namespace lemon
{

	struct API_EXPORT Opcode
	{
		enum class Type : uint8
		{
			// Standard opcodes
			NOP,
			MOVE_STACK,
			MOVE_VAR_STACK,
			PUSH_CONSTANT,
			GET_VARIABLE_VALUE,
			SET_VARIABLE_VALUE,
			READ_MEMORY,
			WRITE_MEMORY,
			CAST_VALUE,
			MAKE_BOOL,		// Actually only a kind of cast, but at the moment we don't have a real bool data type
			ARITHM_ADD,
			ARITHM_SUB,
			ARITHM_MUL,
			ARITHM_DIV,
			ARITHM_MOD,
			ARITHM_AND,
			ARITHM_OR,
			ARITHM_XOR,
			ARITHM_SHL,
			ARITHM_SHR,
			ARITHM_NEG,
			ARITHM_NOT,
			ARITHM_BITNOT,
			COMPARE_EQ,
			COMPARE_NEQ,
			COMPARE_LT,
			COMPARE_LE,
			COMPARE_GT,
			COMPARE_GE,
			JUMP,
			JUMP_CONDITIONAL,
			JUMP_SWITCH,
			CALL,
			RETURN,
			EXTERNAL_CALL,
			EXTERNAL_JUMP,

			_NUM_TYPES
		};

		enum class Flag : uint8
		{
			LABEL		= 0x01,		// Opcode is a label target
			JUMP_TARGET = 0x02,		// Opcode is a jump target
			NEW_LINE	= 0x04,		// Start of a new line
			CTRLFLOW	= 0x08,		// Control flow opcode like jump, call, etc.
			JUMP		= 0x10,		// Conditional or unconditional jump (implies FLAG_CTRLFLOW)
			SEQ_BREAK	= 0x20,		// There's a sequence break just after this opcode; this is a result of the other flags of this and the next opcode
			TEMP_FLAG	= 0x80		// Only used temporarily during optimization
		};

		Type mType = Type::NOP;
		BaseType mDataType = BaseType::VOID;
		BitFlagSet<Flag> mFlags;
		uint32 mLineNumber = 0;
		int64 mParameter = 0;	// For constants, or ID in case of variables and calls

	public:
		static const char* GetTypeString(Type type);
	};

}
