/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/SaveStateSerializer.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/rendering/parts/RenderParts.h"
#include "oxygen/rendering/parts/PaletteManager.h"


namespace
{
	// Version history
	//  - 2 and lower: See serialization code for changes
	//  - 3: Using shared memory access flags
	static const constexpr uint8 STANDALONE_SAVESTATE_FORMATVERSION = 3;
}


SaveStateSerializer::SaveStateSerializer(CodeExec& codeExec, RenderParts& renderParts) :
	mCodeExec(codeExec),
	mRenderParts(renderParts)
{
}

bool SaveStateSerializer::loadState(const std::vector<uint8>& input, StateType* outStateType)
{
	// Deserialize
	VectorBinarySerializer serializer(true, input);

	StateType stateType = StateType::INVALID;
	if (!serializeState(serializer, stateType))
		return false;

	// Success
	if (nullptr != outStateType)
		*outStateType = stateType;
	return true;
}

bool SaveStateSerializer::loadState(const std::wstring& filename, StateType* outStateType)
{
	// Fallback for return value
	if (nullptr != outStateType)
		*outStateType = StateType::INVALID;

	// Load file
	std::vector<uint8> state;
	if (!FTX::FileSystem->readFile(filename, state))
		return false;

	// Deserialize
	return loadState(state, outStateType);
}

bool SaveStateSerializer::saveState(std::vector<uint8>& output)
{
	// Save state
	VectorBinarySerializer serializer(false, output);
	StateType stateType = StateType::STANDALONE;	// This is actually ignored
	return serializeState(serializer, stateType);
}

bool SaveStateSerializer::saveState(const std::wstring& filename)
{
	// Save state
	std::vector<uint8> state;
	if (!saveState(state))
		return false;

	return FTX::FileSystem->saveFile(filename, state);
}

bool SaveStateSerializer::serializeState(VectorBinarySerializer& serializer, StateType& stateType)
{
	EmulatorInterface& emulatorInterface = mCodeExec.getEmulatorInterface();

	// Signature and version
	char signature[16];
	if (serializer.isReading())
	{
		serializer.serialize(signature, 16);

		if (memcmp(signature, "Gensx State 1.0", 16) == 0)
		{
			stateType = StateType::GENSX;
			readGensxState(serializer);
		}
		else if (memcmp(signature, "AIR Standalone", 15) == 0)
		{
			stateType = StateType::STANDALONE;
		}
		else if (memcmp(signature, "Oxygen_State__", 15) == 0)
		{
			stateType = StateType::STANDALONE;
		}
		else
		{
			RMX_ERROR("Unrecognized save state format", return false);
		}
	}
	else
	{
		stateType = StateType::STANDALONE;

		memcpy(signature, "Oxygen_State__", 15);
		signature[15] = STANDALONE_SAVESTATE_FORMATVERSION;

		serializer.serialize(signature, 16);
	}

	if (stateType == StateType::STANDALONE)
	{
		const uint8 formatVersion = signature[15];

		// Registers
		for (size_t i = 0; i < 16; ++i)
		{
			serializer.serialize(emulatorInterface.getRegister(i));
		}

		// RAM and VRAM
		serializer.serialize(emulatorInterface.getRam(), 0x10000);
		serializer.serialize(emulatorInterface.getVRam(), 0x10000);
		if (serializer.isReading())
			emulatorInterface.getVRamChangeBits().setAllBits();

		// Shared memory
		if (formatVersion >= 3)
		{
			if (serializer.isReading())
			{
				emulatorInterface.clearSharedMemory();
				const uint64 usageFlags = serializer.read<uint64>();
				for (int bit = 0; bit < 64; ++bit)
				{
					if ((usageFlags >> bit) == 0)
						break;

					if ((usageFlags >> bit) & 1)
					{
						const size_t address = 0x4000 * bit;
						uint8* ptr = emulatorInterface.getMemoryPointer(0x800000 + (uint32)address, true, 0x4000);
						serializer.read(ptr, 0x4000);
					}
				}
			}
			else
			{
				const uint64 usageFlags = emulatorInterface.getSharedMemoryUsage();
				serializer.write(usageFlags);
				for (int bit = 0; bit < 64; ++bit)
				{
					if ((usageFlags >> bit) == 0)
						break;

					if ((usageFlags >> bit) & 1)
					{
						const size_t address = 0x4000 * bit;
						serializer.serialize(emulatorInterface.getSharedMemory() + address, 0x4000);
					}
				}
			}
		}
		else if (formatVersion >= 1)
		{
			serializer.serialize(emulatorInterface.getSharedMemory(), 0x100000);
		}
		else
		{
			if (serializer.isReading())
			{
				memset(emulatorInterface.getSharedMemory(), 0, 0x100000);
			}
		}

		// CRAM, actually part of palette manager's data
		{
			PaletteManager& paletteManager = mRenderParts.getPaletteManager();
			uint16 buffer[0x40];
			if (serializer.isReading())
			{
				serializer.serialize(buffer, 0x80);
				for (int i = 0; i < 0x40; ++i)
				{
					paletteManager.writePaletteEntryPacked(0, i, buffer[i]);
					paletteManager.writePaletteEntryPacked(1, i, buffer[i]);
				}
			}
			else
			{
				for (int i = 0; i < 0x40; ++i)
				{
					buffer[i] = paletteManager.getPaletteEntryPacked(0, i, true);
				}
				serializer.serialize(buffer, 0x80);
			}
		}

		// VSRAM
		serializer.serialize(emulatorInterface.getVSRam(), 0x80);

		// VDP config
		PlaneManager& planeManager = mRenderParts.getPlaneManager();
		ScrollOffsetsManager& scrollOffsetsManager = mRenderParts.getScrollOffsetsManager();
		if (serializer.isReading())
		{
			planeManager.setNameTableBaseA(serializer.read<uint16>());
			planeManager.setNameTableBaseB(serializer.read<uint16>());

			Vec2i playfieldSize;
			playfieldSize.x = serializer.read<uint16>();
			playfieldSize.y = serializer.read<uint16>();
			planeManager.setPlayfieldSizeInPixels(playfieldSize);

			scrollOffsetsManager.setVerticalScrolling(serializer.read<uint8>() != 0);
			scrollOffsetsManager.setHorizontalScrollMask(serializer.read<uint8>());

			if (formatVersion >= 2)
			{
				scrollOffsetsManager.setHorizontalScrollTableBase(serializer.read<uint16>());
			}
		}
		else
		{
			serializer.write<uint16>(planeManager.getNameTableBaseA());
			serializer.write<uint16>(planeManager.getNameTableBaseB());

			const Vec2i playfieldSize = planeManager.getPlayfieldSizeInPixels();
			serializer.write<uint16>(playfieldSize.x);
			serializer.write<uint16>(playfieldSize.y);

			serializer.write<uint8>(scrollOffsetsManager.getVerticalScrolling() ? 1 : 0);
			serializer.write<uint8>(scrollOffsetsManager.getHorizontalScrollMask());

			if (formatVersion >= 2)
			{
				serializer.write<uint16>(scrollOffsetsManager.getHorizontalScrollTableBase());
			}
		}

		// Lemon script runtime state
		if (!mCodeExec.getLemonScriptRuntime().serializeRuntime(serializer))
			return false;
	}

	if (serializer.isReading())
	{
		mRenderParts.getSpriteManager().reset();
	}

	return true;
}

