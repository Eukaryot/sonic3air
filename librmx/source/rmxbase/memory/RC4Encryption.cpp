/*
*	rmx Library
*	Copyright (C) 2008-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxbase.h"

#define SWAP(a, b) ((a) ^= (b), (b) ^= (a), (a) ^= (b))


void RC4Encryption::encrypt(const void* source, void* dest, int length, const void* key_, int keylen)
{
	const uint8* src = (const uint8*)source;
	uint8* dst = (uint8*)dest;
	const uint8* key = (const uint8*)key_;

	uint8 sbox[256];
	for (int m = 0; m < 256; ++m)
		sbox[m] = m;

	int k = 0;
	for (int m = 0; m < 256; ++m)
	{
		k = (k + sbox[m] + key[m % keylen]) & 0xff;
		SWAP(sbox[m], sbox[k]);
	}

	int i = 0;
	int j = 0;
	for (int m = 0; m < length; ++m)
	{
		i = (i + 1) & 0xff;
		j = (j + sbox[i]) & 0xff;
		SWAP(sbox[i], sbox[j]);

		int k = sbox[(sbox[i] + sbox[j]) & 0xff];
		dst[m] = src[m] ^ k;
	}
}

void RC4Encryption::decrypt(const void* source, void* dest, int length, const void* key, int keylen)
{
	encrypt(source, dest, length, key, keylen);
}
