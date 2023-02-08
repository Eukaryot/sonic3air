/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2023 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/software/SoftwareRasterizer.h"


namespace
{
	template<typename VERTEX>
	void sortThreeVerticesByY(const VERTEX* inVertices, int* outSortedIndices)
	{
		if (inVertices[0].mPosition.y > inVertices[1].mPosition.y)
		{
			outSortedIndices[2] = 0;
			outSortedIndices[0] = 1;
		}
		else
		{
			outSortedIndices[0] = 0;
			outSortedIndices[2] = 1;
		}

		if (inVertices[2].mPosition.y < inVertices[outSortedIndices[0]].mPosition.y)
		{
			outSortedIndices[1] = outSortedIndices[0];
			outSortedIndices[0] = 2;
		}
		else if (inVertices[2].mPosition.y < inVertices[outSortedIndices[2]].mPosition.y)
		{
			outSortedIndices[1] = 2;
		}
		else
		{
			outSortedIndices[1] = outSortedIndices[2];
			outSortedIndices[2] = 2;
		}
	}
}


void SoftwareRasterizer::drawTriangle(const Vertex_P2_T2* vertices, const Bitmap& texture)
{
	// TODO: All of this does not handle vertices outside the valid rectangle at all!

	// Sort vertices by y-coordinate
	int sortedIndices[3];
	sortThreeVerticesByY(vertices, sortedIndices);

	const Vertex_P2_T2& vertex0 = vertices[sortedIndices[0]];
	const Vertex_P2_T2& vertex1 = vertices[sortedIndices[1]];
	const Vertex_P2_T2& vertex2 = vertices[sortedIndices[2]];

	// Check for a really flat triangle that does not cross any integer line at all
	if (std::floor(vertex0.mPosition.y) == std::floor(vertex2.mPosition.y))
		return;

	// Is the middle vertex on the left or right side?
	Vertex_P2_T2 middleLeft = vertex1;
	Vertex_P2_T2 middleRight = vertex1;
	{
		// "Split" refers to the point between first and last vertex (in y-direction) with the same y-coordinate as the middle vertex
		const float splitFraction = (vertex1.mPosition.y - vertex0.mPosition.y) / (vertex2.mPosition.y - vertex0.mPosition.y);
		const float splitX = vertex0.mPosition.x + (vertex2.mPosition.x - vertex0.mPosition.x) * splitFraction;
		const float splitU = vertex0.mUV.x + (vertex2.mUV.x - vertex0.mUV.x) * splitFraction;
		const float splitV = vertex0.mUV.y + (vertex2.mUV.y - vertex0.mUV.y) * splitFraction;
		if (vertex1.mPosition.x < splitX)
		{
			middleRight.mPosition.set(splitX, vertex1.mPosition.y);
			middleRight.mUV.set(splitU, splitV);
		}
		else
		{
			middleLeft.mPosition.set(splitX, vertex1.mPosition.y);
			middleLeft.mUV.set(splitU, splitV);
		}
	}

	// Draw the triangle as two trapezoids with top and bottom parallel to the x-axis
	drawTrapezoid(vertex0, vertex0, middleLeft, middleRight, texture);
	drawTrapezoid(middleLeft, middleRight, vertex2, vertex2, texture);
}

void SoftwareRasterizer::drawTriangle(const Vertex_P2_C4* vertices)
{
	// TODO: All of this does not handle vertices outside the valid rectangle at all!

	// Sort vertices by y-coordinate
	int sortedIndices[3];
	sortThreeVerticesByY(vertices, sortedIndices);

	const Vertex_P2_C4& vertex0 = vertices[sortedIndices[0]];
	const Vertex_P2_C4& vertex1 = vertices[sortedIndices[1]];
	const Vertex_P2_C4& vertex2 = vertices[sortedIndices[2]];

	// Check for a really flat triangle that does not cross any integer line at all
	if (std::floor(vertex0.mPosition.y) == std::floor(vertex2.mPosition.y))
		return;

	// Is the middle vertex on the left or right side?
	Vertex_P2_C4 middleLeft = vertex1;
	Vertex_P2_C4 middleRight = vertex1;
	{
		// "Split" refers to the point between first and last vertex (in y-direction) with the same y-coordinate as the middle vertex
		const float splitFraction = (vertex1.mPosition.y - vertex0.mPosition.y) / (vertex2.mPosition.y - vertex0.mPosition.y);
		const float splitX = vertex0.mPosition.x + (vertex2.mPosition.x - vertex0.mPosition.x) * splitFraction;
		Color splitColor;
		for (int k = 0; k < 4; ++k)
		{
			splitColor[k] = vertex0.mColor[k] + (vertex2.mColor[k] - vertex0.mColor[k]) * splitFraction;
		}
		if (vertex1.mPosition.x < splitX)
		{
			middleRight.mPosition.set(splitX, vertex1.mPosition.y);
			middleRight.mColor = splitColor;
		}
		else
		{
			middleLeft.mPosition.set(splitX, vertex1.mPosition.y);
			middleLeft.mColor = splitColor;
		}
	}

	// Draw the triangle as two trapezoids with top and bottom parallel to the x-axis
	drawTrapezoid(vertex0, vertex0, middleLeft, middleRight);
	drawTrapezoid(middleLeft, middleRight, vertex2, vertex2);
}

