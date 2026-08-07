/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/simulation/sound/SoundDriver.h"
#include "oxygen/simulation/EmulatorInterface.h"


// General note:
//  This is heavily based on the S&K sound driver disassembly by MarkeyJester, Linncaki and Flamewing.
//  When the following code's comments talk about "the disassembly" and fixes from it, that refers to this exact disassembly.
//  You can find it in the skdisasm project on Github: https://github.com/sonicretro/skdisasm/blob/master/Sound/Z80%20Sound%20Driver.asm


static const uint8 DACBanksData[0x47] =		// Originally only 0x45 bytes, but extended to allow for a fix for the S3A mini-boss and Knuckles themes
{
	0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e,
	0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x5d, 0x5d, 0x5d, 0x5d,
	0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5d, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
	0x5e, 0x5e, 0x1e, 0x1e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e,
	0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e, 0x5e
};

static const uint8 zFMInstrumentRegTableData[0x1d] =
{
	0xb0, 0x30, 0x38, 0x34, 0x3c, 0x50, 0x58, 0x54, 0x5c, 0x60, 0x68, 0x64, 0x6c, 0x70, 0x78, 0x74,
	0x7c, 0x80, 0x88, 0x84, 0x8c, 0x40, 0x48, 0x44, 0x4c, 0x90, 0x98, 0x94, 0x9c
};

static const uint8 zFMDACInitBytesData[0x14] =
{
	0x80, 0x06, 0x80, 0x00, 0x80, 0x01, 0x80, 0x02, 0x80, 0x04, 0x80, 0x05, 0x80, 0x06,		// zFMDACInitBytes
	0x80, 0x80, 0x80, 0xa0, 0x80, 0xc0														// zPSGInitBytes
};

static const uint8 zSFXChannelDataData[0x20] =
{
	0xf0, 0x1d, 0x20, 0x1e, 0x50, 0x1e, 0x80, 0x1e, 0xb0, 0x1e, 0xe0, 0x1e, 0x10, 0x1f, 0x10, 0x1f,
	0xd0, 0x1c, 0x00, 0x1d, 0x30, 0x1d, 0x40, 0x1c, 0x60, 0x1d, 0x90, 0x1d, 0xc0, 0x1d, 0xc0, 0x1d
};

static const uint8 zPSGFrequenciesData[0xf3] =
{
	0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03, 0xff, 0x03,
	0xff, 0x03, 0xf7, 0x03, 0xbe, 0x03, 0x88, 0x03, 0x56, 0x03, 0x26, 0x03, 0xf9, 0x02, 0xce, 0x02,
	0xa5, 0x02, 0x80, 0x02, 0x5c, 0x02, 0x3a, 0x02, 0x1a, 0x02, 0xfb, 0x01, 0xdf, 0x01, 0xc4, 0x01,
	0xab, 0x01, 0x93, 0x01, 0x7d, 0x01, 0x67, 0x01, 0x53, 0x01, 0x40, 0x01, 0x2e, 0x01, 0x1d, 0x01,
	0x0d, 0x01, 0xfe, 0x00, 0xef, 0x00, 0xe2, 0x00, 0xd6, 0x00, 0xc9, 0x00, 0xbe, 0x00, 0xb4, 0x00,
	0xa9, 0x00, 0xa0, 0x00, 0x97, 0x00, 0x8f, 0x00, 0x87, 0x00, 0x7f, 0x00, 0x78, 0x00, 0x71, 0x00,
	0x6b, 0x00, 0x65, 0x00, 0x5f, 0x00, 0x5a, 0x00, 0x55, 0x00, 0x50, 0x00, 0x4b, 0x00, 0x47, 0x00,
	0x43, 0x00, 0x40, 0x00, 0x3c, 0x00, 0x39, 0x00, 0x36, 0x00, 0x33, 0x00, 0x30, 0x00, 0x2d, 0x00,
	0x2b, 0x00, 0x28, 0x00, 0x26, 0x00, 0x24, 0x00, 0x22, 0x00, 0x20, 0x00, 0x1f, 0x00, 0x1d, 0x00,
	0x1b, 0x00, 0x1a, 0x00, 0x18, 0x00, 0x17, 0x00, 0x16, 0x00, 0x15, 0x00, 0x13, 0x00, 0x12, 0x00,
	0x11, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x84, 0x02, 0xab, 0x02, 0xd3, 0x02, 0xfe, 0x02,
	0x2d, 0x03, 0x5c, 0x03, 0x8f, 0x03, 0xc5, 0x03, 0xff, 0x03, 0x3c, 0x04, 0x7c, 0x04, 0xc0, 0x04,
	0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x59, 0x1d, 0x1d, 0x5a, 0x5a, 0x5a, 0x5a, 0x1d, 0x1d,
	0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x5b, 0x1d, 0x5b,
	0x5b, 0x5b, 0x5b, 0x5b, 0x1d, 0x5b, 0x1c, 0x1c, 0x1c, 0x1d, 0x1d, 0x1c, 0x5b, 0x1d, 0x1c, 0x1c,
	0x5b, 0x1c, 0x1c
};

static const uint8 DecTableData[0x10] =
{
	0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0
};

