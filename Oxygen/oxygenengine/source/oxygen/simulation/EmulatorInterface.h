/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/simulation/DebuggingInterfaces.h"

#include <lemon/runtime/Runtime.h>	// Definition of "lemon::MemoryAccessHandler"

namespace emulatorinterface
{
	struct Internal;
}


class EmulatorInterface : public SingleInstance<EmulatorInterface>, public lemon::MemoryAccessHandler
{
public:
	enum class Register
	{
		D0, D1, D2, D3, D4, D5, D6, D7,
		A0, A1, A2, A3, A4, A5, A6, A7
	};

	struct Watch
	{
		uint32 mAddress;
		uint16 mBytes;
	};

public:
	EmulatorInterface();
	virtual ~EmulatorInterface();

	void clear();
	void applyRomInjections();

	void setDebugNotificationInterface(DebugNotificationInterface* debugNotificationInterface);

	// ROM
	uint32 getRomSize();
	uint8* getRom();

	// RAM
	uint8* getRam();

	// Shared memory
	uint8* getSharedMemory();
	uint64 getSharedMemoryUsage();
	void   clearSharedMemory();

	// General memory access
	bool isValidMemoryRegion(uint32 address, uint32 size);
	virtual uint8* getMemoryPointer(uint32 address, bool writeAccess, uint32 size);

	uint8  readMemory8(uint32 address);
	uint16 readMemory16(uint32 address);
	uint32 readMemory32(uint32 address);
	uint64 readMemory64(uint32 address);

	void writeMemory8(uint32 address, uint8 value);
	void writeMemory16(uint32 address, uint16 value);
	void writeMemory32(uint32 address, uint32 value);
	void writeMemory64(uint32 address, uint64 value);

	void writeMemory8_dev(uint32 address, uint8 value);
	void writeMemory16_dev(uint32 address, uint16 value);
	void writeMemory32_dev(uint32 address, uint32 value);
	void writeMemory64_dev(uint32 address, uint64 value);

	// Registers
	uint32& getRegister(size_t index);
	uint32& getRegister(Register reg);

	// Flags
	bool getFlagZ();
	bool getFlagN();
	void setFlagZ(bool value);
	void setFlagN(bool value);

	// VRAM = Video RAM
	uint8* getVRam();
	uint16 readVRam16(uint16 vramAddress);
	void writeVRam16(uint16 vramAddress, uint16 value);
	void fillVRam(uint16 vramAddress, uint16 fillValue, uint16 bytes);
	void copyFromMemoryToVRam(uint16 vramAddress, uint32 sourceAddress, uint16 bytes);
	BitArray<0x800>& getVRamChangeBits();

	// VSRAM = Vertical scroll RAM
	uint16* getVSRam();

	// SRAM
	size_t loadSRAM(uint32 address, size_t offset, size_t bytes);
	void saveSRAM(uint32 address, size_t offset, size_t bytes);

	// RAM watches
	std::vector<Watch>& getWatches();

public:
	// MemoryAccessHandler interface implementation
	uint8  read8(uint64 address) override	{ return readMemory8((uint32)address); }
	uint16 read16(uint64 address) override	{ return readMemory16((uint32)address); }
	uint32 read32(uint64 address) override	{ return readMemory32((uint32)address); }
	uint64 read64(uint64 address) override	{ return readMemory64((uint32)address); }

	virtual void write8(uint64 address, uint8 value) override	{ writeMemory8((uint32)address, value); }
	virtual void write16(uint64 address, uint16 value) override	{ writeMemory16((uint32)address, value); }
	virtual void write32(uint64 address, uint32 value) override	{ writeMemory32((uint32)address, value); }
	virtual void write64(uint64 address, uint64 value) override	{ writeMemory64((uint32)address, value); }

	virtual void getDirectAccessSpecialization(SpecializationResult& outResult, uint64 address, size_t size, bool writeAccess) override;

protected:
	emulatorinterface::Internal& mInternal;
};


class EmulatorInterfaceDev : public EmulatorInterface
{
public:
	uint8* getMemoryPointer(uint32 address, bool writeAccess, uint32 size) override;

	// MemoryAccessHandler interface implementation
	void write8(uint64 address, uint8 value) override	{ writeMemory8_dev((uint32)address, value); }
	void write16(uint64 address, uint16 value) override	{ writeMemory16_dev((uint32)address, value); }
	void write32(uint64 address, uint32 value) override	{ writeMemory32_dev((uint32)address, value); }
	void write64(uint64 address, uint64 value) override	{ writeMemory64_dev((uint32)address, value); }

	void getDirectAccessSpecialization(SpecializationResult& outResult, uint64 address, size_t size, bool writeAccess) override;
};
