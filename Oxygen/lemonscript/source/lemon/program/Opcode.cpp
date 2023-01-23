/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license: see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "lemon/pch.h"
#include "lemon/program/Opcode.h"


namespace lemon
{

	const char* Opcode::GetTypeString(Type type)
	{
		switch (type)
		{
			case Type::NOP:					return "NOP";
			case Type::MOVE_STACK:			return "MOVE_STACK";
			case Type::MOVE_VAR_STACK:		return "MOVE_VAR_STACK";
			case Type::PUSH_CONSTANT:		return "PUSH_CONSTANT";
			case Type::GET_VARIABLE_VALUE:	return "GET_VARIABLE_VALUE";
			case Type::SET_VARIABLE_VALUE:	return "SET_VARIABLE_VALUE";
			case Type::READ_MEMORY:			return "READ_MEMORY";
			case Type::WRITE_MEMORY:		return "WRITE_MEMORY";
			case Type::CAST_VALUE:			return "CAST_VALUE";
			case Type::MAKE_BOOL:			return "MAKE_BOOL";
			case Type::ARITHM_ADD:			return "ARITHM_ADD";
			case Type::ARITHM_SUB:			return "ARITHM_SUB";
			case Type::ARITHM_MUL:			return "ARITHM_MUL";
			case Type::ARITHM_DIV:			return "ARITHM_DIV";
			case Type::ARITHM_MOD:			return "ARITHM_MOD";
			case Type::ARITHM_AND:			return "ARITHM_AND";
			case Type::ARITHM_OR:			return "ARITHM_OR";
			case Type::ARITHM_XOR:			return "ARITHM_XOR";
			case Type::ARITHM_SHL:			return "ARITHM_SHL";
			case Type::ARITHM_SHR:			return "ARITHM_SHR";
			case Type::ARITHM_NEG:			return "ARITHM_NEG";
			case Type::ARITHM_NOT:			return "ARITHM_NOT";
			case Type::ARITHM_BITNOT:		return "ARITHM_BITNOT";
			case Type::COMPARE_EQ:			return "COMPARE_EQ";
			case Type::COMPARE_NEQ:			return "COMPARE_NEQ";
			case Type::COMPARE_LT:			return "COMPARE_LT";
			case Type::COMPARE_LE:			return "COMPARE_LE";
			case Type::COMPARE_GT:			return "COMPARE_GT";
			case Type::COMPARE_GE:			return "COMPARE_GE";
			case Type::JUMP:				return "JUMP";
			case Type::JUMP_CONDITIONAL:	return "JUMP_CONDITIONAL";
			case Type::CALL:				return "CALL";
			case Type::RETURN:				return "RETURN";
			case Type::EXTERNAL_CALL:		return "EXTERNAL_CALL";
			case Type::EXTERNAL_JUMP:		return "EXTERNAL_JUMP";
			default:  return "";
		};
	}

}
