/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


enum class SamplingMode
{
	POINT,
	BILINEAR
};

enum class TextureWrapMode
{
	CLAMP,
	REPEAT
};

enum class BlendMode
{
	OPAQUE,
	ALPHA,
	ONE_BIT,
	ADDITIVE,
	SUBTRACTIVE,
	MULTIPLICATIVE,
	MINIMUM,
	MAXIMUM
};