static const uint8 z80_SoundDriverPointersData[0x843] =
{
	0x18, 0x16, 0xd8, 0x17, 0x18, 0x16, 0x7e, 0x16, 0x0e, 0x13, 0x87, 0x13, 0x33, 0x00, 0x1f, 0x13, 0x1e, 0x13, 0x2b, 0x13, 0x38, 0x13, 0x46, 0x13, 0x52, 0x13, 0x64, 0x13, 0x75, 0x13, 0x00, 0x01,
	0x02, 0x01, 0x00, 0xff, 0xfe, 0xfd, 0xfc, 0xfd, 0xfe, 0xff, 0x83, 0x00, 0x00, 0x00, 0x00, 0x13, 0x26, 0x39, 0x4c, 0x5f, 0x72, 0x7f, 0x72, 0x83, 0x01, 0x02, 0x03, 0x02, 0x01, 0x00, 0xff, 0xfe,
	0xfd, 0xfe, 0xff, 0x00, 0x82, 0x00, 0x00, 0x00, 0x01, 0x03, 0x01, 0x00, 0xff, 0xfd, 0xff, 0x00, 0x82, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0a, 0x14, 0x1e, 0x14, 0x0a, 0x00, 0xf6, 0xec, 0xe2,
	0xec, 0xf6, 0x82, 0x04, 0x00, 0x00, 0x00, 0x00, 0x16, 0x2c, 0x42, 0x2c, 0x16, 0x00, 0xea, 0xd4, 0xbe, 0xd4, 0xea, 0x82, 0x03, 0x01, 0x02, 0x03, 0x04, 0x03, 0x02, 0x01, 0x00, 0xff, 0xfe, 0xfd,
	0xfc, 0xfd, 0xfe, 0xff, 0x00, 0x82, 0x01, 0xd5, 0x13, 0xd7, 0x13, 0xde, 0x13, 0xf7, 0x13, 0x03, 0x14, 0x0e, 0x14, 0x1d, 0x14, 0x26, 0x14, 0x37, 0x14, 0x42, 0x14, 0x57, 0x14, 0x61, 0x14, 0x6a,
	0x14, 0x6c, 0x14, 0x6e, 0x14, 0x75, 0x14, 0x94, 0x14, 0x9b, 0x14, 0xa6, 0x14, 0xb5, 0x14, 0xbb, 0x14, 0xcc, 0x14, 0xd7, 0x14, 0xec, 0x14, 0x05, 0x15, 0x0e, 0x15, 0x15, 0x15, 0x1e, 0x15, 0x2f,
	0x15, 0x5c, 0x15, 0x5f, 0x15, 0x63, 0x15, 0x70, 0x15, 0x82, 0x15, 0x89, 0x15, 0x92, 0x15, 0x96, 0x15, 0xa8, 0x15, 0x0b, 0x16, 0x02, 0x83, 0x00, 0x02, 0x04, 0x06, 0x08, 0x10, 0x83, 0x02, 0x01,
	0x00, 0x00, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x81, 0x00, 0x00, 0x02, 0x03, 0x04, 0x04, 0x05, 0x05, 0x05,
	0x06, 0x06, 0x81, 0x03, 0x00, 0x01, 0x01, 0x01, 0x02, 0x03, 0x04, 0x04, 0x05, 0x81, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x08, 0x07, 0x07, 0x06, 0x81, 0x01, 0x0c, 0x03,
	0x0f, 0x02, 0x07, 0x03, 0x0f, 0x80, 0x00, 0x00, 0x00, 0x02, 0x03, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0e, 0x0f, 0x83, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x01, 0x02, 0x03,
	0x04, 0x81, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x81, 0x10, 0x20, 0x30, 0x40, 0x30, 0x20, 0x10, 0x00, 0xf0,
	0x80, 0x00, 0x00, 0x01, 0x01, 0x03, 0x03, 0x04, 0x05, 0x83, 0x00, 0x81, 0x02, 0x83, 0x00, 0x02, 0x04, 0x06, 0x08, 0x10, 0x83, 0x09, 0x09, 0x09, 0x08, 0x08, 0x08, 0x07, 0x07, 0x07, 0x06, 0x06,
	0x06, 0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x81, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x81, 0x03, 0x00, 0x01, 0x01, 0x01,
	0x02, 0x03, 0x04, 0x04, 0x05, 0x81, 0x00, 0x00, 0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x05, 0x06, 0x08, 0x07, 0x07, 0x06, 0x81, 0x0a, 0x05, 0x00, 0x04, 0x08, 0x83, 0x00, 0x00, 0x00, 0x02, 0x03,
	0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0e, 0x0f, 0x83, 0x03, 0x02, 0x01, 0x01, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 0x81, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x02,
	0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x81, 0x10, 0x20, 0x30, 0x40, 0x30, 0x20, 0x10, 0x00, 0x10, 0x20, 0x30, 0x40, 0x30, 0x20, 0x10, 0x00, 0x10, 0x20, 0x30, 0x40,
	0x30, 0x20, 0x10, 0x00, 0x80, 0x00, 0x00, 0x01, 0x01, 0x03, 0x03, 0x04, 0x05, 0x83, 0x00, 0x02, 0x04, 0x06, 0x08, 0x16, 0x83, 0x00, 0x00, 0x01, 0x01, 0x03, 0x03, 0x04, 0x05, 0x83, 0x04, 0x04,
	0x04, 0x04, 0x03, 0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x83, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x04,
	0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x0a, 0x0a, 0x0a, 0x0a, 0x81, 0x00, 0x0a, 0x83, 0x00,
	0x02, 0x04, 0x81, 0x30, 0x20, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x10, 0x20, 0x30, 0x81, 0x00, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x06, 0x06, 0x06, 0x08, 0x08,
	0x0a, 0x83, 0x00, 0x02, 0x03, 0x04, 0x06, 0x07, 0x81, 0x02, 0x01, 0x00, 0x00, 0x00, 0x02, 0x04, 0x07, 0x81, 0x0f, 0x01, 0x05, 0x83, 0x08, 0x06, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02,
	0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
	0x08, 0x08, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x09, 0x83, 0x00, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x04, 0x04, 0x04, 0x05, 0x05, 0x83, 0x00, 0x80, 0x6d, 0x9b, 0xbc, 0xb0, 0xc6, 0xc0,
	0x64, 0xd3, 0x7b, 0xd9, 0x8f, 0xe4, 0xa9, 0xdd, 0x00, 0x80, 0x97, 0x85, 0xaa, 0x86, 0x00, 0x80, 0x45, 0x93, 0xc8, 0x8d, 0xfe, 0x8a, 0x06, 0x91, 0x88, 0x96, 0xf2, 0x9c, 0xe5, 0xa2, 0xf3, 0xac,
	0x80, 0xbe, 0xb4, 0xc2, 0x9f, 0xc7, 0xb1, 0xcb, 0xe1, 0xce, 0xdd, 0xd3, 0xc0, 0xdc, 0x23, 0xe2, 0xbb, 0xea, 0xe8, 0x8a, 0xa3, 0xf5, 0xf7, 0x99, 0xfd, 0xa4, 0xec, 0xb0, 0x24, 0xc3, 0x47, 0xda,
	0x8e, 0xf8, 0x87, 0xe5, 0x4b, 0xdd, 0xa6, 0xdf, 0xc0, 0xe3, 0x4b, 0xfd, 0x75, 0xfe, 0x74, 0xe5, 0xe4, 0xf5, 0xb1, 0xcb, 0xaf, 0xe7, 0x4c, 0xf7, 0xbe, 0xfa, 0xde, 0xfc, 0x04, 0xc1, 0x30, 0xde,
	0x5e, 0xde, 0x6f, 0xde, 0xa1, 0xde, 0xc5, 0xde, 0xf4, 0xde, 0x2a, 0xdf, 0x6b, 0xdf, 0x96, 0xdf, 0xe5, 0xdf, 0x23, 0xe0, 0x5d, 0xe0, 0x88, 0xe0, 0xab, 0xe0, 0xce, 0xe0, 0xf1, 0xe0, 0x09, 0xe1,
	0x22, 0xe1, 0x4f, 0xe1, 0x77, 0xe1, 0xa4, 0xe1, 0xc4, 0xe1, 0xde, 0xe1, 0x06, 0xe2, 0x2e, 0xe2, 0x78, 0xe2, 0xa2, 0xe2, 0xcf, 0xe2, 0x13, 0xe3, 0x22, 0xe3, 0x5a, 0xe3, 0x8b, 0xe3, 0xa8, 0xe3,
	0xe8, 0xe3, 0x2b, 0xe4, 0x53, 0xe4, 0x63, 0xe4, 0x81, 0xe4, 0x9a, 0xe4, 0xf6, 0xe4, 0x23, 0xe5, 0x30, 0xe5, 0x58, 0xe5, 0x81, 0xe5, 0xb2, 0xe5, 0xda, 0xe5, 0x1b, 0xe6, 0x4c, 0xe6, 0x62, 0xe6,
	0x8c, 0xe6, 0xab, 0xe6, 0xe1, 0xe6, 0x30, 0xe7, 0x5c, 0xe7, 0xb0, 0xe7, 0xdd, 0xe7, 0x11, 0xe8, 0x23, 0xe8, 0x33, 0xe8, 0x52, 0xe8, 0x86, 0xe8, 0x96, 0xe8, 0xe0, 0xe8, 0xea, 0xe8, 0x17, 0xe9,
	0x4b, 0xe9, 0x78, 0xe9, 0xa7, 0xe9, 0xd1, 0xe9, 0x1b, 0xea, 0x48, 0xea, 0x93, 0xea, 0xc7, 0xea, 0xf7, 0xea, 0x28, 0xeb, 0x55, 0xeb, 0x6d, 0xeb, 0x8b, 0xeb, 0xba, 0xeb, 0x05, 0xec, 0x32, 0xec,
	0x7e, 0xec, 0xab, 0xec, 0xd8, 0xec, 0x05, 0xed, 0x3b, 0xed, 0x68, 0xed, 0x75, 0xed, 0xa9, 0xed, 0xdf, 0xed, 0x10, 0xee, 0x2a, 0xee, 0x5b, 0xee, 0x91, 0xee, 0xc3, 0xee, 0xf9, 0xee, 0x2d, 0xef,
	0x77, 0xef, 0xa6, 0xef, 0xd5, 0xef, 0x09, 0xf0, 0x1c, 0xf0, 0x68, 0xf0, 0x90, 0xf0, 0xaf, 0xf0, 0x14, 0xf1, 0x4b, 0xf1, 0x7f, 0xf1, 0xc0, 0xf1, 0xfc, 0xf1, 0x14, 0xf2, 0x3c, 0xf2, 0x74, 0xf2,
	0xa1, 0xf2, 0xce, 0xf2, 0xfb, 0xf2, 0x13, 0xf3, 0x3b, 0xf3, 0x65, 0xf3, 0x8f, 0xf3, 0xea, 0xf3, 0x2a, 0xf4, 0x9c, 0xf4, 0xab, 0xf4, 0xda, 0xf4, 0x07, 0xf5, 0x82, 0xf5, 0xd7, 0xf5, 0x03, 0xf6,
	0x7d, 0xf6, 0xaa, 0xf6, 0xd2, 0xf6, 0x13, 0xf7, 0x45, 0xf7, 0x6c, 0xf7, 0x94, 0xf7, 0xbe, 0xf7, 0xce, 0xf7, 0xf9, 0xf7, 0x37, 0xf8, 0x6a, 0xf8, 0x9c, 0xf8, 0xd1, 0xf8, 0x07, 0xf9, 0x1e, 0xf9,
	0x4e, 0xf9, 0x7e, 0xf9, 0xb7, 0xf9, 0xf2, 0xf9, 0x21, 0xfa, 0x2b, 0xfa, 0x66, 0xfa, 0x9c, 0xfa, 0xd7, 0xfa, 0x12, 0xfb, 0x45, 0xfb, 0x60, 0xfb, 0x6a, 0xfb, 0xa1, 0xfb, 0xbe, 0xfb, 0xf4, 0xfb,
	0x2d, 0xfc, 0x64, 0xfc, 0x9d, 0xfc, 0xce, 0xfc, 0xff, 0xfc, 0x32, 0xfd, 0x62, 0xfd, 0x94, 0xfd, 0x94, 0xfd, 0x94, 0xfd, 0x94, 0xfd, 0x94, 0xfd, 0x3c, 0x01, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x15,
	0x1f, 0x11, 0x0d, 0x12, 0x05, 0x07, 0x04, 0x09, 0x02, 0x55, 0x3a, 0x25, 0x1a, 0x1a, 0x80, 0x07, 0x80, 0x3d, 0x01, 0x01, 0x01, 0x01, 0x94, 0x19, 0x19, 0x19, 0x0f, 0x0d, 0x0d, 0x0d, 0x07, 0x04,
	0x04, 0x04, 0x25, 0x1a, 0x1a, 0x1a, 0x15, 0x80, 0x80, 0x80, 0x03, 0x00, 0xd7, 0x33, 0x02, 0x5f, 0x9f, 0x5f, 0x1f, 0x13, 0x0f, 0x0a, 0x0a, 0x10, 0x0f, 0x02, 0x09, 0x35, 0x15, 0x25, 0x1a, 0x13,
	0x16, 0x15, 0x80, 0x34, 0x70, 0x72, 0x31, 0x31, 0x1f, 0x1f, 0x1f, 0x1f, 0x10, 0x06, 0x06, 0x06, 0x01, 0x06, 0x06, 0x06, 0x35, 0x1a, 0x15, 0x1a, 0x10, 0x83, 0x18, 0x83, 0x3e, 0x77, 0x71, 0x32,
	0x31, 0x1f, 0x1f, 0x1f, 0x1f, 0x0d, 0x06, 0x00, 0x00, 0x08, 0x06, 0x00, 0x00, 0x15, 0x0a, 0x0a, 0x0a, 0x1b, 0x80, 0x80, 0x80, 0x34, 0x33, 0x41, 0x7e, 0x74, 0x5b, 0x9f, 0x5f, 0x1f, 0x04, 0x07,
	0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xef, 0xff, 0x23, 0x80, 0x29, 0x87, 0x3a, 0x01, 0x07, 0x31, 0x71, 0x8e, 0x8e, 0x8d, 0x53, 0x0e, 0x0e, 0x0e, 0x03, 0x00, 0x00, 0x00, 0x07, 0x1f,
	0xff, 0x1f, 0x0f, 0x18, 0x28, 0x27, 0x80, 0x3c, 0x32, 0x32, 0x71, 0x42, 0x1f, 0x18, 0x1f, 0x1e, 0x07, 0x1f, 0x07, 0x1f, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x0f, 0x1f, 0x0f, 0x1e, 0x80, 0x0c, 0x80,
	0x3c, 0x71, 0x72, 0x3f, 0x34, 0x8d, 0x52, 0x9f, 0x1f, 0x09, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x23, 0x08, 0x02, 0xf7, 0x15, 0x80, 0x1d, 0x87, 0x3d, 0x01, 0x01, 0x00, 0x00, 0x8e, 0x52,
	0x14, 0x4c, 0x08, 0x08, 0x0e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1b, 0x80, 0x80, 0x9b, 0x3a, 0x01, 0x01, 0x01, 0x02, 0x8d, 0x07, 0x07, 0x52, 0x09, 0x00, 0x00, 0x03, 0x01,
	0x02, 0x02, 0x00, 0x52, 0x02, 0x02, 0x28, 0x18, 0x22, 0x18, 0x80, 0x3c, 0x36, 0x31, 0x76, 0x71, 0x94, 0x9f, 0x96, 0x9f, 0x12, 0x00, 0x14, 0x0f, 0x04, 0x0a, 0x04, 0x0d, 0x2f, 0x0f, 0x4f, 0x2f,
	0x33, 0x80, 0x1a, 0x80, 0x34, 0x33, 0x41, 0x7e, 0x74, 0x5b, 0x9f, 0x5f, 0x1f, 0x04, 0x07, 0x07, 0x08, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xef, 0xff, 0x23, 0x90, 0x29, 0x97, 0x38, 0x63, 0x31,
	0x31, 0x31, 0x10, 0x13, 0x1a, 0x1b, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3f, 0x0f, 0x0f, 0x0f, 0x1a, 0x19, 0x1a, 0x80, 0x3a, 0x31, 0x25, 0x73, 0x41, 0x5f, 0x1f, 0x1f, 0x9c, 0x08,
	0x05, 0x04, 0x05, 0x03, 0x04, 0x02, 0x02, 0x2f, 0x2f, 0x1f, 0x2f, 0x29, 0x27, 0x1f, 0x80, 0x04, 0x71, 0x41, 0x31, 0x31, 0x12, 0x12, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x0f, 0x0f, 0x0f, 0x0f, 0x23, 0x80, 0x23, 0x80, 0x14, 0x75, 0x72, 0x35, 0x32, 0x9f, 0x9f, 0x9f, 0x9f, 0x05, 0x05, 0x00, 0x0a, 0x05, 0x05, 0x07, 0x05, 0x2f, 0xff, 0x0f, 0x2f, 0x1e, 0x80, 0x14,
	0x80, 0x3d, 0x01, 0x00, 0x01, 0x02, 0x12, 0x1f, 0x1f, 0x14, 0x07, 0x02, 0x02, 0x0a, 0x05, 0x05, 0x05, 0x05, 0x2f, 0x2f, 0x2f, 0xaf, 0x1c, 0x80, 0x82, 0x80, 0x1c, 0x73, 0x72, 0x33, 0x32, 0x94,
	0x99, 0x94, 0x99, 0x08, 0x0a, 0x08, 0x0a, 0x00, 0x05, 0x00, 0x05, 0x3f, 0x4f, 0x3f, 0x4f, 0x1e, 0x80, 0x19, 0x80, 0x31, 0x33, 0x01, 0x00, 0x00, 0x9f, 0x1f, 0x1f, 0x1f, 0x0d, 0x0a, 0x0a, 0x0a,
	0x0a, 0x07, 0x07, 0x07, 0xff, 0xaf, 0xaf, 0xaf, 0x1e, 0x1e, 0x1e, 0x80, 0x3a, 0x70, 0x76, 0x30, 0x71, 0x1f, 0x95, 0x1f, 0x1f, 0x0e, 0x0f, 0x05, 0x0c, 0x07, 0x06, 0x06, 0x07, 0x2f, 0x4f, 0x1f,
	0x5f, 0x21, 0x12, 0x28, 0x80, 0x28, 0x71, 0x00, 0x30, 0x01, 0x1f, 0x1f, 0x1d, 0x1f, 0x13, 0x13, 0x06, 0x05, 0x03, 0x03, 0x02, 0x05, 0x4f, 0x4f, 0x2f, 0x3f, 0x0e, 0x14, 0x1e, 0x80, 0x3e, 0x38,
	0x01, 0x7a, 0x34, 0x59, 0xd9, 0x5f, 0x9c, 0x0f, 0x04, 0x0f, 0x0a, 0x02, 0x02, 0x05, 0x05, 0xaf, 0xaf, 0x66, 0x66, 0x28, 0x80, 0xa3, 0x80, 0x39, 0x32, 0x31, 0x72, 0x71, 0x1f, 0x1f, 0x1f, 0x1f,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x0f, 0x0f, 0x1b, 0x32, 0x28, 0x80, 0x07, 0x34, 0x74, 0x32, 0x71, 0x1f, 0x1f, 0x1f, 0x1f, 0x0a, 0x0a, 0x05, 0x03, 0x00, 0x00, 0x00,
	0x00, 0x3f, 0x3f, 0x2f, 0x2f, 0x8a, 0x8a, 0x80, 0x80, 0x3a, 0x31, 0x37, 0x31, 0x31, 0x8d, 0x8d, 0x8e, 0x53, 0x0e, 0x0e, 0x0e, 0x03, 0x00, 0x00, 0x00, 0x00, 0x1f, 0xff, 0x1f, 0x0f, 0x17, 0x28,
	0x26, 0x80, 0x3b, 0x3a, 0x31, 0x71, 0x74, 0xdf, 0x1f, 0x1f, 0xdf, 0x00, 0x0a, 0x0a, 0x05, 0x00, 0x05, 0x05, 0x03, 0x0f, 0x5f, 0x1f, 0x5f, 0x32, 0x1e, 0x0f, 0x80, 0x05, 0x04, 0x01, 0x02, 0x04,
	0x8d, 0x1f, 0x15, 0x52, 0x06, 0x00, 0x00, 0x04, 0x02, 0x08, 0x00, 0x00, 0x1f, 0x0f, 0x0f, 0x2f, 0x16, 0x90, 0x84, 0x8c, 0x2c, 0x71, 0x74, 0x32, 0x32, 0x1f, 0x12, 0x1f, 0x12, 0x00, 0x0a, 0x00,
	0x0a, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x1f, 0x0f, 0x1f, 0x16, 0x80, 0x17, 0x80, 0x3a, 0x01, 0x07, 0x01, 0x01, 0x8e, 0x8e, 0x8d, 0x53, 0x0e, 0x0e, 0x0e, 0x03, 0x00, 0x00, 0x00, 0x07, 0x1f, 0xff,
	0x1f, 0x0f, 0x18, 0x28, 0x27, 0x8f, 0x36, 0x7a, 0x32, 0x51, 0x11, 0x1f, 0x1f, 0x59, 0x1c, 0x0a, 0x0d, 0x06, 0x0a, 0x07, 0x00, 0x02, 0x02, 0xaf, 0x5f, 0x5f, 0x5f, 0x1e, 0x8b, 0x81, 0x80, 0x3c,
	0x71, 0x72, 0x3f, 0x34, 0x8d, 0x52, 0x9f, 0x1f, 0x09, 0x00, 0x00, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x23, 0x08, 0x02, 0xf7, 0x15, 0x85, 0x1d, 0x8a, 0x3e, 0x77, 0x71, 0x32, 0x31, 0x1f, 0x1f, 0x1f,
	0x1f, 0x0d, 0x06, 0x00, 0x00, 0x08, 0x06, 0x00, 0x00, 0x15, 0x0a, 0x0a, 0x0a, 0x1b, 0x8f, 0x8f, 0x8f, 0x07, 0x34, 0x74, 0x32, 0x71, 0x1f, 0x1f, 0x1f, 0x1f, 0x0a, 0x0a, 0x05, 0x03, 0x00, 0x00,
	0x00, 0x00, 0x3f, 0x3f, 0x2f, 0x2f, 0x8a, 0x8a, 0x8a, 0x8a, 0x20, 0x36, 0x35, 0x30, 0x31, 0xdf, 0xdf, 0x9f, 0x9f, 0x07, 0x06, 0x09, 0x06, 0x07, 0x06, 0x06, 0x08, 0x20, 0x10, 0x10, 0xf8, 0x19,
	0x37, 0x13, 0x80
};



// 32500 is the "gap" when Z80 is blocked, at least it's this long in S3&K level select; the rest is just a guess
static const constexpr uint32 CYCLES_PER_FRAME = SoundDriver::MCYCLES_PER_FRAME - 32500 - 3000;


//#define VERIFY_AGAINST_DUMPS		// Enable verification against dumped data from Gensx
#ifndef VERIFY_AGAINST_DUMPS
	#define SOUNDDRIVER_FIX_BUGS	// Apply some of the bug fixes from the disassembly
#endif

//#define SOUNDDRIVER_DEBUG_CYCLES
#ifdef SOUNDDRIVER_DEBUG_CYCLES
	#define IMPLEMENT_CYCLES(x) RMX_ERROR("Implement cycles", );
#else
	#define IMPLEMENT_CYCLES(x)
#endif



// The main processor will access the following bytes in Z80 RAM:
//  0x1c00...0x1c0f get initialized from ROM address 0x001348 on
//  0x1c02 = Set to 1 for PAL, 0 for NTSC
//  0x1c08 = Music tempo, usually 0 or 8 (maybe other values as well in Blue Spheres)
//  0x1c0a = Current music ID
//  0x1c0b = Current sfx ID, slot 1
//  0x1c0c = Current sfx ID, slot 2
//  0x1c10 = Game paused flag (0x80 or 0)


// Constants
const uint8 zID_MusicPointers	= 0;
const uint8 zID_SFXPointers		= 2;
const uint8 zID_ModEnvPointers	= 4;
const uint8 zID_VolEnvPointers	= 6;

