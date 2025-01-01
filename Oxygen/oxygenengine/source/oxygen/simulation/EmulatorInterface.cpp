/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/application/Configuration.h"
#include "oxygen/application/GameProfile.h"
#include "oxygen/resources/RawDataCollection.h"
#include "oxygen/resources/ResourcesCache.h"


namespace emulatorinterface
{

	struct Internal : public RuntimeMemory
	{
	public:
		// Debugging
		std::vector<EmulatorInterface::Watch> mWatches;
		DebugNotificationInterface* mDebugNotificationInterface = nullptr;

	public:
		FORCE_INLINE bool isValidMemoryRegion(uint32 address, uint32 size)
		{
			address &= 0x00ffffff;
			if (address >= 0xff0000)
			{
				return ((address & 0x00ffff) + size <= sizeof(mRam));
			}
			else if (address < 0x400000)
			{
				return (address + size <= sizeof(mRom));
			}
			else if (address >= 0x800000 && address < 0x9000000)
			{
				return ((address & 0x0fffff) + size <= sizeof(mSharedMemory));
			}
			return false;
		}

		// Modes for "accessMemory" (i.e. possible template parameter values)
		#define MEMORY_MODE_READ      0		// Read access
		#define MEMORY_MODE_WRITE     1		// Write access without debugging support
		#define MEMORY_MODE_WRITE_DEV 2		// Write access with debugging support

		template<int MODE>
		FORCE_INLINE uint8* accessMemory(uint32 address, uint32 size)
		{
			address &= 0x00ffffff;
			if (address >= 0xff0000)
			{
				if (MODE == MEMORY_MODE_WRITE_DEV && !mWatches.empty())
					checkWatches(address, size);
				address &= 0x00ffff;
				RMX_CHECK(address + size <= sizeof(mRam), "Too large memory " << (MODE == MEMORY_MODE_READ ? "read" : "write") << " access of " << rmx::hexString(size) << " bytes at RAM address " << rmx::hexString(0xffff0000 + address), RMX_REACT_THROW);
				return &mRam[address];
			}
			else if (address < 0x400000)
			{
				RMX_CHECK(address + size <= sizeof(mRom), "Too large memory " << (MODE == MEMORY_MODE_READ ? "read" : "write") << " access of " << rmx::hexString(size) << " bytes at ROM address " << rmx::hexString(address, 6), RMX_REACT_THROW);
				return &mRom[address];
			}
			else if (address >= 0x800000 && address < 0x900000)
			{
				if (MODE == MEMORY_MODE_WRITE_DEV && !mWatches.empty())
					checkWatches(address, size);
				address &= 0x0fffff;
				RMX_CHECK(address + size <= sizeof(mSharedMemory), "Too large memory " << (MODE == MEMORY_MODE_READ ? "read" : "write") << " access of " << rmx::hexString(size) << " bytes at shared memory address " << rmx::hexString(0x800000 + address, 6), RMX_REACT_THROW);
				if (MODE != MEMORY_MODE_READ)
				{
					if ((address & 0x3fff) + size <= 0x4000)
						mSharedMemoryUsage |= (1ull << uint64(address >> 14));
					else
						mSharedMemoryUsage |= (3ull << uint64(address >> 14));	// Assmuming size is smaller than 16 KB
				}
				return &mSharedMemory[address];
			}
			else if (address >= 0xa00000 && address < 0xd00000)
			{
				//if ((address & 0xfffff0) == 0xc00000)
				//	_asm nop;
				static uint64 dummy;
				dummy = 0;
				return (uint8*)&dummy;
			}
			else
			{
				RMX_ERROR("Invalid memory access at " << rmx::hexString(address, 6) << " of " << rmx::hexString(size) << " bytes", RMX_REACT_THROW);
				return nullptr;
			}
		}

	private:
		FORCE_INLINE void checkWatches(uint32 address, uint16 bytes)
		{
			if (nullptr != mDebugNotificationInterface)
			{
				address &= 0x00ffffff;
				for (size_t i = 0; i < mWatches.size(); ++i)
				{
					const EmulatorInterface::Watch& watch = mWatches[i];
					if (address + bytes > watch.mAddress && address < watch.mAddress + watch.mBytes)
					{
						mDebugNotificationInterface->onWatchTriggered(i, address, bytes);
					}
				}
			}
		}
	};

}