void SoftwareRasterizer::drawTrapezoid(const Vertex_P2_T2& vertex00, const Vertex_P2_T2& vertex10, const Vertex_P2_T2& vertex01, const Vertex_P2_T2& vertex11, const Bitmap& texture)
{
	const float startY = vertex00.mPosition.y;	// Can be assumed to be the same as vertex10.mPosition.y
	const float endY = vertex01.mPosition.y;	// Can be assumed to be the same as vertex11.mPosition.y

	// Any integer line in between at all?
	if (std::floor(endY) == std::floor(startY))
		return;

	const float rangeY = endY - startY;
	Vertex_P2_T2 vertexLeft  = vertex00;
	Vertex_P2_T2 vertexRight = vertex10;

	// Changes of left and right vertex when advancing by one in y-direction
	Vertex_P2_T2 advanceLeft;
	Vertex_P2_T2 advanceRight;
	advanceLeft.mPosition  = (vertex01.mPosition - vertex00.mPosition) / rangeY;
	advanceRight.mPosition = (vertex11.mPosition - vertex10.mPosition) / rangeY;
	advanceLeft.mUV  = (vertex01.mUV - vertex00.mUV) / rangeY;
	advanceRight.mUV = (vertex11.mUV - vertex10.mUV) / rangeY;

	// Move to the first integer line
	const float firstIntegerY = std::ceil(startY);
	const float firstStepY = firstIntegerY - startY;
	vertexLeft.mPosition  += advanceLeft.mPosition * firstStepY;
	vertexRight.mPosition += advanceRight.mPosition * firstStepY;
	vertexLeft.mUV  += advanceLeft.mUV * firstStepY;
	vertexRight.mUV += advanceRight.mUV * firstStepY;

	int minY = (int)(firstIntegerY);
	int maxY = (int)(std::floor(endY));
	minY = clamp(minY, 0, mOutput.getSize().y);
	maxY = clamp(maxY, -1, mOutput.getSize().y-1);

	const float scaleU = (float)(texture.getWidth() - 1);
	const float scaleV = (float)(texture.getHeight() - 1);

	for (int y = minY; y <= maxY; ++y)
	{
		int minX = (int)(std::ceil(vertexLeft.mPosition.x));
		int maxX = (int)(std::floor(vertexRight.mPosition.x));
		minX = clamp(minX, 0, mOutput.getSize().x);
		maxX = clamp(maxX, -1, mOutput.getSize().x-1);

		if (minX <= maxX)
		{
			uint32* data = mOutput.getPixelPointer(minX, y);
			const Vec2f diffUV = (vertexRight.mUV - vertexLeft.mUV) / std::max(vertexRight.mPosition.x - vertexLeft.mPosition.x, 1.0f);
			Vec2f currentUV = vertexLeft.mUV + diffUV * ((float)(minX) - vertexLeft.mPosition.x);

			if (mOptions.mBlendMode == BlendMode::ALPHA)
			{
				uint8* bytes = (uint8*)data;
				if (mOptions.mSamplingMode == SamplingMode::POINT)
				{
					for (int x = minX; x <= maxX; ++x)
					{
						const float u = currentUV.x - std::floor(currentUV.x);
						const float v = currentUV.y - std::floor(currentUV.y);

						// Point sampling
						const int sampleX = roundToInt(u * scaleU);
						const int sampleY = roundToInt(v * scaleV);
						const uint32 texColor = texture.getPixel(sampleX, sampleY);

						// Alpha blending
						const uint16 multiplierA = ((texColor >> 16) & 0xff00) / 255;
						const uint16 multiplierB = 256 - multiplierA;
						bytes[0] = (((texColor)       & 0xff) * multiplierA + bytes[0] * multiplierB) >> 8;
						bytes[1] = (((texColor >> 8)  & 0xff) * multiplierA + bytes[1] * multiplierB) >> 8;
						bytes[2] = (((texColor >> 16) & 0xff) * multiplierA + bytes[2] * multiplierB) >> 8;
						bytes[3] = 0xff;

						bytes += 4;
						currentUV += diffUV;
					}
				}
				else
				{
					for (int x = minX; x <= maxX; ++x)
					{
						const float u = currentUV.x - std::floor(currentUV.x);
						const float v = currentUV.y - std::floor(currentUV.y);

						// Bilinear sampling
						// TODO: There's plenty of room for optimizations here
						const float sampleFloatX = u * scaleU;
						const float sampleFloatY = v * scaleV;
						const float factorX = sampleFloatX - std::floor(sampleFloatX);
						const float factorY = sampleFloatY - std::floor(sampleFloatY);
						const int sampleX0 = (int)(sampleFloatX);
						const int sampleY0 = (int)(sampleFloatY);
						const int sampleX1 = std::min(sampleX0 + 1, texture.getWidth() - 1);
						const int sampleY1 = std::min(sampleY0 + 1, texture.getHeight() - 1);
						const uint8* sample00 = (const uint8*)texture.getPixelPointer(sampleX0, sampleY0);
						const uint8* sample01 = (const uint8*)texture.getPixelPointer(sampleX0, sampleY1);
						const uint8* sample10 = (const uint8*)texture.getPixelPointer(sampleX1, sampleY0);
						const uint8* sample11 = (const uint8*)texture.getPixelPointer(sampleX1, sampleY1);
						const float r = ((float)sample00[0] * (1.0f - factorX) + (float)sample10[0] * factorX) * (1.0f - factorY) + ((float)sample01[0] * (1.0f - factorX) + (float)sample11[0] * factorX) * factorY;
						const float g = ((float)sample00[1] * (1.0f - factorX) + (float)sample10[1] * factorX) * (1.0f - factorY) + ((float)sample01[1] * (1.0f - factorX) + (float)sample11[1] * factorX) * factorY;
						const float b = ((float)sample00[2] * (1.0f - factorX) + (float)sample10[2] * factorX) * (1.0f - factorY) + ((float)sample01[2] * (1.0f - factorX) + (float)sample11[2] * factorX) * factorY;
						const float a = ((float)sample00[3] * (1.0f - factorX) + (float)sample10[3] * factorX) * (1.0f - factorY) + ((float)sample01[3] * (1.0f - factorX) + (float)sample11[3] * factorX) * factorY;

						// Alpha blending
						bytes[0] = (uint8)((r * a + (float)bytes[0] * (255.0f - a)) / 255.0f);
						bytes[1] = (uint8)((g * a + (float)bytes[1] * (255.0f - a)) / 255.0f);
						bytes[2] = (uint8)((b * a + (float)bytes[2] * (255.0f - a)) / 255.0f);
						bytes[3] = 0xff;

						bytes += 4;
						currentUV += diffUV;
					}
				}
			}
			else
			{
				for (int x = minX; x <= maxX; ++x)
				{
					const float u = currentUV.x - std::floor(currentUV.x);
					const float v = currentUV.y - std::floor(currentUV.y);

					// Point sampling
					const int sampleX = roundToInt(u * scaleU);
					const int sampleY = roundToInt(v * scaleV);
					const uint32 texColor = texture.getPixel(sampleX, sampleY);

					// TODO: Support bilinear sampling here as well

					// No blending
					*data = texColor;

					++data;
					currentUV += diffUV;
				}
			}
		}

		vertexLeft.mPosition  += advanceLeft.mPosition;
		vertexRight.mPosition += advanceRight.mPosition;
		vertexLeft.mUV  += advanceLeft.mUV;
		vertexRight.mUV += advanceRight.mUV;
	}
}