bool SaveStateSerializer::readGensxState(VectorBinarySerializer& serializer)
{
	EmulatorInterface& emulatorInterface = mCodeExec.getEmulatorInterface();

	// Load RAM and swap bytes of each word
	uint8* ram = emulatorInterface.getRam();
	serializer.serialize(ram, 0x10000);
	for (uint32 i = 0; i < 0x10000; i += 2)
	{
		uint8 tmp = ram[i];
		ram[i] = ram[i+1];
		ram[i+1] = tmp;
	}

	memset(emulatorInterface.getSharedMemory(), 0, 0x100000);

	serializer.skip(0x2005);	// Z80 ram and state
	serializer.skip(16);		// IO state

	// Load VDP data
	{
		serializer.skip(0x400);		// VDP state: SAT

		// Load VRAM
		serializer.serialize(emulatorInterface.getVRam(), 0x10000);
		if (serializer.isReading())
			emulatorInterface.getVRamChangeBits().setAllBits();

		// Load CRAM
		{
			PaletteManager& paletteManager = mRenderParts.getPaletteManager();
			uint16 buffer[0x40];
			serializer.serialize(buffer, 0x80);
			for (uint16 i = 0; i < 0x40; ++i)
			{
				const uint16 packedColor = (((buffer[i])      & 0x07) << 1)
										 + (((buffer[i] >> 3) & 0x07) << 5)
										 + (((buffer[i] >> 6) & 0x07) << 9);
				paletteManager.writePaletteEntryPacked(0, i, packedColor);
			}
		}

		// Load VSRAM
		serializer.serialize(emulatorInterface.getVSRam(), 0x80);

		// Load and (at least partially) evaluate registers
		{
			uint8 vdp_reg[0x20];
			serializer.serialize(vdp_reg, 0x20);

			mRenderParts.getPlaneManager().setNameTableBaseA(uint16(vdp_reg[0x02] >> 3) << 13);
			mRenderParts.getPlaneManager().setNameTableBaseB(uint16(vdp_reg[0x04]) << 13);

			const uint8 scrollMasks[4] = { 0x00, 0x07, 0xf8, 0xff };
			mRenderParts.getScrollOffsetsManager().setVerticalScrolling((vdp_reg[0x0b] & 0x04) != 0);
			mRenderParts.getScrollOffsetsManager().setHorizontalScrollMask(scrollMasks[vdp_reg[0x0b] & 0x03]);

			mRenderParts.getScrollOffsetsManager().setHorizontalScrollTableBase((uint16)vdp_reg[0x0d] << 10);

			const uint16 playfieldWidths[4]  = { 256, 512, 256, 1024 };
			const uint16 playfieldHeights[4] = { 256, 512, 768, 1024 };
			mRenderParts.getPlaneManager().setPlayfieldSizeInPixels(Vec2i(playfieldWidths[vdp_reg[0x10] & 3], playfieldHeights[(vdp_reg[0x10] >> 4) & 3]));
		}

		serializer.skip(0x26);		// VDP state: rest
	}

	serializer.skip(0x0df4);	// Sound state

	for (size_t i = 0; i < 16; ++i)
	{
		emulatorInterface.getRegister(i) = serializer.read<uint32>();
	}

	serializer.skip(14);	// PC, SR, USP, ISP
	serializer.skip(12);	// Cycles, int-level, stopped
	serializer.skip(76);	// More Z80 state
	serializer.skip(44);	// MD cartridge ext

	return true;
}
