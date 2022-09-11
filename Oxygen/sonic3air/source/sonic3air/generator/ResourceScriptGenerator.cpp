/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "sonic3air/pch.h"
#include "sonic3air/generator/ResourceScriptGenerator.h"

#include "oxygen/application/Configuration.h"
#include "oxygen/simulation/CodeExec.h"
#include "oxygen/simulation/EmulatorInterface.h"
#include "oxygen/simulation/LemonScriptProgram.h"

#include <lemon/program/Function.h>


namespace
{
	struct Zone
	{
		String mName;
		std::vector<uint16> mInternalZoneActs;

		inline Zone(const char* name, uint16 zoneAct1, uint16 zoneAct2 = 0xffff, uint16 zoneAct3 = 0xffff) :
			mName(name)
		{
			mInternalZoneActs.push_back(zoneAct1);
			if (zoneAct2 != 0xffff)
			{
				mInternalZoneActs.push_back(zoneAct2);
				if (zoneAct3 != 0xffff)
				{
					mInternalZoneActs.push_back(zoneAct3);
				}
			}
		}
	};

	std::vector<Zone> getZonesByGame(int game)		// game 0 = Sonic 3 & Knuckles; game 1 = Sonic 3 alone
	{
		std::vector<Zone> zones;
		zones.emplace_back("01_AIZ", 0x0000, 0x0001);
		zones.emplace_back("02_HCZ", 0x0100, 0x0101);
		zones.emplace_back("03_MGZ", 0x0200, 0x0201);
		zones.emplace_back("04_CNZ", 0x0300, 0x0301);
		zones.emplace_back("05_ICZ", 0x0500, 0x0501);
		zones.emplace_back("06_LBZ", 0x0600, 0x0601);

		const bool isGameSK = (game == 0);
		if (isGameSK)
		{
			zones.emplace_back("07_MHZ", 0x0700, 0x0701);
			zones.emplace_back("08_FBZ", 0x0400, 0x0401);
			zones.emplace_back("09_SOZ", 0x0800, 0x0801);
			zones.emplace_back("10_LRZ", 0x0900, 0x0901, 0x1600);
			zones.emplace_back("11_HPZ", 0x1601);
			zones.emplace_back("12_SSZ", 0x0a00, 0x0a01);
			zones.emplace_back("13_DEZ", 0x0b00, 0x0b01, 0x0c01);
			zones.emplace_back("14_DDZ", 0x0c00);
		}
		return zones;
	}

	uint8 translateS3ObjectTypeToS3K(uint8 type)
	{
		switch (type)
		{
			case 0x80:  return 0x90;
			case 0x81:  return 0x8c;
			case 0x82:  return 0x8d;
			case 0x83:  return 0x8e;
			case 0x87:  return 0xbe;
			case 0x88:  return 0xbf;
			case 0x89:  return 0xc0;
			case 0x8a:  return 0xc1;
			case 0x8b:  return 0xc2;
			case 0x8c:  return 0xcb;
			case 0x8d:  return 0xa7;
			case 0x8e:  return 0xa6;
			case 0x99:  return 0x93;
			case 0x9a:  return 0x94;
			case 0x9b:  return 0x95;
			case 0x9c:  return 0x96;
			case 0x9d:  return 0x97;
			case 0x9e:  return 0x98;
			case 0x9f:  return 0x8f;
			case 0xa0:  return 0xa3;
			case 0xa1:  return 0xa4;
			case 0xa2:  return 0xa5;
			case 0xa3:  return 0x9b;
			case 0xa4:  return 0x9e;
			case 0xa5:  return 0x9c;
			case 0xa6:  return 0x9d;
			case 0xa9:  return 0x92;
			case 0xaa:  return 0xad;
			case 0xab:  return 0xae;
			case 0xad:  return 0x99;
			case 0xae:  return 0xc3;
			case 0xb2:  return 0xbd;
			case 0xb3:  return 0xbc;
			case 0xb5:  return 0x9a;
			case 0xb7:  return 0xc6;
			case 0xb8:  return 0xaf;
			case 0xb9:  return 0xb0;
			case 0xba:  return 0xb1;
			case 0xbb:  return 0xb2;
			case 0xbc:  return 0xb3;
			case 0xbd:  return 0xb4;
			case 0xbe:  return 0xb5;
			case 0xbf:  return 0xb6;
			case 0xc0:  return 0xb7;
			case 0xc1:  return 0xb8;
			case 0xc2:  return 0xb9;
			case 0xc3:  return 0xba;
			case 0xc4:  return 0x9f;
			case 0xc5:  return 0x80;
			case 0xc7:  return 0x82;
			case 0xc8:  return 0xbb;
			case 0xc9:  return 0x83;
			case 0xcb:  return 0x85;
			case 0xcc:  return 0xc4;
			case 0xd1:  return 0x88;
			case 0xd2:  return 0x89;
			case 0xd3:  return 0xc8;
		}
		return type;
	}