void RuntimeMemory::clear()
{
	// Reset ROM to unmodified version
	{
		const std::vector<uint8>& unmodifiedROM = ResourcesCache::instance().getUnmodifiedRom();
		memcpy(mRom, &unmodifiedROM[0], unmodifiedROM.size());
		if (sizeof(mRom) > unmodifiedROM.size())
			memset(&mRom[unmodifiedROM.size()], 0, sizeof(mRom) - unmodifiedROM.size());
	}

	memset(mRam, 0, sizeof(mRam));
	memset(mVRam, 0, sizeof(mVRam));
	mVRamChangeBits.setAllBits();		// Count all VRAM as changed
	memset(mSharedMemory, 0, sizeof(mSharedMemory));
	mSharedMemoryUsage = 0;
	memset(mRegisters, 0, sizeof(mRegisters));
	mRegisters[15] = GameProfile::instance().mAsmStackRange.second;   // Initialization of A7 (just leaving it 0 is no good idea)
}

void RuntimeMemory::applyRomInjections()
{
	RawDataCollection::instance().applyRomInjections(mRom, sizeof(mRom));
}


EmulatorInterface::EmulatorInterface() :
	mInternal(*new emulatorinterface::Internal())
{
}

EmulatorInterface::~EmulatorInterface()
{
	delete &mInternal;
}

void EmulatorInterface::clear()
{
	mInternal.clear();
}

void EmulatorInterface::applyRomInjections()
{
	mInternal.applyRomInjections();
}

void EmulatorInterface::setDebugNotificationInterface(DebugNotificationInterface* debugNotificationInterface)
{
	mInternal.mDebugNotificationInterface = debugNotificationInterface;
}

RuntimeMemory& EmulatorInterface::getRuntimeMemory()
{
	return mInternal;
}

uint32 EmulatorInterface::getRomSize()
{
	return sizeof(mInternal.mRom);
}

uint8* EmulatorInterface::getRom()
{
	return mInternal.mRom;
}

uint8* EmulatorInterface::getRam()
{
	return mInternal.mRam;
}

uint8* EmulatorInterface::getSharedMemory()
{
	return mInternal.mSharedMemory;
}

uint64 EmulatorInterface::getSharedMemoryUsage()
{
	return mInternal.mSharedMemoryUsage;
}

void EmulatorInterface::clearSharedMemory()
{
	memset(mInternal.mSharedMemory, 0, sizeof(mInternal.mSharedMemory));
	mInternal.mSharedMemoryUsage = 0;
}

bool EmulatorInterface::isValidMemoryRegion(uint32 address, uint32 size)
{
	return mInternal.isValidMemoryRegion(address, size);
}

uint8* EmulatorInterface::getMemoryPointer(uint32 address, bool writeAccess, uint32 size)
{
	if (writeAccess)
		return mInternal.accessMemory<MEMORY_MODE_WRITE>(address, size);
	else
		return mInternal.accessMemory<MEMORY_MODE_READ>(address, size);
}

uint8 EmulatorInterface::readMemory8(uint32 address)
{
	return *mInternal.accessMemory<MEMORY_MODE_READ>(address, 1);
}

uint16 EmulatorInterface::readMemory16(uint32 address)
{
	const uint8* pointer = mInternal.accessMemory<MEMORY_MODE_READ>(address, 2);
	return rmx::readMemoryUnalignedSwapped<uint16>(pointer);
}

uint32 EmulatorInterface::readMemory32(uint32 address)
{
	const uint8* pointer = mInternal.accessMemory<MEMORY_MODE_READ>(address, 4);
	return rmx::readMemoryUnalignedSwapped<uint32>(pointer);
}

uint64 EmulatorInterface::readMemory64(uint32 address)
{
	const uint8* pointer = mInternal.accessMemory<MEMORY_MODE_READ>(address, 8);
	return rmx::readMemoryUnalignedSwapped<uint64>(pointer);
}

void EmulatorInterface::writeMemory8(uint32 address, uint8 value)
{
	*mInternal.accessMemory<MEMORY_MODE_WRITE>(address, 1) = value;
}