// ZRAM addresses
const uint16 zSpecialFreqCommands	 = 0x0272;
const uint16 zFMInstrumentRegTable	 = 0x049c;
const uint16 zFMInstrumentTLTable	 = 0x04b1;
const uint16 zFMDACInitBytes		 = 0x0695;
const uint16 zSFXChannelData		 = 0x07df;
const uint16 zSFXOverriddenChannel	 = 0x07ef;
const uint16 zPSGFrequencies		 = 0x0aa5;
const uint16 zFMFrequencies			 = 0x0b4d;
const uint16 z80_MusicBanks			 = 0x0b65;
const uint16 zPSGInitBytes			 = 0x06a3;
const uint16 z80_SoundDriverPointers = 0x1300 + 4;		// Modified to be compatible with original RAM dump
const uint16 z80_MusicPointers		 = 0x1618;
const uint16 z80_SFXPointers		 = 0x167e;
const uint16 z80_ModEnvPointers		 = 0x130e;
const uint16 z80_VolEnvPointers		 = 0x1387;
const uint16 zSpecFM3Freqs			 = 0x1bf0;
const uint16 zSpecFM3FreqsSFX		 = 0x1bf8;
const uint16 zTempVariablesStart	 = 0x1c0d;
const uint16 zSoundQueue0			 = 0x1c05;
const uint16 zTracksStart			 = 0x1c40;
const uint16 zSongFM6_DAC			 = zTracksStart + 0 * 0x30;
const uint16 zSongFM1				 = zTracksStart + 1 * 0x30;
const uint16 zSongPSG1				 = zTracksStart + 6 * 0x30;
const uint16 zSongPSG2				 = zTracksStart + 7 * 0x30;
const uint16 zSongPSG3				 = zTracksStart + 8 * 0x30;
const uint16 zTracksSFXStart		 = 0x1df0;
const uint16 zTracksSaveStart		 = 0x1df0;
const uint16 zTracksSaveEnd			 = 0x1fa0;


class Internal
{
public:
	Internal()
	{
		// Some sanity checks
		RMX_ASSERT(sizeof(zTrack) == 0x30, "");
		reset();
	}

	void setFixedContent(const uint8* data, uint32 size, uint32 offset)
	{
		mFixedContentData = data;
		mFixedContentSize = size;
		mFixedContentOffset = offset & 0x7fff;
	}

	void reset()
	{
		memset(mRam, 0, 0x2000);
		memcpy(mRam + 0x00d6, DACBanksData, sizeof(DACBanksData));
		memcpy(mRam + 0x049c, zFMInstrumentRegTableData, sizeof(zFMInstrumentRegTableData));
		memcpy(mRam + 0x0695, zFMDACInitBytesData, sizeof(zFMDACInitBytesData));
		memcpy(mRam + 0x07df, zSFXChannelDataData, sizeof(zSFXChannelDataData));
		memcpy(mRam + 0x0aa5, zPSGFrequenciesData, sizeof(zPSGFrequenciesData));
		memcpy(mRam + 0x1116, DecTableData, sizeof(DecTableData));
		memcpy(mRam + 0x1300, z80_SoundDriverPointersData, sizeof(z80_SoundDriverPointersData));

		zRingSpeaker = 1;		// 0x33 should be the right (not left) ring sound, as 0x34 is the right one already
		zSpindashRev = 0;

		zBankBaseAddress = 0;
		mCycles = 0;
		mFrameNumber = 0;
		mNumFramesCalculated = 0;
		mStopped = false;
		mSoundChipWritesThisFrame.clear();
		mSoundChipWritesCalculated.clear();

		mDACPlaybackState = DACPlaybackState::IDLE;
		mDACSampleLength = 0;
		mDACSampleDataPtr = 0;
		sample1_rate = 0;
		sample2_rate = 0;
		sample1_index = 0;
		sample2_index = 0;
	}

	SoundDriver::UpdateResult performSoundDriverUpdate()
	{
		mSoundChipWritesThisFrame.clear();
		mCycles = 0;
		if (mStopped)
			return SoundDriver::UpdateResult::STOP;

		// Are there still future frames that already got calculated?
		//  -> In that case there would be no need to to the update again right now
		if (mNumFramesCalculated == 0)
		{
			mSoundChipWritesCalculated.clear();

		#ifdef VERIFY_AGAINST_DUMPS
			static FileHandle file("../gensx/z80_ram_dumps.bin");
			{
				if (mFrameNumber == 0)
				{
					// Read initial ZRAM
					//  -> Not really used here, but it can be interesting to check for differences before actual verification
					uint8 zram[0x2000];
					file.read(zram, 0x2000);
				}

				if (mFrameNumber == 2)
				{
					_asm nop;
				}
			}
		#endif

			// Run V-Int
			zVInt();

			// If we got too many cycles, that means that V-Int took longer than one frame
			while (mCycles >= mNumFramesCalculated * CYCLES_PER_FRAME)
			{
				++mNumFramesCalculated;
			}

			// Run DAC update
			zPlayDigitalAudio();

		#ifdef VERIFY_AGAINST_DUMPS
			{
				// Read and compare ZRAM
				uint8 zram[0x2000];
				file.read(zram, 0x2000);

				for (int i = 0; i < 0x2000; ++i)
				{
					if (mRam[i] != zram[i] &&
						i >= 0x1116 &&					// Ignore Z80 code
						i < 0x1fe0 &&					// Exclude Z80 stack
						//i != 0x1c0a && i != 0x1c0b &&	// Exclude music and (first) sound number
						i != 0x1c17 &&					// Exclude that byte, it does not seem important
						i != 0x1c28)
					{
						_asm nop;
					}
				}

				// Read and compare sound chip writes
				static std::vector<SoundChipWrite> writesBuffer;
				{
					uint32 count = 0;
					file.read(&count, 4);
					writesBuffer.resize(count);
					for (uint32 k = 0; k < count; ++k)
					{
						SoundChipWrite& write = writesBuffer[k];
						file.read(&write.mTarget, 1);
						file.read(&write.mAddress, 1);
						file.read(&write.mData, 1);
						file.read(&write.mCycles, 4);
						file.read(&write.mLocation, 2);
						write.mFrameNumber = mFrameNumber;
					}
				}

				// Type 0: Non-DAC writes
				// Type 1: DAC writes
				static std::deque<SoundChipWrite> dumpedWrites[2];
				static std::deque<SoundChipWrite> calculatedWrites[2];
				static int writeCounter[2] = { 0, 0 };
				for (const SoundChipWrite& write : writesBuffer)
				{
					const int type = (write.mLocation >= 0x108a && write.mLocation < 0x1100) ? 1 : 0;
					dumpedWrites[type].push_back(write);
				}
				for (const SoundChipWrite& write : mSoundChipWritesCalculated)
				{
					const int type = (write.mLocation >= 0x108a && write.mLocation < 0x1100) ? 1 : 0;
					calculatedWrites[type].push_back(write);
					calculatedWrites[type].back().mFrameNumber = mFrameNumber;
				}
				for (int type = 0; type < 1; ++type)
				{
					while (!dumpedWrites[type].empty() && !calculatedWrites[type].empty())
					{
						const SoundChipWrite& dumpedWrite = dumpedWrites[type].front();
						const SoundChipWrite& calculatedWrite = calculatedWrites[type].front();
						RMX_CHECK(dumpedWrite == calculatedWrite, "Difference in sound chip write #" << writeCounter[type] << " of type " << type, );
						dumpedWrites[type].pop_front();
						calculatedWrites[type].pop_front();
						++writeCounter[type];
					}
					int diff = std::abs((int)dumpedWrites[type].size() - (int)calculatedWrites[type].size());
					RMX_CHECK(diff < 20, "Got in large difference (" << diff << ") of sound chip writes of type " << type, );
				}
				_asm nop;
			}
		#endif

			++mFrameNumber;
		}

		if (mNumFramesCalculated == 1)
		{
			mSoundChipWritesThisFrame = mSoundChipWritesCalculated;
		}
		else
		{
			// More than one frame calculated
			//  -> Find the position in the writes where the first frame ends
			size_t split = mSoundChipWritesCalculated.size();
			for (size_t i = 0; i < mSoundChipWritesCalculated.size(); ++i)
			{
				if (mSoundChipWritesCalculated[i].mCycles >= CYCLES_PER_FRAME)
				{
					split = i;
					break;
				}
			}

			for (size_t i = 0; i < split; ++i)
			{
				mSoundChipWritesThisFrame.push_back(mSoundChipWritesCalculated[i]);
			}
			for (size_t i = split; i < mSoundChipWritesCalculated.size(); ++i)
			{
				mSoundChipWritesCalculated[i].mCycles -= CYCLES_PER_FRAME;
				RMX_CHECK((mSoundChipWritesCalculated[i].mCycles & 0x80000000) == 0, "Negative cycles", );
			}
			mSoundChipWritesCalculated.erase(mSoundChipWritesCalculated.begin(), mSoundChipWritesCalculated.begin() + split);
		}
		--mNumFramesCalculated;

		return mStopped ? SoundDriver::UpdateResult::STOP : isStillPlaying() ? SoundDriver::UpdateResult::CONTINUE : SoundDriver::UpdateResult::FINISHED;
	}

	const std::vector<SoundChipWrite>& getSoundChipWrites() const
	{
		return mSoundChipWritesThisFrame;
	}

	void setMusic(uint8 musicId)
	{
		zMusicNumber = musicId;
	}

	void setSfx(uint8 sfxId)
	{
		if (zSFXNumber0 == 0)
		{
			zSFXNumber0 = sfxId;
		}
		else
		{
			zSFXNumber1 = sfxId;
		}
	}

	void setSourceAddress(uint32 sourceAddress)
	{
		mEnforcedSourceAddress = sourceAddress;
	}

	void setTempoSpeedup(uint8 tempoSpeedup)
	{
		zTempoSpeedup = tempoSpeedup;
	}

private:
	bool isStillPlaying() const
	{
		if (mStopped)
			return false;

		if (!mSoundChipWritesCalculated.empty())
			return true;

		// Check if any track is still playing
		const zTrack* tracks = (zTrack*)&mRam[zSongFM6_DAC];
		for (int i = 0; i < 16; ++i)
		{
			if (tracks[i].PlaybackControl & 0x80)
				return true;
		}

		return false;
	}

	uint8 read8(uint16 address)
	{
		if (address < 0x2000)
		{
			return mRam[address];
		}
		else if (address >= 0x8000)
		{
			if (nullptr == mFixedContentData || zBankBaseAddress != 0)
			{
				const uint32 fullAddress = zBankBaseAddress + (address & 0x7fff);
				return EmulatorInterface::instance().readMemory8(fullAddress);
			}
			else
			{
				const uint32 contentAddress = address - 0x8000 - mFixedContentOffset;
				RMX_CHECK(contentAddress < mFixedContentSize, "Invalid access to address " << rmx::hexString(contentAddress, 4) << " inside SMPS data", return 0);
				return mFixedContentData[contentAddress];
			}
		}
		else
		{
			RMX_ERROR("Invalid address", return 0);
		}
	}

	uint16 read16(uint16 address)
	{
		return read8(address) + ((uint16)read8(address + 1) << 8);
	}

	void writeFMI(uint8 reg, uint8 data, uint16 location = 0)
	{
		SoundChipWrite& write = vectorAdd(mSoundChipWritesCalculated);
		write.mTarget = SoundChipWrite::Target::YAMAHA_FMI;
		write.mAddress = reg;
		write.mData = data;
		write.mCycles = mCycles;
		write.mLocation = location;
	}

	void writeFMII(uint8 reg, uint8 data, uint16 location = 0)
	{
		SoundChipWrite& write = vectorAdd(mSoundChipWritesCalculated);
		write.mTarget = SoundChipWrite::Target::YAMAHA_FMII;
		write.mAddress = reg;
		write.mData = data;
		write.mCycles = mCycles;
		write.mLocation = location;
	}

	void writePSG(uint8 data, uint16 location = 0)
	{
		SoundChipWrite& write = vectorAdd(mSoundChipWritesCalculated);
		write.mTarget = SoundChipWrite::Target::SN76489;
		write.mData = data;
		write.mCycles = mCycles;
		write.mLocation = location;
	}

	void setzBank(uint32 baseAddress)
	{
		zBankBaseAddress = baseAddress & 0xff8000;
	}

	void bankswitch()
	{
		setzBank(((uint32)a) << 15);
	}

	void bankswitchToMusic()
	{
		a = zSongBank;
		bankswitch();
	}

	// Locations 0x0008 - 0x0014
	void GetPointerTable()
	{
		hl = read16(z80_SoundDriverPointers + c);
	}

	// Locations 0x0018 - 0x001f
	void PointerTableOffset()
	{
		hl = read16(hl + a * 2);
	}

	// Locations 0x0020 - 0x0024
	void ReadPointer()
	{
		hl = read16(hl);
	}

	// Locations 0x0038 - 0x0084
	void zVInt()
	{
		const uint16 backup_af = af;
		const uint16 backup_iy = iy;
		const uint16 backup_bc = bc;
		const uint16 backup_de = de;
		const uint16 backup_hl = hl;

		// At this point, there's a write access to address 0x1c17, but it's not present in the disassembly, so I guess it's not important

		mCycles = 1095;
		zUpdateEverything();

		// PAL-specific parts are skipped here

		hl = 0x00d6 + (zDACIndex & 0x7f);	// 0x00d6 is start of "DAC_Banks"
		a = read8(hl);

		bankswitch();

		af = backup_af;
		iy = backup_iy;
		bc = backup_bc;
		de = backup_de;
		hl = backup_hl;
		b = 1;
		mCycles += 3570;
	}