	template<typename T>
	void buildInterleavedArray(std::vector<T>& interleavedArray, const std::vector<T>& arrayA, const std::vector<T>& arrayB)
	{
		interleavedArray.clear();

		size_t ia = 0;
		size_t ib = 0;
		while (ia < arrayA.size() || ib < arrayB.size())
		{
			if (ia >= arrayA.size())
			{
				interleavedArray.emplace_back(arrayB[ib]);
				++ib;
				continue;
			}
			if (ib >= arrayB.size())
			{
				interleavedArray.emplace_back(arrayA[ia]);
				++ia;
				continue;
			}

			const T& a = arrayA[ia];
			const T& b = arrayB[ib];
			const int comparisonResult = (a.mPosition.x != b.mPosition.x) ? ((a.mPosition.x < b.mPosition.x) ? -1 : 1) :
										 (a.mPosition.y != b.mPosition.y) ? ((a.mPosition.y < b.mPosition.y) ? -1 : 1) : 0;
			if (comparisonResult < 0)
			{
				interleavedArray.emplace_back(a);
				++ia;
				continue;
			}
			if (comparisonResult > 0)
			{
				interleavedArray.emplace_back(b);
				++ib;
				continue;
			}

			if (a.mFlags == b.mFlags && a.mType == b.mType && a.mSubtype == b.mSubtype)
			{
				// Both are equal
				interleavedArray.emplace_back(a);
				interleavedArray.back().mGames = 2;		// Part of both games
			}
			else
			{
				interleavedArray.emplace_back(a);
				interleavedArray.emplace_back(b);
			}
			++ia;
			++ib;
		}
	}
}


