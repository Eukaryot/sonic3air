/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2025 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "oxygen/drawing/software/Blitter.h"


class SoftwareRasterizer
{
public:
	struct Vertex_P2_T2
	{
		Vec2f mPosition;
		Vec2f mUV;
	};
	struct Vertex_P2_C4
	{
		Vec2f mPosition;
		Color mColor;
	};

public:
	SoftwareRasterizer(BitmapViewMutable<uint32>& output, const Blitter::Options& options) : mOutput(output), mOptions(options) {}

	void drawTriangle(const Vertex_P2_T2* vertices, const Bitmap& texture);
	void drawTriangle(const Vertex_P2_C4* vertices);

private:
	void drawTrapezoid(const Vertex_P2_T2& vertex00, const Vertex_P2_T2& vertex10, const Vertex_P2_T2& vertex01, const Vertex_P2_T2& vertex11, const Bitmap& texture);
	void drawTrapezoid(const Vertex_P2_C4& vertex00, const Vertex_P2_C4& vertex10, const Vertex_P2_C4& vertex01, const Vertex_P2_C4& vertex11);

private:
	BitmapViewMutable<uint32>& mOutput;
	Blitter::Options mOptions;
};