	// Locations 0x00af - ?
	void zWriteFMIorII()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.VoiceControl & 0x80)		// PSG track?
		{
			mCycles += 465;
			return;
		}
		if (track.PlaybackControl & 0x04)	// SFX override?
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		mCycles += 750;
		a += track.VoiceControl;
		if (track.VoiceControl & 0x04)
		{
			a -= 4;		// Throw away the 4 again
			mCycles += 1185;
			writeFMII(a, c, 0x00d2);
		}
		else
		{
			mCycles += 1005;
			writeFMI(a, c, 0x00c7);
		}
		mCycles += 345;
	}

	// Locations 0x00c2 - 0x00ca
	void zWriteFMI()
	{
		writeFMI(a, c, 0x00c7);
	}

	void zWriteFMI(uint8 reg, uint8 data)
	{
		writeFMI(reg, data, 0x00c7);
	}

	// Locations 0x00cb - 0x00cb
	void zWriteFMII_reduced()
	{
		a -= 4;
		zWriteFMII();
	}

	// Locations 0x00cd - 0x00d5
	void zWriteFMII()
	{
		writeFMII(a, c, 0x00d2);
	}

	// Locations 0x011b - 0x011e
	void zUpdateEverything()
	{
		//zPauseUnpause();		// We will happily ignore that

		mCycles += 990;
		zUpdateSFXTracks();

		mCycles += 255;
		zUpdateMusic();
	}

	// Locations 0x0121 - 0x019c
	void zUpdateMusic()
	{
		TempoWait();

		mCycles += 255;
		zDoMusicFadeOut();

		mCycles += 255;
		zDoMusicFadeIn();

		mCycles += 195;
		if (zFadeToPrevFlag == 0x2a)	// Is it the 1-up jingle?
		{
			a = zMusicNumber;
			if (a == 0x2a)			// Another 1-up enqueued?
			{
				zMusicNumber = 0;
			}
			else if (a < 0x33)		// Is it music?
			{
				zMusicNumber = 0;
			}
			zSFXNumber0 = 0;
			zSFXNumber1 = 0;
		}
		else if (zFadeToPrevFlag != 0xff)
		{
			mCycles += 285;
			if (zMusicNumber != 0 || zSFXNumber0 != 0 || zSFXNumber1 != 0)
			{
				mCycles += 1530;
				zFillSoundQueue();

				for (int i = 0; i < 3; ++i)
				{
					mCycles += 255;
					zCycleSoundQueue();
				}
			}
			else
			{
				mCycles += 1350;
			}
		}
		else
		{
			IMPLEMENT_CYCLES("mCycles += ?");
		}

		// .update_music:
		bankswitchToMusic();

		zUpdatingSFX = 0;
		a = zFadeToPrevFlag;
		mCycles += 2220;
		if (a == 0xff)
		{
		#if 0
			RMX_ERROR("Not implemented", );
			zFadeInToPrevious();
		#else
			mStopped = true;
		#endif
			return;
		}

		ix = zSongFM6_DAC;
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.PlaybackControl & 0x80)
		{
			mCycles += 1275;
			zUpdateDACTrack();
		}
		else
		{
			mCycles += 915;
		}

		b = 8;		// Number of tracks from FM1 on
		ix = zSongFM1;
		mCycles += 495;
		zTrackUpdLoop();
	}

	// Locations 0x019e - 0x01bd
	void zUpdateSFXTracks()
	{
		zUpdatingSFX = 1;
		a = 0x1f;
		bankswitch();

		// Go through all 7 SFX tracks; it's 4 FM + 3 PSG
		ix = zTracksSFXStart;
		b = 7;		// Number of tracks

		mCycles += 2295;
		zTrackUpdLoop();
	}

	// Locations 0x01bf - 0x01e8
	void zTrackUpdLoop()
	{
		const int loops = b;
		for (int i = 0; i < loops; ++i)
		{
			zTrack* track = (zTrack*)&mRam[ix];
			if (track->PlaybackControl & 0x80)
			{
				mCycles += 720;
				zUpdateFMorPSGTrack();
			}
			else
			{
				mCycles += 615;
			}

			ix += sizeof(zTrack);
			mCycles += 720;
		}

		mCycles += 345;
		if (zTempoSpeedup)
		{
			if (zSpeedupTimeout > 0)
			{
				--zSpeedupTimeout;
			}
			else
			{
				zSpeedupTimeout = zTempoSpeedup;

				// A second music update
				zUpdateMusic();
			}
			IMPLEMENT_CYCLES("mCycles += ?");
		}
	}

	// Locations 0x01e9 - 0x0228
	void zUpdateFMorPSGTrack()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		mCycles += 300;
		if (track.VoiceControl & 0x80)
		{
			mCycles += 405;
			zUpdatePSGTrack();
		}
		else
		{
			--track.DurationTimeout;
			if (track.DurationTimeout == 0)
			{
				mCycles += 1545;
				zGetNextNote();

				// Check playback of this track if just stopped (this part is not needed in original code)
				if ((track.PlaybackControl & 0x80) == 0)
					return;

				if (track.PlaybackControl & 0x10)	// Track is resting?
				{
					mCycles += 465;
					return;
				}

				mCycles += 630;
				zPrepareModulation();

				mCycles += 255;
				zUpdateFreq();

				mCycles += 255;
				if (zDoModulation())
					return;

				mCycles += 255;
				zFMSendFreq();

				mCycles += 150;
				zFMNoteOn();
			}
			else
			{
				// .note_going:
				mCycles += 1365;
				if (track.PlaybackControl & 0x10)	// Track is resting?
				{
					mCycles += 465;
					return;
				}

				mCycles += 375;
				zDoFMVolEnv();

				if (track.NoteFillTimeout != 0)
				{
					--track.NoteFillTimeout;
					if (track.NoteFillTimeout == 0)
					{
						IMPLEMENT_CYCLES("mCycles += ?");
						zKeyOffIfActive();
						return;
					}
					else
					{
						IMPLEMENT_CYCLES("mCycles += ?");
					}
				}
				else
				{
					mCycles += 1290;
				}

				mCycles += 255;
				zUpdateFreq();

				if ((track.PlaybackControl & 0x40) == 0)	// Sustain frequency bit clear?
				{
					mCycles += 630;
					if (zDoModulation())
						return;

					zFMSendFreq();
				}
				else
				{
					IMPLEMENT_CYCLES("mCycles += ?");
				}
			}
		}
	}

	// Locations 0x022b - (around) 0x0270
	void zFMSendFreq()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.PlaybackControl & 0x04)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		if (track.PlaybackControl & 0x01)	// Special mode?
		{
			// .special_mode:
			IMPLEMENT_CYCLES("mCycles += ?");
			if (track.VoiceControl == 2)	// FM3?
			{
				IMPLEMENT_CYCLES("mCycles += ?");
				zGetSpecialFM3DataPointer();
				b = 4;		// Number of special frequencies
				hl = zSpecialFreqCommands;

				while (b > 0)
				{
					const uint16 backup_bc = bc;
					a = read8(hl);
					++hl;

					const uint16 backup_hl = hl;
					bc = read16(de);
					de += 2;

					l = track.FreqLow;
					h = track.FreqHigh;
					hl += bc;

					zWriteFMI(a, h);
					a -= 4;
					zWriteFMI(a, l);

					hl = backup_hl;
					bc = backup_bc;
					--b;
				}
			}
			return;
		}

		// .not_fm3:
		// Write to channel
		a = 0xa4;
		c = h;
		mCycles += 1245;
		zWriteFMIorII();

		a = 0xa0;
		c = l;
		mCycles += 420;
		zWriteFMIorII();

		mCycles += 150;
	}

	void zGetSpecialFM3DataPointer()
	{
		de = zSpecFM3Freqs;
		a = zUpdatingSFX;
		if (a == 0)
			return;

		de = zSpecFM3FreqsSFX;
	}

	// Locations 0x0277 - 0x0281
	void zGetNextNote()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		e = track.DataPointerLow;
		d = track.DataPointerHigh;
		track.PlaybackControl &= ~0x02;
		track.PlaybackControl &= ~0x10;
		mCycles += 1260;

		zGetNextNote_cont();
	}

	// Locations 0x0285 - ?
	void zGetNextNote_cont()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		uint16 backup;
		bool gotNoteDuration = false;

		a = read8(de);
		++de;
		if (a >= 0xe0)
		{
			mCycles += 450;
			zHandleFMorPSGCoordFlag();
			return;
		}

		backup = af;
		mCycles += 765;
		zKeyOffIfActive();
		af = backup;

		mCycles += 60;
		if ((track.PlaybackControl & 0x08) == 0)
		{
			if ((a & 0x80) == 0)		// Is this a duration?
			{
				mCycles += 915;
				zComputeNoteDuration();

				track.SavedDuration = a;
				mCycles += 285;
			}
			else
			{
				a -= 0x81;		// Make the note into a 0-based index
				mCycles += 765;
				if ((int8)a < 0)
				{
					mCycles += 405;
					zRestTrack();
					mCycles += 180;
				}
				else
				{
					// .got_note:
					a += track.Transpose;
					hl = zPSGFrequencies;

					backup = af;
					PointerTableOffset();
					af = backup;

					mCycles += 2550;
					if ((track.VoiceControl & 0x80) == 0)	// Is PSG track?
					{
						const int8 octave = a / 12;
						const int8 note = a % 12;

						a = note;
						hl = zFMFrequencies;
						PointerTableOffset();

						a = (octave * 8) | h;
						h = a;
						mCycles += 2790 + 525 * octave;
					}
					else
					{
						mCycles += 180;
					}

					// zGotNoteFreq:
					track.FreqLow = l;
					track.FreqHigh = h;
					mCycles += 570;
				}

				// zGetNoteDuration:
				a = read8(de);
				gotNoteDuration = (a & 0x80) == 0;
				mCycles += (gotNoteDuration) ? 315 : 1065;
			}
		}
		else
		{
			// zAltFreqMode:
			IMPLEMENT_CYCLES("mCycles += ?");
			h = a;
			a = read8(de);
			++de;
			l = a;
			if (h != 0)
			{
				a = track.Transpose;
				hl += (int8)a;
			}

			track.FreqLow = l;
			track.FreqHigh = h;
			++de;
			a = read8(de);

			gotNoteDuration = true;
		}

		if (gotNoteDuration)
		{
			// zGotNoteDuration:
			++de;

			// zStoreDuration:
			mCycles += 345;
			zComputeNoteDuration();

			track.SavedDuration = a;
			mCycles += 285;
		}

		zFinishTrackUpdate();
	}

	// Locations 0x030e - 0x032f
	void zFinishTrackUpdate()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		track.DataPointerLow = e;
		track.DataPointerHigh = d;
		track.DurationTimeout = track.SavedDuration;
		mCycles += 1440;

		if (track.PlaybackControl & 0x02)
		{
			mCycles += 165;
			return;
		}

		track.ModulationSpeed = 0;
		track.ModulationValLow = 0;
		track.VolEnv = 0;
		track.NoteFillTimeout = track.NoteFillMaster;
		mCycles += 1710;
	}

	// Locations 0x0330 - (around) 0x0339
	void zComputeNoteDuration()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		a *= track.TempoDivider;
		mCycles += 510;		// This value is only valid for (track.TempoDivider == 1)
	}

	// Locations 0x0342 - ?
	void zFMNoteOn()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.FreqLow == 0 && track.FreqHigh == 0)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

	#ifdef SOUNDDRIVER_FIX_BUGS
		if (track.PlaybackControl & 0x16)
	#else
		if (track.PlaybackControl & 0x06)
	#endif
		{
			mCycles += 1200;
			return;
		}

		// Set operator
		c = track.VoiceControl | 0xf0;
		mCycles += 2235;
		zWriteFMI(0x28, c);

		mCycles += 495;
	}

	// Locations 0x035b - 0x0360
	void zKeyOffIfActive()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		a = track.PlaybackControl;
		if (a & 0x06)
		{
			mCycles += 555;
		}
		else
		{
			mCycles += 465;
			zKeyOff();
		}
	}

	// Locations 0x0361 - 0x0366
	void zKeyOff()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		c = track.VoiceControl;
		if (c & 0x80)
		{
			mCycles += 570;
			return;
		}

		mCycles += 480;
		zKeyOnOff();
	}

	// Locations 0x0367 - 0x036c
	void zKeyOnOff()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.PlaybackControl &= ~0x40;		// Fix not present in original

		// Set operator
		mCycles += 675;
		zWriteFMI(0x28, c);		// Data parameter = c must have been set before
		mCycles += 495;
	}

	// Locations 0x036d - ?
	void zDoFMVolEnv()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		a = track.FMVolEnv;
		if ((int8)a <= 0)
			return;

		--a;
		c = zID_VolEnvPointers;
		GetPointerTable();
		PointerTableOffset();
		zDoVolEnv();

		h = track.TLPtrHigh;
		l = track.TLPtrLow;
		de = zFMInstrumentTLTable;
		b = 4;		// Number of entries in zFMInstrumentTLTable
		c = track.FMVolEnvMask;

		while (b > 0)
		{
			const uint16 backup_af = af;
			bool flag = (c & 1) != 0;
			c >>= 1;

			if (flag)
			{
				const uint16 backup_bc = bc;

				a += read8(hl);
				c = a & 0x7f;
				a = read8(de);
				zWriteFMIorII();

				bc = backup_bc;
			}

			++de;
			++hl;
			af = backup_af;
			--b;
		}
	}

	// Locations 0x039e - 0x03c8
	void zPrepareModulation()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if ((track.ModulationCtrl & 0x80) == 0)
		{
			mCycles += 465;
			return;
		}
		if ((track.PlaybackControl & 0x02) != 0)
		{
			mCycles += 840;
			return;
		}

		e = track.ModulationPtrLow;
		d = track.ModulationPtrHigh;
		hl = ix + 0x24;		// Pointer to track.ModulationWait

		std::swap(de, hl);
		mRam[de]   = read8(hl);
		mRam[de+1] = read8(hl+1);
		mRam[de+2] = read8(hl+2);
		de += 3;
		hl += 3;
		a = read8(hl) / 2;
		mRam[de] = a;

		track.ModulationValLow = 0;
		track.ModulationValHigh = 0;
		mCycles += 3960;
	}

	// Locations 0x03c9 - ?
	bool zDoModulation()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		a = track.ModulationCtrl;
		if (a == 0)
		{
			mCycles += 510;
			return false;
		}

		if (a == 0x80)
		{
			if (track.ModulationWait != 1)
			{
				--track.ModulationWait;
				mCycles += 1140;
				return false;
			}

			mCycles += 1395;
			const uint16 backup_hl = hl;
			l = track.ModulationValLow;
			h = track.ModulationValHigh;
			e = track.ModulationPtrLow;
			d = track.ModulationPtrHigh;
			iy = de;

			--track.ModulationSpeed;
			if (track.ModulationSpeed == 0)
			{
				a = read8(iy + 1);
				track.ModulationSpeed = a;
				a = track.ModulationDelta;
				hl += (int8)a;
				track.ModulationValLow = l;
				track.ModulationValHigh = h;
				mCycles += 4125;
			}
			else
			{
				mCycles += 2205;
			}

			bc = backup_hl;
			hl += bc;
			--track.ModulationSteps;
			if (track.ModulationSteps != 0)
			{
				mCycles += 825;
				return false;
			}

			track.ModulationSteps = read8(iy + 3);
			a = -track.ModulationDelta;
			track.ModulationDelta = a;
			mCycles += 2145;
			return false;
		}
		else
		{
			// zDoModEnvelope:
			--a;
			std::swap(hl, de);
			c = zID_ModEnvPointers;
			GetPointerTable();
			PointerTableOffset();

			mCycles += 3765;
			while (true)
			{
				// zDoModEnvelope_cont:

				bc = track.ModEnvIndex;
				a = read8(hl + bc);

				if ((a & 0x80) == 0)
				{
					// zlocPositiveModEnvMod:
					h = 0;
					mCycles += 1350;
					zlocApplyModEnvMod();
					return false;
				}

				if (a == 0x82)	// Change mod env index
				{
					// zlocChangeModEnvIndex:
					++bc;
					a = read8(bc);
					track.ModEnvIndex = a;
					mCycles += 2190;
					continue;
				}

				if (a == 0x80)	// Reset mod env index
				{
					a = 0;
					track.ModEnvIndex = a;
					IMPLEMENT_CYCLES("mCycles += ?");
					continue;
				}

				if (a == 0x84)	// Env inc multiplier
				{
					++bc;
					a = read8(bc);
					track.ModEnvSens += a;
					track.ModEnvIndex += 2;
					IMPLEMENT_CYCLES("mCycles += ?");
					continue;
				}

				h = 0xff;
				if (a > 0x84)
				{
					mCycles += 2160;
					zlocApplyModEnvMod();
					return false;
				}

				mCycles += 2730;	// Incl. return
				break;
			}

			track.PlaybackControl |= 0x40;
			return true;	// Skip the rest of caller
		}
	}

	// Locations 0x0462 - ?
	void zlocApplyModEnvMod()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		l = a;
		b = track.ModEnvSens + 1;
		std::swap(de, hl);

		hl += de * b;
		++track.ModEnvIndex;
		mCycles += 1245;
	}

	// Locations 0x046f - 0x0482
	void zUpdateFreq()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		h = track.FreqHigh;
		l = track.FreqLow;
		hl += (int8)track.Detune;
		mCycles += ((int8)track.Detune < 0) ? 1650 : 1545;
	}

	// Locations 0x0483 - (around) 0x0491
	void zGetFMInstrumentPointer()
	{
		if (zUpdatingSFX == 0)
		{
			hl = zVoiceTblPtr;
			mCycles += 675;
		}
		else
		{
			zTrack& track = *(zTrack*)&mRam[ix];
			l = track.VoicesLow;
			h = track.VoicesHigh;
			mCycles += 1170;
		}
		zGetFMInstrumentOffset();
	}

	// Locations 0x0492 - (around) 0x049b
	void zGetFMInstrumentOffset()
	{
		a = b;
		hl += 25 * b;	// 25 = size of each FM instrument
		mCycles += (b == 0) ? 285 : (420 + b * 360);
	}

	// Locations 0x04b9 - ?
	void zSendFMInstrument()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		de = zFMInstrumentRegTable;
		c = track.AMSFMSPan;
		a = 0xb4;				// Select AMS/FMS/panning register
		mCycles += 795;
		zWriteFMIorII();

		mCycles += 255;
		zSendFMInstrData();
		track.FeedbackAlgo = a;

		// These are some larger code fixes (it's usually just one loop calling "zSendFMInstrData" for all 20 instruments)
		mCycles += 645;
		for (int i = 0; i < 4; ++i)
		{
			zSendFMInstrData();
			mCycles += 450;
		}
		for (int i = 0; i < 4; ++i)
		{
			zSendFMInstrDataRSAR();
			mCycles += 450;
		}
		for (int i = 0; i < 12; ++i)
		{
			zSendFMInstrData();
			mCycles += 450;
		}

		track.TLPtrLow = l;
		track.TLPtrHigh = h;

		mCycles += 390;
		zSendTL();
	}

	// Locations 0x04da - ?
	void zSendFMInstrData()
	{
		a = read8(de);
		++de;
		c = read8(hl);
		++hl;
		mCycles += 645;
		zWriteFMIorII();

		a = c;		// Needed inside zSendFMInstrument
		mCycles += 150;
	}

	void zSendFMInstrDataRSAR()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		a = track.HaveSSGEGFlag;
		if (a & 0x80)
		{
			a = read8(de);
			++de;
			c = read8(hl) | 0x1f;
			++hl;
			mCycles += 645;
			zWriteFMIorII();

			mCycles += 150;
		}
		else
		{
			zSendFMInstrData();
		}
	}

	// Locations 0x04e2 - ?
	void zCycleSoundQueue()
	{
		zNextSound = read8(zSoundQueue0);
		mRam[zSoundQueue0]     = mRam[zSoundQueue0 + 1];
		mRam[zSoundQueue0 + 1] = mRam[zSoundQueue0 + 2];
		mRam[zSoundQueue0 + 2] = 0;
		a = zNextSound;

		mCycles += 1620;
		zPlaySoundByIndex();
	}

	// Locations 0x04fb - ?
	void zPlaySoundByIndex()
	{
		// This check is done differently (towards the end) in original
		if (a == 0)
		{
			mCycles += 1035;
			return;
		}

		if (a == 0xdc)
		{
			mCycles += 255;
			zPlayMusicCredits();
		}
		else if (a == 0xff)
		{
			RMX_ERROR("Not implemented", );
			//zPlaySegaSound();
		}
		else if (a < 0x33)
		{
			mCycles += 765;
			zPlayMusic();
		}
		else if (a < 0xe0)
		{
			mCycles += 1020;
			zPlaySound_CheckRing();
		}
		else if (a == 0xe0 || a >= 0xe6)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			zStopAllSound();
		}
		else
		{
			// TODO
/*
			a -= 0xe1;
			hl = zFadeEffects;
			PointerTableOffset();
			call hl;
*/
		}
	}

	// Locations 0x0552 - ?
	void zPlayMusicCredits()
	{
		a = 0x32;
		const uint16 backup_af = af;

		mCycles += 675;
		zStopAllSound();
		zBGMLoad(backup_af);
	}

	// Locations 0x0558 - ?
	void zPlayMusic()
	{
		const uint8 soundId = a;
		--a;	// Remap music index to start from 0 for AIZ 1 music
		const uint16 backup_af = af;
		if (soundId == 0x2a)		// 1-up jingle
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			if (zFadeInTimeout != 0)
			{
				zMusicNumber = 0;
				zSFXNumber0 = 0;
				zSFXNumber1 = 0;
				zSoundQueueEntry0 = 0;
				zSoundQueueEntry1 = 0;
				zSoundQueueEntry2 = 0;
				zNextSound = 0;
				af = backup_af;
				return;
			}

			a = zFadeToPrevFlag;
			if (zFadeToPrevFlag != 0x2a)
			{
				zMusicNumber = 0;
				zSFXNumber0 = 0;
				zSFXNumber1 = 0;
				zSoundQueueEntry0 = 0;
				zSoundQueueEntry1 = 0;
				zSoundQueueEntry2 = 0;
				zSongBankSave = zSongBank;
				zTempoSpeedupSave = zTempoSpeedup;
				zTempoSpeedup = 0;

				hl = zTracksStart;
				de = zTracksSaveStart;
				bc = 9 * sizeof(zTrack);
				while (bc-- > 0)
				{
					mRam[de] = read8(hl);
					++de;
					++hl;
				}

				hl = zTracksStart;
				de = sizeof(zTrack);
				b = 9;
				while (b > 0)
				{
					mRam[hl] = (read8(hl) & 0x7f) | 0x04;
					hl += de;
					--b;
				}

				zFadeToPrevFlag = 0x2a - 1;
				zCurrentTempoSave = zCurrentTempo;
				hl = zVoiceTblPtr;
				zVoiceTblPtrSave = hl;
			}
		}
		else
		{
			// zPlayMusic_DoFade:
			mCycles += 855;
			zStopAllSound();
		}

		zBGMLoad(backup_af);
	}

	// Locations 0x05de - ?
	void zBGMLoad(uint16 backup_af)
	{
		af = backup_af;

		hl = z80_MusicBanks;	// Table of music banks
		hl += a;
		a = read8(hl);

	#ifdef VERIFY_AGAINST_DUMPS
		*(uint16*)&mRam[0x05ec] = hl;	// Actually only needed for verification (thanks to self-modifying code in the original)
	#endif

		zSongBank = a;
		bankswitch();

		// Set channel 6 L+R
		mCycles += 3435;
		writeFMII(0xb6, 0xc0);
		af = backup_af;

		c = zID_MusicPointers;
		GetPointerTable();
		PointerTableOffset();

		if (nullptr != mFixedContentData)
		{
			zBankBaseAddress = 0;
			zSongBank = 0;
			hl = 0x8000 + (uint16)mFixedContentOffset;
		}
		else if (mEnforcedSourceAddress != 0)
		{
			// Ignore the music pointer and use a custom source address (e.g. to play Sonic 3 track versions directly from the ROM, or tracks from other sources)
			zBankBaseAddress = mEnforcedSourceAddress & 0xff8000;
			zSongBank = (uint8)(zBankBaseAddress >> 15);
			hl = (mEnforcedSourceAddress & 0x7fff) | 0x8000;
		}

		const uint16 backup_hl = hl;
		ReadPointer();
		zVoiceTblPtr = hl;
		hl = backup_hl;
		iy = backup_hl;

		// iy and hl now point to the SMPS header (see https://segaretro.org/SMPS/Headers )
		a = read8(iy + 5);		// Main tempo value
		zTempoAccumulator = a;
		zCurrentTempo = a;

		hl += 6;
		zSongPosition = hl;

		zTrackInitPos = zFMDACInitBytes;
		de = zTracksStart;
		b = read8(iy + 2);		// Number of FM + DAC channels
		a = read8(iy + 4);		// Tempo divider
		mCycles += 7050;

		// .fm_dac_loop:
		while (b > 0)
		{
			const uint16 backup_bc = bc;

			hl = zTrackInitPos;
			mRam[de]   = read8(hl);
			mRam[de+1] = read8(hl+1);
			mRam[de+2] = a;
			hl += 2;
			de += 3;
			zTrackInitPos = hl;

			hl = zSongPosition;
			mRam[de]   = read8(hl);
			mRam[de+1] = read8(hl+1);
			mRam[de+2] = read8(hl+2);
			mRam[de+3] = read8(hl+3);
			hl += 4;
			de += 4;
			zSongPosition = hl;

			mCycles += 2760+255;
			zInitFMDACTrack();

			bc = backup_bc;
			--b;
			mCycles += 345;
		}

		a = read8(iy + 3);		// Number of PSG channels
		mCycles += 210;
		if (a != 0)
		{
			b = a;
			zTrackInitPos = zPSGInitBytes;
			de = zSongPSG1;
			a = read8(iy + 4);		// Tempo divider (again)
			mCycles += 1095;

			while (b > 0)
			{
				const uint16 backup_bc = bc;

				hl = zTrackInitPos;
				mRam[de]   = read8(hl);
				mRam[de+1] = read8(hl+1);
				mRam[de+2] = a;
				hl += 2;
				de += 3;
				zTrackInitPos = hl;

				hl = zSongPosition;
				mRam[de]   = read8(hl);
				mRam[de+1] = read8(hl+1);
				mRam[de+2] = read8(hl+2);
				mRam[de+3] = read8(hl+3);
				mRam[de+4] = read8(hl+4);
				mRam[de+5] = read8(hl+5);
				hl += 6;
				de += 6;
				zSongPosition = hl;

				mCycles += 4020;
				zZeroFillTrackRAM();

				bc = backup_bc;
				--b;
				mCycles += 345;
			}
		}

		zNextSound = 0;
		mCycles += 330;		// TODO: This includes part of the while loop evaluation in the if-block
	}

	// Locations 0x0690 - 0x0694
	void zClearNextSound()
	{
		zNextSound = 0;
	}

	// Locations 0x06a9 - ?
	void zPlaySound_CheckRing()
	{
		const uint8 sfxId = a;

		a -= 0x33;
		if (sfxId == 0x33)		// Check for alternating ring sound
		{
			zRingSpeaker ^= 1;
			a = zRingSpeaker;
		}
		else
		{
			mCycles += 315;
		}

		// zPlaySound_Bankswitch:
		uint16 backup_af = af;
		a = 0x1f;
		bankswitch();

		c = zID_SFXPointers;
		zUpdatingSFX = 0;
		af = backup_af;

		mCycles += 2160;
		if (sfxId == 0xab)		// Spindash sound
		{
			// Do nothing special
			mCycles += 255;
		}
		else if (sfxId < 0xbc)	// Normal (non-continuous) sound?
		{
			zSpindashRev = 0;
			mCycles += 1080;
		}
		else
		{
			mCycles += 510;
			backup_af = af;
			b = a;
			a = zContinuousSFX;
			if (a != b)
			{
				zContinuousSFXFlag = 0;
				af = backup_af;
				zContinuousSFX = a;
				mCycles += 1380;
			}
			else
			{
				IMPLEMENT_CYCLES("mCycles += ?");
				a = 0x80;
				zContinuousSFXFlag = 0x80;
				GetPointerTable();
				af = backup_af;
				PointerTableOffset();

				hl += 3;
				a = read8(hl);
				zContSFXLoopCnt = a;

				zNextSound = 0;
				return;
			}
		}

		// zPlaySound:
		GetPointerTable();
		PointerTableOffset();
		{
			const uint16 backup_hl = hl;
			ReadPointer();
			zSFXVoiceTblPtr = hl;
		#ifndef SOUNDDRIVER_FIX_BUGS
			mRam[0x1c15] = 0;
		#endif
			hl = backup_hl;
			iy = hl;
		}

		a = read8(iy + 2);
		zSFXTempoDivider = a;
		de = 4;
		hl += 4;
		b = read8(iy + 3);
		a = b;
		zContSFXLoopCnt = a;

		// zSFXTrackInitLoop:
		mCycles += 5850;
		while (b > 0)
		{
			const uint16 backup_bc = bc;
			uint16 backup_hl = hl;

			++hl;
			c = read8(hl);
			mCycles += 780;
			zGetSFXChannelPointers();
			mRam[hl] |= 0x04;

			de = ix;
			hl = backup_hl;

			mRam[de] = read8(hl);
			++de;
			++hl;
			a = read8(de);
			if (a == 2)		// Is this FM3?
			{
				zFM3NormalMode();
			}
			else
			{
				mCycles += 1785;
			}

			mRam[de] = read8(hl);
			++de;
			++hl;

			a = zSFXTempoDivider;
			mRam[de] = a;
			++de;
			mRam[de]   = read8(hl);
			mRam[de+1] = read8(hl+1);
			mRam[de+2] = read8(hl+2);
			mRam[de+3] = read8(hl+3);
			de += 4;
			hl += 4;

			mCycles += 1845;
			zInitFMDACTrack();

			backup_hl = hl;
			hl = zSFXVoiceTblPtr;

			zTrack& track = *(zTrack*)&mRam[ix];
			track.VoicesLow = l;
			track.VoicesHigh = h;

			mCycles += 2820;
			zKeyOffIfActive();

		#ifdef SOUNDDRIVER_FIX_BUGS
			if (track.VoiceControl & 0x80)	// The check is added as a fix in disassembly, but it breaks verification
		#endif
			{
				mCycles += 255;
				zFMClearSSGEGOps();
			}

			hl = backup_hl;
			bc = backup_bc;
			--b;
			mCycles += 420;
		}

		zNextSound = 0;
		mCycles += 555;
	}

	// Locations 0x078f - ?
	void zGetSFXChannelPointers()
	{
		if ((c & 0x80) == 0)	// Check if this is a PSG track (bit clear = no)
		{
			a = c - 2;
			if (c & 0x04)		// Remove gap between FM3 and FM4
			{
				--a;
				mCycles += 165;
			}
			mCycles += 690;
		}
		else
		{
			mCycles += 660;
			zSilencePSGChannel();

			mCycles += 105;
		#ifndef SOUNDDRIVERFIX_BUGS
			writePSG(0xff);
		#endif

			a = (c >> 5) & 0x07;	// No +1 here as in the disassembly, as we don't have the "filler" in zSFXChannelData
			mCycles += 1065;
		}

		// .get_ptrs:
		zSFXSaveIndex = a;

		const uint16 backup_af = af;
		hl = zSFXChannelData;
		PointerTableOffset();
		ix = hl;

		af = backup_af;
		hl = zSFXOverriddenChannel;
		PointerTableOffset();

		mCycles += 4035;
	}

	// Locations 0x07c5 - ?
	void zInitFMDACTrack()
	{
		mRam[de] = 0;
		mRam[de+1] = 0;
		de += 2;

		mCycles += 570;
		zZeroFillTrackRAM();
	}

	// Locations 0x07cc - ?
	void zZeroFillTrackRAM()
	{
		mRam[de]   = 0x30;	// sizeof(zTrack)
		mRam[de+1] = 0xc0;	// Default Panning / AMS / FMS settings (only stereo L/R enabled)
		mRam[de+2] = 1;		// Current note duration timeout
		de += 3;
		memset(&mRam[de], 0, 0x24);		// Clear everything from track.DurationTimeout on
		de += 0x24;
		mCycles += 16680;
	}

	// Locations 0x0869 - ?
	void zHaltDACPSG()
	{
		a = 0;
		mRam[zSongFM6_DAC] = 0;
		mRam[zSongPSG1] = 0;
		mRam[zSongPSG2] = 0;
		mRam[zSongPSG3] = 0;

		zPSGSilenceAll();
	}

	// Locations 0x0879 - ?
	void zDoMusicFadeOut()
	{
		hl = 0x1c00;		// Address of zFadeOutTimeout
		mCycles += 480;
		if (zFadeOutTimeout == 0)
			return;

		if ((int8)zFadeOutTimeout < 0)
		{
			zHaltDACPSG();
		}

		zFadeOutTimeout &= 0x7f;
		if (zFadeDelayTimeout > 1)
		{
			--zFadeDelayTimeout;
			return;
		}

		// Timer expired
		a = zFadeDelay;
		zFadeDelayTimeout = a;

		hl = 0x1c00;		// Address of zFadeOutTimeout
		--zFadeOutTimeout;
		if (zFadeOutTimeout == 0)
		{
			zStopAllSound();
			return;
		}

		bankswitchToMusic();
		ix = zTracksStart;
		b = 6;			// Number of FM + DAC tracks

		while (b > 0)
		{
			zTrack& track = *(zTrack*)&mRam[ix];
			if (track.Volume < 0x7f)
			{
				++track.Volume;		// It may look different, but this is in fact a volume *decrease*

				if (track.PlaybackControl & 0x80 && (track.PlaybackControl & 0x40) == 0)
				{
					const uint16 backup_bc = bc;
					zSendTL();
					bc = backup_bc;
				}
			}

			ix += 0x30;
			--b;
		}
	}

	// Locations 0x08df - ?
	void zDoMusicFadeIn()
	{
		mCycles += 420;
		if (zFadeInTimeout == 0)
			return;

		bankswitchToMusic();

		--zFadeDelay;
		if (zFadeDelay != 0)
			return;

		zFadeDelay = zFadeDelayTimeout;
		b = 5;		// Number of FM tracks
		ix = zSongFM1;
		de = 0x30;

		while (b > 0)
		{
			zTrack& track = *(zTrack*)&mRam[ix];
			--track.Volume;

			const uint16 backup_bc = bc;
			zSendTL();
			bc = backup_bc;

			ix += 0x30;
			--b;
		}

		--zFadeInTimeout;
		if (zFadeInTimeout != 0)
			return;

		b = 3;		// Number of PSG tracks
		ix = zSongPSG1;
		de = 0x30;

		while (b > 0)
		{
			zTrack& track = *(zTrack*)&mRam[ix];
			track.PlaybackControl &= ~0x04;
			ix += 0x30;
			--b;
		}

		{
			ix = zSongFM6_DAC;
			zTrack& track = *(zTrack*)&mRam[ix];
			track.PlaybackControl &= ~0x04;
		}
	}

	// Locations 0x0944 - ?
	void zStopAllSound()
	{
		memset(&mRam[zTempVariablesStart], 0, 0x0393);		// Zero RAM from 0x1c0d to 0x1f9f
		zTempoSpeedup = 0;
		ix = zFMDACInitBytes;
		b = 6;		// Number of FM channels
		mCycles += 305385;

		while (b > 0)
		{
			const uint16 backup_bc = bc;

			// The following is a replacement for "zFMSilenceChannel", see disassembly for why
			{
				mCycles += 675;
				zSetMaxRelRate();

				a = 0x40;
				c = 0x7f;
				mCycles += 465;
				zFMOperatorWriteLoop();

				// Set operator
				zTrack& track = *(zTrack*)&mRam[ix];
				mCycles += 1110;
				zWriteFMI(0x28, track.VoiceControl);
				mCycles += 495;
			}

			mCycles += 255;
			zFMClearSSGEGOps();
			ix += 2;

			bc = backup_bc;
			--b;
			mCycles += 645;
		}

		// Here's some extra code in original that an be skipped

		mCycles += 285;
		zPSGSilenceAll();

		// Disable DAC
		mCycles += 780;
		zWriteFMI(0x2b, 0);
		mCycles += 345;

		zFM3NormalMode();
	}

	// Locations 0x0979 - ?
	void zFM3NormalMode()
	{
		// Reset mode
		mCycles += 990;
		zWriteFMI(0x27, 0);
		zNextSound = 0;
		mCycles += 900;
	}

	// Locations 0x0986 - ?
	void zFMClearSSGEGOps()
	{
		a = 0x90;
		c = 0;
		mCycles += 360;
		zFMOperatorWriteLoop();
	}

	// Locations 0x09bc - ?
	void zPSGSilenceAll()
	{
		const uint16 backup_bc = bc;
		a = 0x9f;		// Command to silence PSG1

		mCycles += 630;
		for (int i = 0; i < 4; ++i)		// Loop 4 times: 3 PSG channels + noise channel
		{
			writePSG(a);
			a += 0x20;
			mCycles += 495;
		}

		bc = backup_bc;
		zNextSound = 0;
		mCycles += 630;
	}

	// Locations 0x09cc - ?
	void TempoWait()
	{
		uint16 result = (uint16)zTempoAccumulator + zCurrentTempo;
		zTempoAccumulator = (uint8)result;
		if (result < 0x100)	// Check for overflow
		{
			mCycles += 720;
			return;
		}

		hl = zTracksStart + 0x0b;	// Offset = zTrack.DurationTimeout
		de = 0x30;					// sizeof(zTrack)
		b = 9;						// Number of tracks
		while (b > 0)
		{
			++mRam[hl];
			hl += 0x30;
			--b;
		}
		mCycles += 5835;
	}

	// Locations 0x09e2 - ?
	void zFillSoundQueue()
	{
		hl = 0x1c0a;		// Address of zMusicNumber
		de = zSoundQueue0;
		mRam[de]   = read8(hl);
		mRam[de+1] = read8(hl+1);
		mRam[de+2] = read8(hl+2);
		de += 3;
		a = 0;
		mRam[hl]   = 0;
		mRam[hl+1] = 0;
		mRam[hl+2] = 0;
		mCycles += 1815;
	}

	// Locations 0x09f6 - ?
	void zFMSilenceChannel()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		mCycles += 255;
		zSetMaxRelRate();

		a = 0x40;
		c = 0x7f;
		mCycles += 465;
		zFMOperatorWriteLoop();

		c = track.VoiceControl;
		zKeyOnOff();
	}

	// Locations 0x0a06 - ?
	void zSetMaxRelRate()
	{
		a = 0x80;
		c = 0xff;
		mCycles += 210;
		zFMOperatorWriteLoop();
	}

	// Locations 0x0a0a - ?
	void zFMOperatorWriteLoop()
	{
		mCycles += 105;
		for (int i = 0; i < 4; ++i)
		{
			const uint16 backup_af = af;
			mCycles += 420;
			zWriteFMIorII();

			af = backup_af;
			a += 4;
			mCycles += 450;
		}
		mCycles += 75;
	}

	// Locations 0x0b98 - ?
	void zUpdateDACTrack()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		--track.DurationTimeout;
		if (track.DurationTimeout != 0)
		{
			mCycles += 945;
			return;
		}

		e = track.DataPointerLow;
		d = track.DataPointerHigh;
		mCycles += 1425;
		zUpdateDACTrack_cont();
	}

	// Locations 0x0ba2 - ?
	void zUpdateDACTrack_cont()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		a = read8(de);
		++de;
		if (a >= 0xe0)
		{
			mCycles += 450;
			zHandleDACCoordFlag();
			return;
		}

		mCycles += 450;
		if ((a & 0x80) == 0)
		{
			--de;
			a = track.SavedDAC;
			mCycles += 870;
		}
		else
		{
			// .got_sample:
			track.SavedDAC = a;
			mCycles += 495;
		}

		if (a != 0x80)		// Is it a rest?
		{
			a &= 0x7f;
			const uint16 backup_de = de;
			const uint16 backup_af = af;

			mCycles += 855;
			zKeyOffIfActive();

			mCycles += 255;
			zFM3NormalMode();

			af = backup_af;
			ix = zSongFM6_DAC;
			zTrack& track2 = *(zTrack*)&mRam[ix];
			if ((track2.PlaybackControl & 0x04) == 0)
			{
				mCycles += 1065;
				zDACIndex = a;
			}
			else
			{
				IMPLEMENT_CYCLES("mCycles += ?");	// Include the following "de = backup_de;" as well
			}
			de = backup_de;
		}
		else
		{
			mCycles += 255;
		}

		// zUpdateDACTrack_GetDuration:
		a = read8(de);
		if ((int8)a > 0)
		{
			// zStoreDuration:
			++de;
			mCycles += 660;
			zComputeNoteDuration();

			track.SavedDuration = a;
			mCycles += 285;
		}
		else
		{
			mCycles += 1215;
		}

		zFinishTrackUpdate();
	}

	// Locations 0x0be3 - ?
	void zHandleDACCoordFlag()
	{
		mCycles += 300;
		zHandleCoordFlag();
		++de;
		mCycles += 240;

		zUpdateDACTrack_cont();
	}

	// Locations 0x0bed - ?
	void zHandleFMorPSGCoordFlag()
	{
		mCycles += 150;
		zHandleCoordFlag();

		// Check playback of this track if just stopped (this part is not needed in original code)
		zTrack& track = *(zTrack*)&mRam[ix];
		if ((track.PlaybackControl & 0x80) == 0)
			return;

		++de;
		mCycles += 240;
		zGetNextNote_cont();
	}

	// Location 0x0bf0 - ?
	void zHandleCoordFlag()
	{
		const uint8 coordFlag = a;
		a = read8(de);
		mCycles += 1935;	// Cycles after the jump

		// TODO: What's not implemented, is out-commented here
		switch (coordFlag)
		{
			case 0xe0:  cfPanningAMSFMS();		 break;
			case 0xe1:  cfDetune();				 break;
			case 0xe2:  cfFadeInToPrevious();	 break;
			case 0xe3:  cfSilenceStopTrack();	 break;
			case 0xe4:  cfSetVolume();			 break;
			case 0xe5:  cfChangeVolume2();		 break;
			case 0xe6:  cfChangeVolume();		 break;
			case 0xe7:  cfPreventAttack();		 break;
			case 0xe8:  cfNoteFill();			 break;
			case 0xe9:  cfSpindashRev();		 break;
		//	case 0xea:  cfPlayDACSample();		 break;
		//	case 0xeb:  cfConditionalJump();	 break;
			case 0xec:  cfChangePSGVolume();	 break;
			case 0xed:  cfSetKey();				 break;
		//	case 0xee:  cfSendFMI();			 break;
			case 0xef:  cfSetVoice();			 break;
			case 0xf0:  cfModulation();			 break;
		//	case 0xf1:  cfAlterModulation();	 break;
			case 0xf2:  cfStopTrack();			 break;
			case 0xf3:  cfSetPSGNoise();		 break;
			case 0xf4:  cfSetModulation();		 break;
			case 0xf5:  cfSetPSGVolEnv();		 break;
			case 0xf6:  cfJumpTo();				 break;
			case 0xf7:  cfRepeatAtPos();		 break;
			case 0xf8:  cfJumpToGosub();		 break;
			case 0xf9:  cfJumpReturn();			 break;
			case 0xfa:  cfDisableModulation();	 break;
			case 0xfb:  cfChangeTransposition(); break;
			case 0xfc:  cfLoopContinuousSFX();	 break;
		//	case 0xfd:  cfToggleAltFreqMode();	 break;
		//	case 0xfe:  cfFM3SpecialMode();		 break;
			case 0xff:  cfMetaCF();				 break;

			default:
				RMX_ERROR("Not implemented: coordFlag = " << rmx::hexString(coordFlag, 2), );
		}
	}

	// Locations 0x0c51 - ?
	void cfPanningAMSFMS()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		a = track.AMSFMSPan & 0x3f;
		const uint16 backup_de = de;
		std::swap(de, hl);
		a |= read8(hl);
		track.AMSFMSPan = a;

		c = a;
		a = 0xb4;
		mCycles += 1485;
		zWriteFMIorII();

		de = backup_de;
		mCycles += 300;
	}

	// Locations 0x0c65 - ?
	void cfSpindashRev()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.Transpose += zSpindashRev;
		if (track.Transpose != 0x10)
		{
			++zSpindashRev;
			mCycles += 1485;
		}
		else
		{
			IMPLEMENT_CYCLES("mCycles += ?");
		}
		--de;
	}

	// Locations 0x0c77 - ?
	void cfDetune()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.Detune = a;
		mCycles += 435;
	}

	// Locations 0x0c7b - ?
	void cfFadeInToPrevious()
	{
		// No fading over please, as that is already handled outside of this sound driver
		//  -> Instead just stop here
	#if 0
		zFadeToPrevFlag = a;
	#else
		// There's two situations that need different handling:
		//  -> Last frame of sound effect 0x2a (extra life)	-> Stop immediately, otherwise a drum sound will be heard at the end that does not belong there
		//  -> First frame of sound effects 0x2b and 0x31	-> Do not stop, as these sound would not play at all
		mStopped = (mFrameNumber > 0);
	#endif
		mCycles += 345;
	}

	// Locations 0x0c7f - ?
	void cfSilenceStopTrack()
	{
		IMPLEMENT_CYCLES("mCycles += ?");
		zTrack& track = *(zTrack*)&mRam[ix];
		if ((track.VoiceControl & 0x80) == 0)
		{
			zFMSilenceChannel();
		}
		cfStopTrack();
	}

	// Locations 0x0c85 - ?
	void cfSetVolume()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.VoiceControl & 0x80)
		{
			a = 0x0f - ((a >> 3) & 0x0f);
			track.Volume = a;
			mCycles += 1560;
		}
		else
		{
			a = 0x7f - (a & 0x7f);
			track.Volume = a;
			mCycles += 1155;
			zSendTL();
		}
	}

	// Locations 0x0ca1 - 0x0ca2
	void cfChangeVolume2()
	{
		++de;
		a = read8(de);
		mCycles += 195;
		cfChangeVolume();
	}

	// Locations 0x0ca3 - ?
	void cfChangeVolume()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.VoiceControl & 0x80)
		{
			mCycles += 465;
			return;
		}

		mCycles += 375;
		const int8 old_a = a;
		a += track.Volume;
		if ((int8)a < 0)
		{
			a = (old_a < 0) ? 0 : 0x7f;
			mCycles += 1080;
		}
		else
		{
			mCycles += 720;
		}
		track.Volume = a;
		zSendTL();
	}

	// Locations 0x0cba - ?
	void zSendTL()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		const uint16 backup_de = de;
		de = zFMInstrumentTLTable;

		l = track.TLPtrLow;
		h = track.TLPtrHigh;
		b = 4;		// Number of instruments
		mCycles += 990;

		while (b > 0)
		{
			a = read8(hl);
			mCycles += 165;
			if (a & 0x80)
			{
				a += track.Volume;
				mCycles += 285;
			}
			mCycles += 150;

			c = a & 0x7f;
			a = read8(de);
			mCycles += 525;
			zWriteFMIorII();

			++de;
			++hl;
			--b;
			mCycles += 375;
		}

		de = backup_de;
		mCycles += 225;
	}

	// Locations 0x0cdb - ?
	void cfPreventAttack()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.PlaybackControl |= 0x02;
		--de;
		mCycles += 585;
	}

	// Locations 0x0ce1 - ?
	void cfNoteFill()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		mCycles += 255;
		zComputeNoteDuration();

		track.NoteFillTimeout = a;
		track.NoteFillMaster = a;
		mCycles += 720;
	}

	// Locations 0x0d01 - ?
	void cfChangePSGVolume()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		if ((track.VoiceControl & 0x80) == 0)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		track.PlaybackControl &= ~0x10;
		--track.VolEnv;
		track.Volume = std::min(track.Volume + (int8)a, 0x0f);	// Limit to 0x0f = silence
		mCycles += 2040;
	}

	// Locations 0x0d1b - ?
	void cfSetKey()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		a -= 0x40;
		track.Transpose = a;
		mCycles += 540;
	}

	// Locations 0x0d2e - ?
	void cfSetVoice()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		if (track.VoiceControl & 0x80)
		{
			// zSetVoicePSG():
			if (a & 0x80)
			{
				++de;
			}
			track.VoiceIndex = a;
			return;
		}

		mCycles += 660;
		zSetMaxRelRate();

		uint16 backup_de;
		a = read8(de);
		track.VoiceIndex = a;
		mCycles += 450;

		if ((int8)a < 0)
		{
			++de;
			track.VoiceSongID = read8(de);

			// zSetVoiceUploadAlter:
			backup_de = de;
			a = track.VoiceSongID - 0x81;
			c = zID_MusicPointers;
			GetPointerTable();
			PointerTableOffset();
			ReadPointer();

			b = a = track.VoiceIndex & 0x7f;
			zGetFMInstrumentOffset();
		}
		else
		{
			// zSetVoiceUpload:
			backup_de = de;
			b = a;
			mCycles += 630;
			zGetFMInstrumentPointer();
		}

		// zSetVoiceDoUpload:
		mCycles += 255;
		zSendFMInstrument();

		de = backup_de;
		mCycles += 300;
	}

	// Locations 0x0d6d - ?
	void cfModulation()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		track.ModulationPtrLow = e;
		track.ModulationPtrHigh = d;
		track.ModulationCtrl = 0x80;
		de += 3;
		mCycles += 1275;
	}

	// Locations 0x0d87 - ?
	void cfStopTrack()
	{
		zTrack& track0 = *(zTrack*)&mRam[ix];

		track0.PlaybackControl &= ~0x80;
	#ifndef SOUNDDRIVER_FIX_BUGS
		mRam[0x1c15] = 0x1f;
	#endif
		mCycles += 900;
		zKeyOffIfActive();

		c = track0.VoiceControl;
		const uint16 backup_ix = ix;
		mCycles += 765;
		zGetSFXChannelPointers();

		a = zUpdatingSFX;
		if (a != 0)
		{
			mCycles += 405;
			ix = hl;
			zTrack& track = *(zTrack*)&mRam[ix];	// After ix has changed
			hl = zVoiceTblPtr;

			track.PlaybackControl &= ~0x04;
			if (track.VoiceControl & 0x80)
			{
				// zStopPSGTrack:
				IMPLEMENT_CYCLES("mCycles += ?");
				if (track.PlaybackControl & 0x01)
				{
					a = track.PSGNoise;
					if (a & 0x80)
					{
						writePSG(a);
					}
				}
			}
			else if (track.PlaybackControl & 0x80)
			{
				// TODO: Out-commented for now due to too many missing functions yet
				RMX_ERROR("Not implemented", );
/*
				if (track.VoiceControl == 2)	// FM3 track?
				{
					a = 0x4f;
					if ((track.PlaybackControl & 0x01) == 0)
						a &= 0x0f;

					zWriteFM3Settings();
				}

				a = track.VoiceIndex;
				if ((int8)a < 0)
				{
					zSetVoiceUploadAlter();
					zSendSSGEGData();
				}
				else
				{
					// .switch_to_music:
					b = a;

					const uint16 backup_hl2 = hl;
					bankswitchToMusic();
					hl = backup_hl2;

					zGetFMInstrumentOffset();
					zSendFMInstrument();

					a = 0x1f;
					bankswitch();

					a = track.HaveSSGEGFlag;
					if ((int8)a < 0)
					{
						e = track.SSGEGPointerLow;
						d = track.SSGEGPointerHigh;
						zSendSSGEGData();
					}
				}
*/
			}
			else
			{
				mCycles += 2760;
			}
		}

		// zStopCleanExit:
		ix = backup_ix;

		// Original code pops two return values from stack
		//  -> That brings us back to zTrackUpdLoop, for the next loop
		//  -> We implement the same behavior by doing checks like ((track.PlaybackControl & 0x80) == 0) where needed
	}

	// Locations 0x0e39 - ?
	void cfSetPSGNoise()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		// Using the original code here to get the cycles right
	#if 1
		// Original code
		if (track.VoiceControl & 0x04)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		mCycles += 480;
		writePSG(0xdf);		// Silence PSG3

		a = read8(de);		// Get PSG noise value
		track.PSGNoise = a;

		if (a == 0)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			track.PlaybackControl &= ~0x01;
			a = 0xff;		// Silence the noise channel
		}
		else
		{
			track.PlaybackControl |= 0x01;
			mCycles += 1170;
		}
		writePSG(a);
		mCycles += 345;

	#else
		// Fixed code, see disassembly
		if ((track.VoiceControl & 0x80) == 0)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		track.PSGNoise = a;
		if (a != 0)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			track.PlaybackControl &= ~0x01;
			a = 0xff;
		}
		else
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			track.PlaybackControl |= 0x01;
			a = 0xdf;	// Silence PSG3
		}

		if (track.PlaybackControl & 0x04)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		IMPLEMENT_CYCLES("mCycles += ?");
		writePSG(a);
		writePSG(read8(de));
	#endif
	}

	// Locations 0x0e58 - ?
	void cfSetModulation()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.ModulationCtrl = a;
		mCycles += 435;
	}

	// Locations 0x0e58 - ?
	void cfSetPSGVolEnv()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		if (track.VoiceControl & 0x80)
		{
			track.VoiceIndex = a;
			mCycles += 810;
		}
		else
		{
			mCycles += 465;
		}
	}

	// Locations 0x0e61 - ?
	void cfJumpTo()
	{
		std::swap(de, hl);
		e = read8(hl);
		++hl;
		d = read8(hl);
		--de;
		mCycles += 600;
	}

	// Locations 0x0e67 - ?
	void cfRepeatAtPos()
	{
		++de;
		hl = ix + 0x28 + a;		// Offset of zTrack.LoopCounters
		a = read8(hl);
		if (a == 0)
		{
			mRam[hl] = read8(de);
			mCycles += 135;
		}
		mCycles += 1245;

		++de;
		--mRam[hl];
		if (mRam[hl] != 0)
		{
			mCycles += 405;
			cfJumpTo();
		}
		else
		{
			++de;
			mCycles += 645;
		}
	}

	// Locations 0x0e7e - ?
	void cfJumpToGosub()
	{
		c = a;
		++de;
		a = read8(de);
		b = a;

		const uint16 backup_bc = bc;

		zTrack& track = *(zTrack*)&mRam[ix];
		--track.StackPointer;
		bc = track.StackPointer;
		--track.StackPointer;

		hl = ix + c;
		mRam[hl] = d;
		--hl;
		mRam[hl] = e;

		de = backup_bc - 1;
		mCycles += 2790;
	}

	// Locations 0x0e98 - ?
	void cfJumpReturn()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		hl = ix + track.StackPointer;
		e = read8(hl);
		++hl;
		d = read8(hl);
		track.StackPointer += 2;
		mCycles += 2070;
	}

	// Locations 0x0eab - ?
	void cfDisableModulation()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.ModulationCtrl &= ~0x80;
		--de;
		mCycles += 585;
	}

	// Locations 0x0eb1 - ?
	void cfChangeTransposition()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.Transpose += a;
		mCycles += 720;
	}

	// Locations 0x0eb8 - ?
	void cfLoopContinuousSFX()
	{
		a = zContinuousSFXFlag;
		if (zContinuousSFXFlag != 0x80)
		{
			zContinuousSFX = 0;
			++de;
			mCycles += 1140;
			return;
		}

		// .run_counter:
		IMPLEMENT_CYCLES("mCycles += ?");
		hl = 0x1c31;	// Address of zContSFXLoopCnt; probably not needed here any more
		--zContSFXLoopCnt;
		if (zContSFXLoopCnt == 0)
		{
			zContinuousSFXFlag = 0;
		}
		cfJumpTo();
	}

	// Locations 0x0f2f - ?
	void cfMetaCF()
	{
		const uint8 extraCoordFlag = a;
		++de;
		a = read8(de);
		mCycles += 1695;	// Cycles after the jump

		// TODO: What's not implemented, is out-commented here
		switch (extraCoordFlag)
		{
			case 0x00:  cfSetTempo();		  break;
		//	case 0x01:  cfPlaySoundByIndex(); break;
		//	case 0x02:  cfHaltSound();		  break;
		//	case 0x03:  cfCopyData();		  break;
		//	case 0x04:  cfSetTempoDivider();  break;
		//	case 0x05:  cfSetSSGEG();		  break;
		//	case 0x06:  cfFMVolEnv();		  break;
			case 0x07:  cfResetSpindashRev(); break;

			default:
				RMX_ERROR("Not implemented: extraCoordFlag = " << rmx::hexString(extraCoordFlag, 2), );
		}
	}

	// Locations 0x0f36 - ?
	void cfSetTempo()
	{
		zCurrentTempo = a;
		mCycles += 405;
	}

	// Locations 0x0fbe - ?
	void cfResetSpindashRev()
	{
		zSpindashRev = 0;
		--de;
		mCycles += 555;
	}

	// Locations 0x0fc4 - 0x1036
	void zUpdatePSGTrack()
	{
		zTrack& track = *(zTrack*)&mRam[ix];

		--track.DurationTimeout;
		mCycles += 780;
		if (track.DurationTimeout == 0)
		{
			mCycles += 360;
			zGetNextNote();

			// Check playback of this track if just stopped (this part is not needed in original code)
			if ((track.PlaybackControl & 0x80) == 0)
				return;

			if (track.PlaybackControl & 0x10)	// Track is resting?
			{
				mCycles += 465;
				return;
			}

			mCycles += 630;
			zPrepareModulation();
			mCycles += 180;
		}
		else
		{
			// .note_going:
			mCycles += 525;
			if (track.NoteFillTimeout != 0)
			{
				--track.NoteFillTimeout;
				mCycles += 600;
				if (track.NoteFillTimeout == 0)
				{
					zRestTrack();
					return;
				}
			}
			else
			{
				mCycles += 180;
			}
		}

		mCycles += 255;
		zUpdateFreq();

		mCycles += 255;
		if (zDoModulation())
			return;

		if (track.PlaybackControl & 0x04)	// Check if SFX overriding this track
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		mCycles += 375;
		a = (l & 0x0f) | track.VoiceControl;
		mCycles += 510;
		writePSG(a);

		a = (l & 0xf0) | h;
		a = (a >> 4) | (a << 4);
		mCycles += 660;
		writePSG(a);

		a = track.VoiceIndex;
		c = 0;
		mCycles += 645;
		if (a != 0)
		{
			--a;

			// Get pointer to volume envelope for track in hl
			c = zID_VolEnvPointers;
			GetPointerTable();
			PointerTableOffset();

			mCycles += 3180;
			if (zDoVolEnv())
				return;

			c = a;
			mCycles += 60;
		}
		else
		{
			mCycles += 180;
		}

		if (track.PlaybackControl & 0x10)
		{
			mCycles += 465;
			return;
		}

		mCycles += 375;
		a = track.Volume + c;
		if (a & 0x10)
		{
			a = 0x0f;	// Set silence on PSG track
			mCycles += 675;
		}
		else
		{
			mCycles += 645;
		}

		a |= track.VoiceControl;
		a += 0x10;
		if (track.PlaybackControl & 1)
		{
			a += 0x20;
			mCycles += 975;
		}
		else
		{
			mCycles += 795;
		}
		writePSG(a);
		mCycles += 345;
	}

	// Locations 0x1037 - 0x1037
	bool zDoVolEnvSetValue()
	{
		IMPLEMENT_CYCLES("mCycles += ?");
		zTrack& track = *(zTrack*)&mRam[ix];
		track.VolEnv = a;
		return zDoVolEnv();
	}

	// Locations 0x103a - 0x106b
	bool zDoVolEnv()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		const uint16 backup_hl = hl;

		hl += track.VolEnv;
		bc = hl;
		a = read8(hl);
		hl = backup_hl;

		mCycles += 975;
		if (a & 0x80)	// Is it a terminator?
		{
			mCycles += 225;
			if (a == 0x83)
			{
				mCycles += 930;
				zRestTrack();
				return true;	// Skip rest of caller
			}
			else if (a == 0x81)
			{
				track.PlaybackControl |= 0x10;
				mCycles += 1140;
				return true;	// Skip rest of caller
			}
			else if (a == 0x80)
			{
				IMPLEMENT_CYCLES("mCycles += ?");
				a = 0;
				return zDoVolEnvSetValue();
			}
			else
			{
				IMPLEMENT_CYCLES("mCycles += ?");
				++bc;
				a = read8(bc);
				return zDoVolEnvSetValue();
			}
		}
		else
		{
			mCycles += 300;
		}

		++track.VolEnv;
		mCycles += 495;
		return false;
	}

	// Locations 0x106c - 0x1074
	void zRestTrack()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		track.PlaybackControl |= 0x10;
		mCycles += 645;
		if (track.PlaybackControl & 0x04)
		{
			IMPLEMENT_CYCLES("mCycles += ?");
			return;
		}

		mCycles += 75;
		zSilencePSGChannel();
	}

	// Locations 0x1075 - 0x1089
	void zSilencePSGChannel()
	{
		zTrack& track = *(zTrack*)&mRam[ix];
		a = 0x1f + track.VoiceControl;
		mCycles += 450;
		if ((a & 0x80) == 0)	// Check if it's a PSG channel at all
		{
			mCycles += 165;
			return;
		}

		mCycles += 75;
		writePSG(a);

		if (a == 0xdf)	// Was this PSG3?
		{
			a = 0xff;
			mCycles += 675;
			writePSG(0xff);		// Silence the noise channel
			mCycles += 345;
		}
		else
		{
			mCycles += 660;
		}
	}

	// Locations 0x108a - 0x1113 (with large structural changes!)
	void zPlayDigitalAudio()
	{
		while (!mStopped)	// That should stay true all the time, or it's already false when entering this function
		{
			switch (mDACPlaybackState)
			{
				case DACPlaybackState::IDLE:
				{
					// .dac_idle_loop:

					// Missing here: SEGA PCM check

					if (zDACIndex == 0)
					{
						// Do nothing and stay in this state
						return;
					}

					// Enable DAC
					mCycles += 1200;
					writeFMI(0x2b, 0x80, 0x10a5);

					iy = 0x1116;			// Start address of "DecTable"
					a = zDACIndex - 1;
					zDACIndex |= 0x80;		// Mark as playing

					hl = 0x8000;
					PointerTableOffset();

					c = 0x80;
					a = read8(hl);
					sample1_rate = a;
					sample2_rate = a;
				#ifdef VERIFY_AGAINST_DUMPS
					mRam[0x10cb] = a;	// Actually only needed for verification (thanks to self-modifying code in the original)
					mRam[0x10e8] = a;	// Actually only needed for verification (thanks to self-modifying code in the original)
				#endif

					mDACSampleLength = read16(hl+1);	// DAC sample's length
					mDACSampleDataPtr = read16(hl+3);	// Pointer to DAC data
					mCycles += 4560;

					mDACPlaybackState = DACPlaybackState::PLAYBACK;
					break;
				}

				case DACPlaybackState::PLAYBACK:
				{
					if ((zDACIndex & 0x80) == 0)	// Playing flag was cleared
					{
						mDACPlaybackState = DACPlaybackState::IDLE;
						break;
					}

					const uint32 maxCycleCount = mNumFramesCalculated * CYCLES_PER_FRAME;
					while (mCycles < maxCycleCount)
					{
						de = mDACSampleLength;
						hl = mDACSampleDataPtr;

						// Reached the end?
						if (de == 0)
						{
							zDACIndex = 0;

							// zPlayDigitalAudio:

							// Disable DAC
							writeFMI(0x2b, 0, 0x108f);

							mDACPlaybackState = DACPlaybackState::IDLE;
							return;
						}

						// .dac_playback_loop:

						// Sample 1
						mCycles += 90 + sample1_rate * 195;

						a = read8(hl) >> 4;
						sample1_index = a;
					#ifdef VERIFY_AGAINST_DUMPS
						mRam[0x10e2] = a;	// Actually only needed for verification (thanks to self-modifying code in the original)
					#endif
						a = c;
						a += read8(iy + sample1_index);

						// Set DAC
						writeFMI(0x2a, a, 0x10e3);
						c = a;
						mCycles += 1605;

						// Sample 2
						mCycles += 90 + sample1_rate * 195;

						a = read8(hl) & 0x0f;
						sample2_index = a;
					#ifdef VERIFY_AGAINST_DUMPS
						mRam[0x10fb] = a;	// Actually only needed for verification (thanks to self-modifying code in the original)
					#endif
						a = c;
						a += read8(iy + sample2_index);

						writeFMI(0x2a, a, 0x10fc);
						c = a;
						mCycles += 1425;

						++mDACSampleDataPtr;
						--mDACSampleLength;
						mCycles += 855;
					}
					return;
				}
			}

			// Loop once more in case of a state transition
		}
	}