void ResourceScriptGenerator::generateLevelObjectTableScript(CodeExec& codeExec)
{
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();

	struct ObjectData
	{
		Vec2i mPosition;
		uint8 mFlags;
		uint8 mType;
		uint8 mSubtype;
		uint8 mGames;
	};
	typedef std::vector<ObjectData> ObjectDataArray;
	std::map<uint32, ObjectDataArray> objectsByLevel;	// Key encodes game, zone and act

	for (int sourceGame = 0; sourceGame < 2; ++sourceGame)
	{
		const bool isGameSK = (sourceGame == 0);
		std::vector<Zone> zones = getZonesByGame(sourceGame);

		for (int zoneNumber = 0; zoneNumber < (int)zones.size(); ++zoneNumber)
		{
			const Zone& zone = zones[zoneNumber];
			const int objectTableNumber = (zone.mInternalZoneActs[0] < 0x0700) ? 1 : 2;

			for (int actNumber = 0; actNumber < (int)zone.mInternalZoneActs.size(); ++actNumber)
			{
				const uint32 levelKey = (sourceGame << 16) + (zoneNumber << 8) + actNumber;
				ObjectDataArray& objects = objectsByLevel[levelKey];
				objects.reserve(0x400);

				const uint32 offset = (zone.mInternalZoneActs[actNumber] >> 8) * 8 + (zone.mInternalZoneActs[actNumber] & 0xff) * 4;
				uint32 startAddress;
				if (isGameSK)
					startAddress = emulatorInterface.readMemory32(0x1e3d98 + offset);				// For S&K
				else
					startAddress = 0x200000 + emulatorInterface.readMemory32(0x25e0d8 + offset);	// For Sonic 3 without S&K

				int objIndex = 0;
				while (emulatorInterface.readMemory16(startAddress + objIndex * 6) != 0xffff)
				{
					const uint32 objAddress = startAddress + objIndex * 6;
					ObjectData& obj = vectorAdd(objects);

					obj.mPosition.x	= emulatorInterface.readMemory16(objAddress);
					obj.mPosition.y	= emulatorInterface.readMemory16(objAddress + 2) & 0x0fff;
					obj.mFlags		= emulatorInterface.readMemory16(objAddress + 2) >> 13;
					obj.mType		= emulatorInterface.readMemory8(objAddress + 4);
					obj.mSubtype	= emulatorInterface.readMemory8(objAddress + 5);
					obj.mGames		= sourceGame;

					if (!isGameSK && objectTableNumber == 1)
					{
						// Translate certain type IDs that have changed from S3 to S3&K
						obj.mType = translateS3ObjectTypeToS3K(obj.mType);
					}

					++objIndex;
					RMX_CHECK(objIndex < 0x1000, "Too many objects", break);
				}

				// Sort the list to have a defined order
				std::sort(objects.begin(), objects.end(), [](const ObjectData& a, const ObjectData& b) { return (a.mPosition.x != b.mPosition.x) ? (a.mPosition.x < b.mPosition.x) : (a.mPosition.y < b.mPosition.y); });
			}
		}
	}

	ObjectDataArray interleavedObjects;
	for (int sourceGame = 0; sourceGame < 2; ++sourceGame)
	{
		const bool isGameSK = (sourceGame == 0);
		std::vector<Zone> zones = getZonesByGame(sourceGame);

		for (int zoneNumber = 0; zoneNumber < (int)zones.size(); ++zoneNumber)
		{
			String output;

			const Zone& zone = zones[zoneNumber];
			const String zoneName = zone.mName;
			const String zoneNameShort = zoneName.getSubString(3, -1);
			const int objectTableNumber = (zone.mInternalZoneActs[0] < 0x0700) ? 1 : 2;

			for (int actNumber = 0; actNumber < (int)zone.mInternalZoneActs.size(); ++actNumber)
			{
				const uint32 levelKey = (sourceGame << 16) + (zoneNumber << 8) + actNumber;
				const ObjectDataArray* objects = &objectsByLevel[levelKey];
				uint8 lastGames = sourceGame;

				if (isGameSK)
				{
					const uint32 levelKeyS3 = (1 << 16) + (zoneNumber << 8) + actNumber;
					const auto it = objectsByLevel.find(levelKeyS3);
					if (it != objectsByLevel.end())
					{
						// Build interleaved object list comparing S3 and S3&K layouts
						buildInterleavedArray<ObjectData>(interleavedObjects, objectsByLevel[levelKey], it->second);
						objects = &interleavedObjects;
						lastGames = 2;
					}
				}

				String functionName;
				functionName << "LevelObjectTableBuilder.buildObjects_" << zoneNameShort << (char)('1' + actNumber);
				if (!isGameSK)
					functionName << "_sonic3";

				output << "\r\n\r\n";
				output << "function void " << functionName << "()\r\n";
				output << "{\r\n";

				int indent = 1;
				for (const ObjectData& obj : *objects)
				{
					if (obj.mGames != lastGames)
					{
						if (lastGames == 2)
						{
							output << "\r\n";
							output << "\tif (useSetting(SETTING_LEVELLAYOUTS) " << (obj.mGames == 0 ? "!=" : "==") << " 0))\r\n";
							output << "\t{\r\n";
							indent = 2;
						}
						else if (obj.mGames == 2)
						{
							output << "\t}\r\n";
							output << "\r\n";
							indent = 1;
						}
						else
						{
							output << "\t}\r\n";
							output << "\telse\r\n";
							output << "\t{\r\n";
						}

						lastGames = obj.mGames;
					}

					output << (indent < 2 ? "\t" : "\t\t")
						   << "LevelObjectTableBuilder.addObject("
						   << rmx::hexString(obj.mPosition.x, 4) << ", "
						   << rmx::hexString(obj.mPosition.y, 4) << ", "
						   << "OBJECTTABLE" << objectTableNumber << "_" << rmx::hexString(obj.mType, 2, "") << ", "
						   << rmx::hexString(obj.mSubtype, 2) << ", ";

					if (obj.mFlags & 0x07)
					{
						if (obj.mFlags & 0x01)
						{
							output << "LVLOBJ_FLIP_X";
						}
						if (obj.mFlags & 0x02)
						{
							if (obj.mFlags & 0x01)
								output << " | ";
							output << "LVLOBJ_FLIP_Y";
						}
						if (obj.mFlags & 0x04)
						{
							if (obj.mFlags & 0x03)
								output << " | ";
							output << "LVLOBJ_IGNORE_Y";
						}
					}
					else
					{
						output << "0";
					}
					output << ")\r\n";
				}

				if (indent > 1)
				{
					output << "\t}\r\n";
					output << "\r\n";
				}

				output << "\tu16[A0] = 0xffff\r\n";
				output << "\tA0 += 2\r\n";
				output << "}\r\n";
			}

			String filename = String("levelobjects_") + zoneName;
			filename.lowerCase();
			if (!isGameSK)
				filename << "_sonic3";
			output.saveFile(*(WString(Configuration::instance().mScriptsDir).toString() + "/standalone/resources/level_objects/" + filename + ".lemon"));
		}
	}

#if 0
	// Extra functionality: Write object tables
	{
		const uint32 tablesAddresses[3] = { 0x094ea2, 0x0952a2, 0x25cc96 };		// First two are S&K tables, last is S3 only (note: function addresses are different there, so resolving won't work)
		for (int tableIndex = 0; tableIndex < 3; ++tableIndex)
		{
			String output;
			const uint32 tableAddress = tablesAddresses[tableIndex];
			for (int i = 0; i < 0x100; ++i)
			{
				const uint32 objectAddress = emulatorInterface.readMemory32(tableAddress + i * 4);
				output << rmx::hexString(i, 2) << ": Object at " << rmx::hexString(objectAddress, 6);

				const LemonScriptProgram::Hook* hook = codeExec.getLemonScriptProgram().checkForAddressHook(objectAddress);
				if (nullptr != hook && hook->mFunction)
				{
					std::string filename;
					uint32 lineNumber;
					codeExec.getLemonScriptProgram().resolveLocation(*hook->mFunction, 0, filename, lineNumber);
					output << " -- " << filename;
				}
				output << "\r\n";
			}

			output.saveFile(*String(0, "table%d.txt", tableIndex + 1));
		}
	}
#endif
}

