/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace Deflate
{
	FUNCTION_EXPORT uint8* decode(int& outputSize, const void* input, int length);
	FUNCTION_EXPORT uint8* encode(int& outputSize, const void* input, int length);
}