private:
	#pragma pack(1)
	struct zTrack
	{
		// Playback control bits:
		// 	0 (01h)		Noise channel (PSG) or FM3 special mode (FM)
		// 	1 (02h)		Do not attack next note
		// 	2 (04h)		SFX is overriding this track
		// 	3 (08h)		'Alternate frequency mode' flag
		// 	4 (10h)		'Track is resting' flag
		// 	5 (20h)		'Pitch slide' flag
		// 	6 (40h)		'Sustain frequency' flag -- prevents frequency from changing again for the lifetime of the track
		// 	7 (80h)		Track is playing
		uint8 PlaybackControl;		// S&K: 0
		// Voice control bits:
		// 	0-1    		FM channel assignment bits (00 = FM1 or FM4, 01 = FM2 or FM5, 10 = FM3 or FM6/DAC, 11 = invalid)
		// 	2 (04h)		For FM/DAC channels, selects if reg/data writes are bound for part II (set) or part I (unset)
		// 	3 (08h)		Unknown/unused
		// 	4 (10h)		Unknown/unused
		// 	5-6    		PSG Channel assignment bits (00 = PSG1, 01 = PSG2, 10 = PSG3, 11 = Noise)
		// 	7 (80h)		PSG track if set, FM or DAC track otherwise
		uint8 VoiceControl;			// S&K: 1
		uint8 TempoDivider;			// S&K: 2
		uint8 DataPointerLow;		// S&K: 3
		uint8 DataPointerHigh;		// S&K: 4
		uint8 Transpose;			// S&K: 5
		uint8 Volume;				// S&K: 6
		uint8 ModulationCtrl;		// S&K: 7		; Modulation is on if nonzero. If only bit 7 is set, then it is normal modulation; otherwise, this-1 is index on modulation envelope pointer table
		uint8 VoiceIndex;			// S&K: 8		; FM instrument/PSG voice
		uint8 StackPointer;			// S&K: 9		; For call subroutine coordination flag
		uint8 AMSFMSPan;			// S&K: 0Ah
		uint8 DurationTimeout;		// S&K: 0Bh
		uint8 SavedDuration;		// S&K: 0Ch		; Already multiplied by timing divisor
		// ---------------------------------
		union
		{
			uint8 SavedDAC;			// S&K: 0Dh		; For DAC channel
			uint8 FreqLow;			// S&K: 0Dh		; For FM/PSG channels
		};
		// ---------------------------------
		uint8 FreqHigh;				// S&K: 0Eh		; For FM/PSG channels
		uint8 VoiceSongID;			// S&K: 0Fh		; For using voices from a different song
		uint8 Detune;				// S&K: 10h		; In S&K, some places used 11h instead of 10h
		uint8 Unk11h;				// S&K: 11h
		uint8 unused[5];			// S&K: 12h-16h	; Unused
		uint8 VolEnv;				// S&K: 17h		; Used for dynamic volume adjustments
		// ---------------------------------
		union
		{
			uint8 FMVolEnv;			// S&K: 18h
			uint8 HaveSSGEGFlag;	// S&K: 18h		; For FM channels, if track has SSG-EG data
		};
		union
		{
			uint8 FMVolEnvMask;		// S&K: 19h
			uint8 SSGEGPointerLow;	// S&K: 19h		; For FM channels, custom SSG-EG data pointer
		};
		union
		{
			uint8 PSGNoise;			// S&K: 1Ah
			uint8 SSGEGPointerHigh;	// S&K: 1Ah		; For FM channels, custom SSG-EG data pointer
		};
		// ---------------------------------
		uint8 FeedbackAlgo;			// S&K: 1Bh
		uint8 TLPtrLow;				// S&K: 1Ch
		uint8 TLPtrHigh;			// S&K: 1Dh
		uint8 NoteFillTimeout;		// S&K: 1Eh
		uint8 NoteFillMaster;		// S&K: 1Fh
		uint8 ModulationPtrLow;		// S&K: 20h
		uint8 ModulationPtrHigh;	// S&K: 21h
		// ---------------------------------
		union
		{
			uint8 ModulationValLow;	// S&K: 22h
			uint8 ModEnvSens;		// S&K: 22h
		};
		// ---------------------------------
		uint8 ModulationValHigh;	// S&K: 23h
		uint8 ModulationWait;		// S&K: 24h
		// ---------------------------------
		union
		{
			uint8 ModulationSpeed;	// S&K: 25h
			uint8 ModEnvIndex;		// S&K: 25h
		};
		// ---------------------------------
		uint8 ModulationDelta;		// S&K: 26h
		uint8 ModulationSteps;		// S&K: 27h
		uint16 LoopCounters;		// S&K: 28h		; Might overflow into the following data
		uint8 VoicesLow;			// S&K: 2Ah		; Low byte of pointer to track's voices, used only if zUpdatingSFX is set
		uint8 VoicesHigh;			// S&K: 2Bh		; High byte of pointer to track's voices, used only if zUpdatingSFX is set
		uint32 Stack_top;			// S&K: 2Ch-2Fh	; Track stack; can be used by LoopCounters
	};

