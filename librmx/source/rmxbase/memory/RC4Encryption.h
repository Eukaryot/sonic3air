/*
*	rmx Library
*	Copyright (C) 2008-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace RC4Encryption
{
	FUNCTION_EXPORT void encrypt(const void* source, void* dest, int length, const void* key, int keylen);
	FUNCTION_EXPORT void decrypt(const void* source, void* dest, int length, const void* key, int keylen);
}
