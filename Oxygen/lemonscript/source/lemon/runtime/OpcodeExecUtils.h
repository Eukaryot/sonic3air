/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "lemon/runtime/Runtime.h"


namespace lemon
{
	class ControlFlow;

	class OpcodeExecUtils
	{
	public:
		template<typename T> FORCE_INLINE static T safeDivide(T a, T b)		 { return (b == 0) ? 0 : (a / b); }
		template<typename T> FORCE_INLINE static T safeModulo(T a, T b)		 { return (b == 0) ? 0 : (a % b); }

		template<typename T> FORCE_INLINE static T readMemory(ControlFlow& controlFlow, uint64 address) {}
		template<typename T> FORCE_INLINE static void writeMemory(ControlFlow& controlFlow, uint64 address, T value) {}
	};

	template<> FORCE_INLINE float  OpcodeExecUtils::safeDivide(float a, float b)   { return a / b; }
	template<> FORCE_INLINE double OpcodeExecUtils::safeDivide(double a, double b) { return a / b; }
	template<> FORCE_INLINE float  OpcodeExecUtils::safeModulo(float a, float b)   { return std::fmod(a, b); }
	template<> FORCE_INLINE double OpcodeExecUtils::safeModulo(double a, double b) { return std::fmod(a, b); }

	template<> FORCE_INLINE int8   OpcodeExecUtils::readMemory<int8>  (ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read8 (address); }
	template<> FORCE_INLINE int16  OpcodeExecUtils::readMemory<int16> (ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read16(address); }
	template<> FORCE_INLINE int32  OpcodeExecUtils::readMemory<int32> (ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read32(address); }
	template<> FORCE_INLINE int64  OpcodeExecUtils::readMemory<int64> (ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read64(address); }
	template<> FORCE_INLINE uint8  OpcodeExecUtils::readMemory<uint8> (ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read8 (address); }
	template<> FORCE_INLINE uint16 OpcodeExecUtils::readMemory<uint16>(ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read16(address); }
	template<> FORCE_INLINE uint32 OpcodeExecUtils::readMemory<uint32>(ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read32(address); }
	template<> FORCE_INLINE uint64 OpcodeExecUtils::readMemory<uint64>(ControlFlow& controlFlow, uint64 address) { return controlFlow.getMemoryAccessHandler().read64(address); }

	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<int8>  (ControlFlow& controlFlow, uint64 address, int8 value)   { return controlFlow.getMemoryAccessHandler().write8 (address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<int16> (ControlFlow& controlFlow, uint64 address, int16 value)  { return controlFlow.getMemoryAccessHandler().write16(address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<int32> (ControlFlow& controlFlow, uint64 address, int32 value)  { return controlFlow.getMemoryAccessHandler().write32(address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<int64> (ControlFlow& controlFlow, uint64 address, int64 value)  { return controlFlow.getMemoryAccessHandler().write64(address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<uint8> (ControlFlow& controlFlow, uint64 address, uint8 value)  { return controlFlow.getMemoryAccessHandler().write8 (address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<uint16>(ControlFlow& controlFlow, uint64 address, uint16 value) { return controlFlow.getMemoryAccessHandler().write16(address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<uint32>(ControlFlow& controlFlow, uint64 address, uint32 value) { return controlFlow.getMemoryAccessHandler().write32(address, value); }
	template<> FORCE_INLINE void OpcodeExecUtils::writeMemory<uint64>(ControlFlow& controlFlow, uint64 address, uint64 value) { return controlFlow.getMemoryAccessHandler().write64(address, value); }

}