void ResourceScriptGenerator::generateLevelRingsTableScript(CodeExec& codeExec)
{
	EmulatorInterface& emulatorInterface = codeExec.getEmulatorInterface();

	struct RingData
	{
		Vec2i mPosition;
		uint8 mFlags = 0;		// Only dummy, not used
		uint8 mType = 0;		// Only dummy, not used
		uint8 mSubtype = 0;		// Only dummy, not used
		uint8 mGames;
	};
	typedef std::vector<RingData> RingDataArray;
	std::map<uint32, RingDataArray> ringsByLevel;	// Key encodes game, zone and act

	for (int sourceGame = 0; sourceGame < 2; ++sourceGame)
	{
		const bool isGameSK = (sourceGame == 0);
		std::vector<Zone> zones = getZonesByGame(sourceGame);

		for (int zoneNumber = 0; zoneNumber < (int)zones.size(); ++zoneNumber)
		{
			const Zone& zone = zones[zoneNumber];

			for (int actNumber = 0; actNumber < (int)zone.mInternalZoneActs.size(); ++actNumber)
			{
				const uint32 levelKey = (sourceGame << 16) + (zoneNumber << 8) + actNumber;
				RingDataArray& rings = ringsByLevel[levelKey];
				rings.reserve(0x400);

				const uint32 offset = (zone.mInternalZoneActs[actNumber] >> 8) * 8 + (zone.mInternalZoneActs[actNumber] & 0xff) * 4;
				uint32 startAddress;
				if (isGameSK)
					startAddress = emulatorInterface.readMemory32(0x1e3e58 + offset);				// For S&K
				else
					startAddress = 0x200000 + emulatorInterface.readMemory32(0x25e198 + offset);	// For Sonic 3 without S&K

				rings.clear();
				for (int ringIndex = 0; ringIndex < 0x1ff; ++ringIndex)
				{
					const uint32 ringAddress = startAddress + ringIndex * 4;
					const uint16 px = emulatorInterface.readMemory16(ringAddress);
					const uint16 py = emulatorInterface.readMemory16(ringAddress + 2);
					if (px & 0x8000)
						break;

					RingData& ring = vectorAdd(rings);
					ring.mPosition.set(px, py);
					ring.mGames = sourceGame;
				}

				// Sort ring list
				//  -> Sonic 3 seems to use different sorting for rings with same x-position than S&K, we better correct this
				std::sort(rings.begin(), rings.end(), [](const RingData& a, const RingData& b) { return (a.mPosition.x != b.mPosition.x) ? (a.mPosition.x < b.mPosition.x) : (a.mPosition.y < b.mPosition.y); });
			}
		}
	}

	RingDataArray interleavedRings;
	for (int sourceGame = 0; sourceGame < 2; ++sourceGame)
	{
		const bool isGameSK = (sourceGame == 0);
		std::vector<Zone> zones = getZonesByGame(sourceGame);

		for (int zoneNumber = 0; zoneNumber < (int)zones.size(); ++zoneNumber)
		{
			String output;

			const Zone& zone = zones[zoneNumber];
			const String zoneName = zone.mName;
			const String zoneNameShort = zoneName.getSubString(3, -1);

			for (int actNumber = 0; actNumber < (int)zone.mInternalZoneActs.size(); ++actNumber)
			{
				const uint32 levelKey = (sourceGame << 16) + (zoneNumber << 8) + actNumber;
				const RingDataArray* rings = &ringsByLevel[levelKey];
				uint8 lastGames = sourceGame;

				if (isGameSK)
				{
					const uint32 levelKeyS3 = (1 << 16) + (zoneNumber << 8) + actNumber;
					const auto it = ringsByLevel.find(levelKeyS3);
					if (it != ringsByLevel.end())
					{
						// Build interleaved object list comparing S3 and S3&K layouts
						buildInterleavedArray<RingData>(interleavedRings, ringsByLevel[levelKey], it->second);
						rings = &interleavedRings;
						lastGames = 2;
					}
				}

				String functionName;
				functionName << "LevelRingsTableBuilder.buildRings_" << zoneNameShort << (char)('1' + actNumber);
				if (!isGameSK)
					functionName << "_sonic3";

				output << "\r\n\r\n";
				output << "function void " << functionName << "()\r\n";
				output << "{\r\n";

				int indent = 1;
				for (const RingData& ring : *rings)
				{
					if (ring.mGames != lastGames)
					{
						if (lastGames == 2)
						{
							output << "\r\n";
							output << "\tif (useSetting(SETTING_LEVELLAYOUTS) " << (ring.mGames == 0 ? "!=" : "==") << " 0))\r\n";
							output << "\t{\r\n";
							indent = 2;
						}
						else if (ring.mGames == 2)
						{
							output << "\t}\r\n";
							output << "\r\n";
							indent = 1;
						}
						else
						{
							output << "\t}\r\n";
							output << "\telse\r\n";
							output << "\t{\r\n";
						}

						lastGames = ring.mGames;
					}

					output << (indent < 2 ? "\t" : "\t\t")
						   << "LevelRingsTableBuilder.addRing("
						   << rmx::hexString(ring.mPosition.x, 4) << ", "
						   << rmx::hexString(ring.mPosition.y, 4) << ")\r\n";
				}

				if (indent > 1)
				{
					output << "\t}\r\n";
					output << "\r\n";
				}

				output << "\tu32[A1] = 0xffffffff\r\n";
				output << "\tA1 += 4\r\n";
				output << "}\r\n";
			}

			String filename = String("levelrings_") + zoneName;
			filename.lowerCase();
			if (!isGameSK)
				filename << "_sonic3";
			output.saveFile(*(WString(Configuration::instance().mScriptsDir).toString() + "/standalone/resources/level_rings/" + filename + ".lemon"));
		}
	}
}