private:
	uint8 mRegisters[0x20] = { 0 };
	uint8& f = mRegisters[0];
	uint8& a = mRegisters[1];
	uint16& af = *(uint16*)&mRegisters[0];
	uint8& c = mRegisters[2];
	uint8& b = mRegisters[3];
	uint16& bc = *(uint16*)&mRegisters[2];
	uint8& e = mRegisters[4];
	uint8& d = mRegisters[5];
	uint16& de = *(uint16*)&mRegisters[4];
	uint8& l = mRegisters[6];
	uint8& h = mRegisters[7];
	uint16& hl = *(uint16*)&mRegisters[6];
	uint16& ix = *(uint16*)&mRegisters[8];
	uint16& iy = *(uint16*)&mRegisters[10];

	const uint8* mFixedContentData = nullptr;
	uint32 mFixedContentSize = 0;
	uint32 mFixedContentOffset = 0;
	uint32 zBankBaseAddress = 0;
	uint32 mEnforcedSourceAddress = 0;
	uint32 mCycles = 0;
	uint32 mFrameNumber = 0;
	uint32 mNumFramesCalculated = 0;
	bool mStopped = false;
	std::vector<SoundChipWrite> mSoundChipWritesThisFrame;		// Only sound chip writes for the current frame
	std::vector<SoundChipWrite> mSoundChipWritesCalculated;		// All sound chip writes created in last "performSoundDriverUpdate"; this includes the current and potentially future frames

	uint8 mRam[0x2000] = { 0 };
	uint8& zSoundQueueEntry0  = mRam[0x1c05];
	uint8& zSoundQueueEntry1  = mRam[0x1c06];
	uint8& zSoundQueueEntry2  = mRam[0x1c07];
	uint8& zTempoSpeedup	  = mRam[0x1c08];		// Higher values mean less speedup (!) -- it's usually 0, Speed Shoes and DDZ set this to 8, Blue Spheres to one of 0x20, 0x18, 0x10, 0x08
	uint8& zNextSound		  = mRam[0x1c09];
	uint8& zMusicNumber		  = mRam[0x1c0a];
	uint8& zSFXNumber0		  = mRam[0x1c0b];
	uint8& zSFXNumber1		  = mRam[0x1c0c];
	uint8& zFadeOutTimeout	  = mRam[0x1c0d];
	uint8& zFadeDelay		  = mRam[0x1c0e];
	uint8& zFadeDelayTimeout  = mRam[0x1c0f];
	uint8& zPauseFlag		  = mRam[0x1c10];
	uint8& zHaltFlag		  = mRam[0x1c11];
	uint8& zTempoAccumulator  = mRam[0x1c13];
	uint8& zUpdatingSFX		  = mRam[0x1c19];
	uint8& zCurrentTempo	  = mRam[0x1c24];
	uint8& zContinuousSFX	  = mRam[0x1c25];
	uint8& zContinuousSFXFlag = mRam[0x1c26];
	uint8& zSpindashRev		  = mRam[0x1c27];
	uint8& zRingSpeaker		  = mRam[0x1c28];
	uint8& zFadeInTimeout	  = mRam[0x1c29];
	uint16& zVoiceTblPtrSave  = *(uint16*)&mRam[0x1c2a];
	uint8& zCurrentTempoSave  = mRam[0x1c2c];
	uint8& zSongBankSave	  = mRam[0x1c2d];
	uint8& zTempoSpeedupSave  = mRam[0x1c2e];
	uint8& zSpeedupTimeout	  = mRam[0x1c2f];
	uint8& zDACIndex		  = mRam[0x1c30];
	uint8& zContSFXLoopCnt	  = mRam[0x1c31];
	uint8& zSFXSaveIndex	  = mRam[0x1c32];
	uint16& zSongPosition	  = *(uint16*)&mRam[0x1c33];
	uint16& zTrackInitPos	  = *(uint16*)&mRam[0x1c35];
	uint16& zVoiceTblPtr	  = *(uint16*)&mRam[0x1c37];
	uint16& zSFXVoiceTblPtr	  = *(uint16*)&mRam[0x1c39];
	uint8& zSFXTempoDivider	  = mRam[0x1c3b];
	uint8& zSongBank		  = mRam[0x1c3e];	//mRam[0x1c3f];			// Bits 15 to 22 of M68K bank address
	uint8 zFadeToPrevFlag = 0;

	enum class DACPlaybackState
	{
		IDLE,
		PLAYBACK
	};
	DACPlaybackState mDACPlaybackState = DACPlaybackState::IDLE;
	uint16 mDACSampleLength = 0;
	uint16 mDACSampleDataPtr = 0;

	uint8 sample1_rate = 0;
	uint8 sample2_rate = 0;
	uint8 sample1_index = 0;
	uint8 sample2_index = 0;
};