void SoftwareRasterizer::drawTrapezoid(const Vertex_P2_C4& vertex00, const Vertex_P2_C4& vertex10, const Vertex_P2_C4& vertex01, const Vertex_P2_C4& vertex11)
{
	const float startY = vertex00.mPosition.y;	// Can be assumed to be the same as vertex10.mPosition.y
	const float endY = vertex01.mPosition.y;	// Can be assumed to be the same as vertex11.mPosition.y

	// Any integer line in between at all?
	if (std::floor(endY) == std::floor(startY))
		return;

	const float rangeY = endY - startY;
	Vertex_P2_C4 vertexLeft  = vertex00;
	Vertex_P2_C4 vertexRight = vertex10;

	// Changes of left and right vertex when advancing by one in y-direction
	Vertex_P2_C4 advanceLeft;
	Vertex_P2_C4 advanceRight;
	advanceLeft.mPosition  = (vertex01.mPosition - vertex00.mPosition) / rangeY;
	advanceRight.mPosition = (vertex11.mPosition - vertex10.mPosition) / rangeY;
	for (int k = 0; k < 4; ++k)
	{
		advanceLeft.mColor[k]  = (vertex01.mColor[k] - vertex00.mColor[k]) / rangeY;
		advanceRight.mColor[k]  = (vertex11.mColor[k] - vertex10.mColor[k]) / rangeY;
	}

	// Move to the first integer line
	const float firstIntegerY = std::ceil(startY);
	const float firstStepY = firstIntegerY - startY;
	vertexLeft.mPosition  += advanceLeft.mPosition * firstStepY;
	vertexRight.mPosition += advanceRight.mPosition * firstStepY;
	for (int k = 0; k < 4; ++k)
	{
		vertexLeft.mColor[k] += advanceLeft.mColor[k] * firstStepY;
		vertexRight.mColor[k] += advanceRight.mColor[k] * firstStepY;
	}

	int minY = (int)(firstIntegerY);
	int maxY = (int)(std::floor(endY));
	minY = clamp(minY, 0, mOutput.getSize().y);
	maxY = clamp(maxY, -1, mOutput.getSize().y-1);

	for (int y = minY; y <= maxY; ++y)
	{
		int minX = (int)(std::ceil(vertexLeft.mPosition.x));
		int maxX = (int)(std::floor(vertexRight.mPosition.x));
		minX = clamp(minX, 0, mOutput.getSize().x);
		maxX = clamp(maxX, -1, mOutput.getSize().x-1);

		if (minX <= maxX)
		{
			uint32* data = mOutput.getPixelPointer(minX, y);
			const float factor1 = 1.0f / std::max(vertexRight.mPosition.x - vertexLeft.mPosition.x, 1.0f);
			const float factor2 = (float)minX - vertexLeft.mPosition.x;
			Color diffColor;
			Color currentColor;
			for (int k = 0; k < 4; ++k)
			{
				diffColor[k] = (vertexRight.mColor[k] - vertexLeft.mColor[k]) * factor1;
				currentColor[k] = vertexLeft.mColor[k] + diffColor[k] * factor2;
			}

			if (mOptions.mBlendMode == BlendMode::ALPHA)
			{
				uint8* bytes = (uint8*)data;
				for (int x = minX; x <= maxX; ++x)
				{
					const uint32 texColor = currentColor.getABGR32();

					// Alpha blending
					const uint16 multiplierA = ((texColor >> 16) & 0xff00) / 255;
					const uint16 multiplierB = 256 - multiplierA;
					bytes[0] = (((texColor)       & 0xff) * multiplierA + bytes[0] * multiplierB) >> 8;
					bytes[1] = (((texColor >> 8)  & 0xff) * multiplierA + bytes[1] * multiplierB) >> 8;
					bytes[2] = (((texColor >> 16) & 0xff) * multiplierA + bytes[2] * multiplierB) >> 8;
					bytes[3] = 0xff;

					bytes += 4;
					for (int k = 0; k < 4; ++k)
					{
						currentColor[k] += diffColor[k];
					}
				}
			}
			else
			{
				for (int x = minX; x <= maxX; ++x)
				{
					// No blending
					*data = currentColor.getABGR32();

					++data;
					for (int k = 0; k < 4; ++k)
					{
						currentColor[k] += diffColor[k];
					}
				}
			}
		}

		vertexLeft.mPosition  += advanceLeft.mPosition;
		vertexRight.mPosition += advanceRight.mPosition;
		for (int k = 0; k < 4; ++k)
		{
			vertexLeft.mColor[k]  += advanceLeft.mColor[k];
			vertexRight.mColor[k] += advanceRight.mColor[k];
		}
	}
}
