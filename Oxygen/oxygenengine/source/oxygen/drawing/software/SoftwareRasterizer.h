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
	struct Vertex
	{
		Vec2f mPosition;
		Color mColor = Color::WHITE;
		Vec2f mUV;
	};

public:
	SoftwareRasterizer(const BitmapViewMutable<uint32>& output, const Blitter::Options& options) : mOutput(output), mOptions(options) {}

	void setOutput(const BitmapViewMutable<uint32>& output);

	void drawTriangle(const Vertex* vertices, const BitmapView<uint32>& texture, bool useVertexColors);
	void drawTriangle(const Vertex* vertices);

private:
	void drawTrapezoid(const Vertex& vertex00, const Vertex& vertex10, const Vertex& vertex01, const Vertex& vertex11, const BitmapView<uint32>& texture, bool useVertexColors);
	void drawTrapezoid(const Vertex& vertex00, const Vertex& vertex10, const Vertex& vertex01, const Vertex& vertex11);

private:
	BitmapViewMutable<uint32> mOutput;
	Blitter::Options mOptions;
};