void ResourceScriptGenerator::convertLevelObjectsBinToScript(const std::wstring& inputFilename, const std::wstring& outputFilename, int objectTableNumber)
{
	std::vector<uint8> inputData;
	if (!FTX::FileSystem->readFile(inputFilename, inputData))
		return;

	VectorBinarySerializer input(true, inputData);
	String output;

	const size_t count = inputData.size() / 6;
	for (size_t i = 0; i < count; ++i)
	{
		uint16 px = swapBytes16(input.read<uint16>());
		uint16 py = swapBytes16(input.read<uint16>());
		const uint8 flags = py >> 13;
		py &= 0x0fff;
		const uint8 type = input.read<uint8>();
		const uint8 subtype = input.read<uint8>();

		output << "\tLevelObjectTableBuilder.addObject("
			   << rmx::hexString(px, 4) << ", "
			   << rmx::hexString(py, 4) << ", "
			   << "OBJECTTABLE" << objectTableNumber << "_" << rmx::hexString(type, 2, "") << ", "
			   << rmx::hexString(subtype, 2) << ", ";

		if (flags & 0x07)
		{
			if (flags & 0x01)
			{
				output << "LVLOBJ_FLIP_X";
			}
			if (flags & 0x02)
			{
				if (flags & 0x01)
					output << " | ";
				output << "LVLOBJ_FLIP_Y";
			}
			if (flags & 0x04)
			{
				if (flags & 0x03)
					output << " | ";
				output << "LVLOBJ_IGNORE_Y";
			}
		}
		else
		{
			output << "0";
		}
		output << ")\r\n";
	}
	output.saveFile(outputFilename);
}

void ResourceScriptGenerator::convertLevelRingsBinToScript(const std::wstring& inputFilename, const std::wstring& outputFilename)
{
	std::vector<uint8> inputData;
	if (!FTX::FileSystem->readFile(inputFilename, inputData))
		return;

	VectorBinarySerializer input(true, inputData);
	String output;

	const size_t count = inputData.size() / 4;
	for (size_t i = 0; i < count; ++i)
	{
		uint16 px = swapBytes16(input.read<uint16>());
		uint16 py = swapBytes16(input.read<uint16>());
		output << "\tLevelRingsTableBuilder.addRing("
			   << rmx::hexString(px, 4) << ", "
			   << rmx::hexString(py, 4) << ")\r\n";
	}
	output.saveFile(outputFilename);
}