SoundDriver::SoundDriver() :
	mInternal(*new Internal())
{
}

SoundDriver::~SoundDriver()
{
	delete &mInternal;
}

void SoundDriver::setFixedContent(const uint8* data, uint32 size, uint32 offset)
{
	mInternal.setFixedContent(data, size, offset);
}

void SoundDriver::setSourceAddress(uint32 sourceAddress)
{
	mInternal.setSourceAddress(sourceAddress);
}

void SoundDriver::reset()
{
	mInternal.reset();
}

void SoundDriver::playSound(uint8 sfxId)
{
#if defined(VERIFY_AGAINST_DUMPS) || defined(SOUNDDRIVER_DEBUG_CYCLES)
	mInternal.setMusic(sfxId);
#else
	if (sfxId <= 0x32 || sfxId == 0xdc)
	{
		mInternal.setMusic(sfxId);
	}
	else
	{
		mInternal.setSfx(sfxId);
	}
#endif
}

void SoundDriver::setTempoSpeedup(uint8 tempoSpeedup)
{
	mInternal.setTempoSpeedup(tempoSpeedup);
}

SoundDriver::UpdateResult SoundDriver::update()
{
	return mInternal.performSoundDriverUpdate();
}

const std::vector<SoundChipWrite>& SoundDriver::getSoundChipWrites() const
{
	return mInternal.getSoundChipWrites();
}