void EmulatorInterface::writeMemory16(uint32 address, uint16 value)
{
	uint16* mem = (uint16*)mInternal.accessMemory<MEMORY_MODE_WRITE>(address, 2);
	*mem = swapBytes16(value);
}

void EmulatorInterface::writeMemory32(uint32 address, uint32 value)
{
	uint32* mem = (uint32*)mInternal.accessMemory<MEMORY_MODE_WRITE>(address, 4);
	*mem = swapBytes32(value);
}

void EmulatorInterface::writeMemory64(uint32 address, uint64 value)
{
	// TODO: Check if the ARM byte alignment issue an Android (see "readMemory64") can happen here as well
	uint64* mem = (uint64*)mInternal.accessMemory<MEMORY_MODE_WRITE>(address, 8);
	*mem = swapBytes64(value);
}

void EmulatorInterface::writeMemory8_dev(uint32 address, uint8 value)
{
	*mInternal.accessMemory<MEMORY_MODE_WRITE_DEV>(address, 1) = value;
}

void EmulatorInterface::writeMemory16_dev(uint32 address, uint16 value)
{
	uint16* mem = (uint16*)mInternal.accessMemory<MEMORY_MODE_WRITE_DEV>(address, 2);
	*mem = swapBytes16(value);
}

void EmulatorInterface::writeMemory32_dev(uint32 address, uint32 value)
{
	uint32* mem = (uint32*)mInternal.accessMemory<MEMORY_MODE_WRITE_DEV>(address, 4);
	*mem = swapBytes32(value);
}

void EmulatorInterface::writeMemory64_dev(uint32 address, uint64 value)
{
	uint64* mem = (uint64*)mInternal.accessMemory<MEMORY_MODE_WRITE_DEV>(address, 8);
	*mem = swapBytes64(value);
}

uint32& EmulatorInterface::getRegister(size_t index)
{
	return mInternal.mRegisters[index];
}

uint32& EmulatorInterface::getRegister(Register reg)
{
	return getRegister((size_t)reg);
}

bool EmulatorInterface::getFlagZ()
{
	return mInternal.mFlagZ;
}

bool EmulatorInterface::getFlagN()
{
	return mInternal.mFlagN;
}

void EmulatorInterface::setFlagZ(bool value)
{
	mInternal.mFlagZ = value;
}

void EmulatorInterface::setFlagN(bool value)
{
	mInternal.mFlagN = value;
}

uint8* EmulatorInterface::getVRam()
{
	return mInternal.mVRam;
}

uint16 EmulatorInterface::readVRam16(uint16 vramAddress)
{
	return *(uint16*)(mInternal.mVRam + vramAddress);
}

void EmulatorInterface::writeVRam16(uint16 vramAddress, uint16 value)
{
	uint16* dst = (uint16*)(mInternal.mVRam + vramAddress);
	*dst = value;

	// Mark as changed
	mInternal.mVRamChangeBits.setBit(vramAddress >> 5);
}

void EmulatorInterface::fillVRam(uint16 vramAddress, uint16 fillValue, uint16 bytes)
{
	if (bytes == 0)
		return;

	uint16* dst = (uint16*)(mInternal.mVRam + vramAddress);
	for (uint16 i = 0; i < bytes; i += 2)
	{
		*dst = fillValue;
		++dst;
	}

	// Mark as changed
	const size_t bitIndexStart = (vramAddress >> 5);
	const size_t bitIndexEnd = ((vramAddress + bytes - 1) >> 5);
	mInternal.mVRamChangeBits.setBitsInRange(bitIndexStart, bitIndexEnd);
}

void EmulatorInterface::copyFromMemoryToVRam(uint16 vramAddress, uint32 sourceAddress, uint16 bytes)
{
	if (bytes == 0)
		return;

	uint16* dst = (uint16*)(mInternal.mVRam + vramAddress);
	const uint16* src = (uint16*)(mInternal.accessMemory<MEMORY_MODE_READ>(sourceAddress, bytes));
	const uint16* end = src + (bytes / 2);
	for (; src != end; ++src, ++dst)
	{
		*dst = swapBytes16(*src);
	}

	// Mark as changed
	const size_t bitIndexStart = (vramAddress >> 5);
	const size_t bitIndexEnd = ((vramAddress + bytes - 1) >> 5);
	mInternal.mVRamChangeBits.setBitsInRange(bitIndexStart, bitIndexEnd);
}

