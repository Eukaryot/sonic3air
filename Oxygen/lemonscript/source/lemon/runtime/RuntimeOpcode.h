/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/program/Opcode.h"
#include "lemon/runtime/Runtime.h"


namespace lemon
{
	class Runtime;
	struct RuntimeOpcode;
	struct RuntimeOpcodeBuffer;
	struct RuntimeOpcodeContext;


	typedef void(*ExecFunc)(const RuntimeOpcodeContext context);

	class API_EXPORT RuntimeOpcodeProvider
	{
	public:
		virtual bool buildRuntimeOpcode(RuntimeOpcodeBuffer& buffer, const Opcode* opcodes, int numOpcodesAvailable, int firstOpcodeIndex, int& outNumOpcodesConsumed, const Runtime& runtime) = 0;
	};


	struct API_EXPORT RuntimeOpcodeBase
	{
	public:
		enum class Flag : uint8
		{
			CALL_IS_BASE_CALL		 = 0x20,	// For CALL opcodes only: It is a base call
			CALL_TARGET_RESOLVED	 = 0x40,	// For CALL opcodes only: Call target is already resolved and can be found in the parameter (as pointer)
			CALL_TARGET_RUNTIME_FUNC = 0x80		// For CALL opcodes only: Call target is resolved and is a RuntimeFunction, not a Function
		};

		ExecFunc mExecFunc;
		RuntimeOpcode* mNext = nullptr;
		Opcode::Type mOpcodeType = Opcode::Type::NOP;
		uint8 mSize = 0;
		BitFlagSet<Flag> mFlags;
		uint8 mSuccessiveHandledOpcodes = 0;	// Number of internally handled opcodes (i.e. not manipulating control flow) in a row from this one -- including this one, so if this is 0, the opcode is not handled
	};

	struct API_EXPORT RuntimeOpcode : public RuntimeOpcodeBase
	{
	public:
		static const constexpr size_t PARAMETER_OFFSET = sizeof(RuntimeOpcodeBase);

		template<typename T> FORCE_INLINE T getParameter() const
		{
			return *reinterpret_cast<const T*>((uint8*)this + PARAMETER_OFFSET);
		}

		template<typename T> FORCE_INLINE T getParameter(size_t offset) const
		{
			return *reinterpret_cast<const T*>((uint8*)this + PARAMETER_OFFSET + offset);
		}

		template<typename T> void setParameter(T value)
		{
			*reinterpret_cast<T*>((uint8*)this + PARAMETER_OFFSET) = value;
		}

		template<typename T> void setParameter(T value, size_t offset)
		{
			*reinterpret_cast<T*>((uint8*)this + PARAMETER_OFFSET + offset) = value;
		}
	};

}