BitArray<0x800>& EmulatorInterface::getVRamChangeBits()
{
	return mInternal.mVRamChangeBits;
}

uint16* EmulatorInterface::getVSRam()
{
	return mInternal.mVSRam;
}

std::vector<EmulatorInterface::Watch>& EmulatorInterface::getWatches()
{
	return mInternal.mWatches;
}

void EmulatorInterface::getDirectAccessSpecialization(SpecializationResult& outResult, uint64 address, size_t size, bool writeAccess)
{
	outResult.mSwapBytes = true;
	address &= 0x00ffffff;
	if (address >= 0xff0000)
	{
		address &= 0x00ffff;
		if (address + size > sizeof(mInternal.mRam))
		{
			RMX_ERROR("Too large memory " << (writeAccess ? "write" : "read") << " access of " << rmx::hexString(size) << " bytes at RAM address " << rmx::hexString(0xffff0000 + address, 6), );
			outResult.mResult = SpecializationResult::Result::INVALID_ACCESS;
		}
		else
		{
			outResult.mResult = SpecializationResult::Result::HAS_SPECIALIZATION;
			outResult.mDirectAccessPointer = &mInternal.mRam[address];
		}
	}
	else if (address < 0x400000)
	{
		if (address + size > sizeof(mInternal.mRom))
		{
			RMX_ERROR("Too large memory " << (writeAccess ? "write" : "read") << " access of " << rmx::hexString(size) << " bytes at ROM address " << rmx::hexString(address, 6), );
			outResult.mResult = SpecializationResult::Result::INVALID_ACCESS;
		}
		else
		{
			outResult.mResult = SpecializationResult::Result::HAS_SPECIALIZATION;
			outResult.mDirectAccessPointer = &mInternal.mRom[address];
		}
	}
	else if (address >= 0x800000 && address < 0x900000)
	{
		address &= 0x0fffff;
		if (address + size > sizeof(mInternal.mSharedMemory))
		{
			RMX_ERROR("Too large memory " << (writeAccess ? "write" : "read") << " access of " << rmx::hexString(size) << " bytes at shared memory address " << rmx::hexString(0x800000 + address, 6), );
			outResult.mResult = SpecializationResult::Result::INVALID_ACCESS;
		}
		else
		{
			// Write access is not supported because mSharedMemoryUsage can't be updated this way
			if (writeAccess)
			{
				outResult.mResult = SpecializationResult::Result::NO_SPECIALIZATION;
				return;
			}

			outResult.mResult = SpecializationResult::Result::HAS_SPECIALIZATION;
			outResult.mDirectAccessPointer = &mInternal.mSharedMemory[address];
		}
	}
	else if (address >= 0xa00000 && address < 0xd00000)
	{
		outResult.mResult = SpecializationResult::Result::NO_SPECIALIZATION;
	}
	else
	{
		RMX_ERROR("Invalid memory access at " << rmx::hexString(address, 6) << " of " << rmx::hexString(size) << " bytes", );
		outResult.mResult = SpecializationResult::Result::INVALID_ACCESS;
	}
}


uint8* EmulatorInterfaceDev::getMemoryPointer(uint32 address, bool writeAccess, uint32 size)
{
	if (writeAccess)
	{
		return mInternal.accessMemory<MEMORY_MODE_WRITE_DEV>(address, size);
	}
	else
	{
		return mInternal.accessMemory<MEMORY_MODE_READ>(address, size);
	}
}

void EmulatorInterfaceDev::getDirectAccessSpecialization(SpecializationResult& outResult, uint64 address, size_t size, bool writeAccess)
{
	if (writeAccess)
	{
		// No specialization for write access, as this would not trigger debug watches
		outResult.mResult = SpecializationResult::Result::NO_SPECIALIZATION;
	}
	else
	{
		// Use base implementation for read access
		EmulatorInterface::getDirectAccessSpecialization(outResult, address, size, writeAccess);
	}
}
