/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "oxygen/pch.h"
#include "oxygen/drawing/software/SoftwareRasterizer.h"


namespace
{
	// We actually need to regard the pixel center at fractions .5f as center of pixels, not the full integer coordinates
	//  -> At least that's how OpenGL handles things, so we mimic that behavior
	constexpr float PIXEL_OFFSET_CORRECTION = 0.5f;

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

	template<int USE_COLORS, bool BILINEAR_SAMPLING, bool ALPHA_BLENDING, bool SWAP_RED_BLUE>
	FORCE_INLINE void processTexturedLineInternal(uint8* output, int numPixels, Color currentColor, Color diffColor, Vec2f currentUV, Vec2f diffUV, Vec2f scaleUV, const BitmapView<uint32>& texture)
	{
		constexpr int SAMPLE_INDEX_R = SWAP_RED_BLUE ? 2 : 0;
		constexpr int SAMPLE_INDEX_G = 1;
		constexpr int SAMPLE_INDEX_B = SWAP_RED_BLUE ? 0 : 2;

		if constexpr (BILINEAR_SAMPLING)
		{
			currentUV.x -= 0.5f / scaleUV.x;
			currentUV.y -= 0.5f / scaleUV.y;
		}

		for (int i = 0; i < numPixels; ++i)
		{
			const float u = currentUV.x - std::floor(currentUV.x);
			const float v = currentUV.y - std::floor(currentUV.y);

			if constexpr (BILINEAR_SAMPLING)
			{
				// Bilinear sampling
				// TODO: There's certainly room for optimizations here
				const float sampleFloatX = u * scaleUV.x;
				const float sampleFloatY = v * scaleUV.y;
				const float factorX = sampleFloatX - std::floor(sampleFloatX);
				const float factorY = sampleFloatY - std::floor(sampleFloatY);
				const int sampleX0 = (int)(sampleFloatX);
				const int sampleY0 = (int)(sampleFloatY);
				const int sampleX1 = ((sampleX0 + 1) < texture.getSize().x) ? (sampleX0 + 1) : 0;
				const int sampleY1 = ((sampleY0 + 1) < texture.getSize().y) ? (sampleY0 + 1) : 0;
				const uint8* sample00 = (const uint8*)texture.getPixelPointer(sampleX0, sampleY0);
				const uint8* sample01 = (const uint8*)texture.getPixelPointer(sampleX0, sampleY1);
				const uint8* sample10 = (const uint8*)texture.getPixelPointer(sampleX1, sampleY0);
				const uint8* sample11 = (const uint8*)texture.getPixelPointer(sampleX1, sampleY1);
				float r = ((float)sample00[SAMPLE_INDEX_R] * (1.0f - factorX) + (float)sample10[SAMPLE_INDEX_R] * factorX) * (1.0f - factorY) + ((float)sample01[SAMPLE_INDEX_R] * (1.0f - factorX) + (float)sample11[SAMPLE_INDEX_R] * factorX) * factorY;
				float g = ((float)sample00[SAMPLE_INDEX_G] * (1.0f - factorX) + (float)sample10[SAMPLE_INDEX_G] * factorX) * (1.0f - factorY) + ((float)sample01[SAMPLE_INDEX_G] * (1.0f - factorX) + (float)sample11[SAMPLE_INDEX_G] * factorX) * factorY;
				float b = ((float)sample00[SAMPLE_INDEX_B] * (1.0f - factorX) + (float)sample10[SAMPLE_INDEX_B] * factorX) * (1.0f - factorY) + ((float)sample01[SAMPLE_INDEX_B] * (1.0f - factorX) + (float)sample11[SAMPLE_INDEX_B] * factorX) * factorY;

				if constexpr (USE_COLORS)
				{
					r *= currentColor.r;
					g *= currentColor.g;
					b *= currentColor.b;
				}

				if constexpr (ALPHA_BLENDING)
				{
					float a = ((float)sample00[3] * (1.0f - factorX) + (float)sample10[3] * factorX) * (1.0f - factorY) + ((float)sample01[3] * (1.0f - factorX) + (float)sample11[3] * factorX) * factorY;

					if constexpr (USE_COLORS)
					{
						a *= currentColor.a;
					}

					// Alpha blending
					output[0] = (uint8)((r * a + (float)output[0] * (255.0f - a)) / 255.0f);
					output[1] = (uint8)((g * a + (float)output[1] * (255.0f - a)) / 255.0f);
					output[2] = (uint8)((b * a + (float)output[2] * (255.0f - a)) / 255.0f);
					output[3] = 0xff;
				}
				else
				{
					// No blending
					output[0] = (uint8)(r / 255.0f);
					output[1] = (uint8)(g / 255.0f);
					output[2] = (uint8)(b / 255.0f);
					output[3] = 0xff;
				}
			}
			else
			{
				// Point sampling
				const int sampleX = (int)std::floor(u * scaleUV.x);
				const int sampleY = (int)std::floor(v * scaleUV.y);

				if constexpr (ALPHA_BLENDING)
				{
					// Alpha blending
					const uint8* sample = (const uint8*)texture.getPixelPointer(sampleX, sampleY);

					if constexpr (USE_COLORS)
					{
						const float multiplierA = (float)sample[3] / 255.0f * currentColor.a;
						const float multiplierB = 1.0f - multiplierA;

						output[0] = (uint8)((float)sample[SAMPLE_INDEX_R] * currentColor.r * multiplierA + (float)output[0] * multiplierB);
						output[1] = (uint8)((float)sample[SAMPLE_INDEX_G] * currentColor.g * multiplierA + (float)output[1] * multiplierB);
						output[2] = (uint8)((float)sample[SAMPLE_INDEX_B] * currentColor.b * multiplierA + (float)output[2] * multiplierB);
						output[3] = 0xff;
					}
					else
					{
						const uint16 multiplierA = ((uint32)sample[3] << 8) / 255;
						const uint16 multiplierB = (256 - multiplierA);

						output[0] = (sample[SAMPLE_INDEX_R] * multiplierA + output[0] * multiplierB) >> 8;
						output[1] = (sample[SAMPLE_INDEX_G] * multiplierA + output[1] * multiplierB) >> 8;
						output[2] = (sample[SAMPLE_INDEX_B] * multiplierA + output[2] * multiplierB) >> 8;
						output[3] = 0xff;
					}
				}
				else
				{
					// No blending
					const uint32 texColor = texture.getPixel(sampleX, sampleY);

					if constexpr (USE_COLORS)
					{
						const uint8* sample = (const uint8*)texture.getPixelPointer(sampleX, sampleY);

						output[0] = (uint8)((float)sample[SAMPLE_INDEX_R] * currentColor.r);
						output[1] = (uint8)((float)sample[SAMPLE_INDEX_G] * currentColor.g);
						output[2] = (uint8)((float)sample[SAMPLE_INDEX_B] * currentColor.b);
						output[3] = 0xff;
					}
					else
					{
						if constexpr (SWAP_RED_BLUE)
						{
							*reinterpret_cast<uint32*>(output) = (texColor & 0x00ff00) | ((texColor & 0xff0000) >> 16) | ((texColor & 0x0000ff) << 16) | 0xff000000;
						}
						else
						{
							*reinterpret_cast<uint32*>(output) = texColor | 0xff000000;
						}
					}
				}
			}

			output += 4;
			currentUV += diffUV;

			if constexpr (USE_COLORS == 2)
			{
				for (int k = 0; k < 4; ++k)
				{
					currentColor[k] += diffColor[k];
				}
			}
		}
	}

	template<int USE_COLORS>
	FORCE_INLINE void processTexturedLine(uint8* output, int numPixels, Color currentColor, Color diffColor, Vec2f currentUV, Vec2f diffUV, Vec2f scaleUV, const BitmapView<uint32>& texture, const Blitter::Options& options)
	{
		if (options.mSamplingMode == SamplingMode::POINT)
		{
			if (options.mBlendMode == BlendMode::ALPHA)
			{
				if (options.mSwapRedBlueChannels)
					processTexturedLineInternal<USE_COLORS, false, true, true>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
				else
					processTexturedLineInternal<USE_COLORS, false, true, false>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
			}
			else
			{
				if (options.mSwapRedBlueChannels)
					processTexturedLineInternal<USE_COLORS, false, false, true>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
				else
					processTexturedLineInternal<USE_COLORS, false, false, false>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
			}
		}
		else
		{
			if (options.mBlendMode == BlendMode::ALPHA)
			{
				if (options.mSwapRedBlueChannels)
					processTexturedLineInternal<USE_COLORS, true, true, true>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
				else
					processTexturedLineInternal<USE_COLORS, true, true, false>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
			}
			else
			{
				if (options.mSwapRedBlueChannels)
					processTexturedLineInternal<USE_COLORS, true, false, true>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
				else
					processTexturedLineInternal<USE_COLORS, true, false, false>(output, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture);
			}
		}
	}

	template<bool ALPHA_BLENDING>
	FORCE_INLINE void processUntexturedLine(uint8* output, int numPixels, Color currentColor, Color diffColor)
	{
		for (int x = 0; x < numPixels; ++x)
		{
			if constexpr (ALPHA_BLENDING)
			{
				const uint32 texColor = currentColor.getABGR32();

				// Alpha blending
				const uint16 multiplierA = ((texColor >> 16) & 0xff00) / 255;
				const uint16 multiplierB = 256 - multiplierA;

				output[0] = (((texColor)       & 0xff) * multiplierA + output[0] * multiplierB) >> 8;
				output[1] = (((texColor >> 8)  & 0xff) * multiplierA + output[1] * multiplierB) >> 8;
				output[2] = (((texColor >> 16) & 0xff) * multiplierA + output[2] * multiplierB) >> 8;
				output[3] = 0xff;
			}
			else
			{
				*reinterpret_cast<uint32*>(output) = currentColor.getABGR32();
			}

			output += 4;
			for (int k = 0; k < 4; ++k)
			{
				currentColor[k] += diffColor[k];
			}
		}
	}
}


void SoftwareRasterizer::setOutput(const BitmapViewMutable<uint32>& output)
{
	mOutput = output;
}

void SoftwareRasterizer::drawTriangle(const Vertex* vertices, const BitmapView<uint32>& texture, bool useVertexColors)
{
	// TODO: All of this does not handle vertices outside the valid rectangle at all!

	// Sort vertices by y-coordinate
	int sortedIndices[3];
	sortThreeVerticesByY(vertices, sortedIndices);

	const Vertex& vertex0 = vertices[sortedIndices[0]];
	const Vertex& vertex1 = vertices[sortedIndices[1]];
	const Vertex& vertex2 = vertices[sortedIndices[2]];

	// Check for a really flat triangle that does not cross any mid-pixel line at all
	if (std::floor(vertex0.mPosition.y - PIXEL_OFFSET_CORRECTION) == std::floor(vertex2.mPosition.y - PIXEL_OFFSET_CORRECTION))
		return;

	// Is the middle vertex on the left or right side?
	Vertex middleLeft = vertex1;
	Vertex middleRight = vertex1;
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
	drawTrapezoid(vertex0, vertex0, middleLeft, middleRight, texture, useVertexColors);
	drawTrapezoid(middleLeft, middleRight, vertex2, vertex2, texture, useVertexColors);
}

void SoftwareRasterizer::drawTriangle(const Vertex* vertices)
{
	// TODO: All of this does not handle vertices outside the valid rectangle at all!

	// Sort vertices by y-coordinate
	int sortedIndices[3];
	sortThreeVerticesByY(vertices, sortedIndices);

	const Vertex& vertex0 = vertices[sortedIndices[0]];
	const Vertex& vertex1 = vertices[sortedIndices[1]];
	const Vertex& vertex2 = vertices[sortedIndices[2]];

	// Check for a really flat triangle that does not cross any mid-pixel line at all
	if (std::floor(vertex0.mPosition.y - PIXEL_OFFSET_CORRECTION) == std::floor(vertex2.mPosition.y - PIXEL_OFFSET_CORRECTION))
		return;

	// Is the middle vertex on the left or right side?
	Vertex middleLeft = vertex1;
	Vertex middleRight = vertex1;
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

void SoftwareRasterizer::drawTrapezoid(const Vertex& vertex00, const Vertex& vertex10, const Vertex& vertex01, const Vertex& vertex11, const BitmapView<uint32>& texture, bool useVertexColors)
{
	const float startY = vertex00.mPosition.y - PIXEL_OFFSET_CORRECTION;	// Can be assumed to be the same as vertex10.mPosition.y
	const float endY = vertex01.mPosition.y - PIXEL_OFFSET_CORRECTION;	// Can be assumed to be the same as vertex11.mPosition.y

	// Any integer line in between at all?
	if (std::floor(endY) == std::floor(startY))
		return;

	const float rangeY = endY - startY;
	Vertex vertexLeft  = vertex00;
	Vertex vertexRight = vertex10;
	const bool anyColorFade = useVertexColors && (vertex00.mColor != vertex01.mColor || vertex00.mColor != vertex10.mColor || vertex00.mColor != vertex11.mColor);

	Vec2f scaleUV;
	scaleUV.x = (float)texture.getSize().x;
	scaleUV.y = (float)texture.getSize().y;

	// Changes of left and right vertex when advancing by one in y-direction
	Vertex advanceLeft;
	Vertex advanceRight;
	advanceLeft.mPosition  = (vertex01.mPosition - vertex00.mPosition) / rangeY;
	advanceRight.mPosition = (vertex11.mPosition - vertex10.mPosition) / rangeY;
	advanceLeft.mUV  = (vertex01.mUV - vertex00.mUV) / rangeY;
	advanceRight.mUV = (vertex11.mUV - vertex10.mUV) / rangeY;
	if (anyColorFade)
	{
		for (int k = 0; k < 4; ++k)
		{
			advanceLeft.mColor[k]  = (vertex01.mColor[k] - vertex00.mColor[k]) / rangeY;
			advanceRight.mColor[k] = (vertex11.mColor[k] - vertex10.mColor[k]) / rangeY;
		}
	}

	// Move to the first integer line
	const float firstIntegerY = std::ceil(startY);
	const float firstStepY = firstIntegerY - startY;
	vertexLeft.mPosition  += advanceLeft.mPosition * firstStepY - Vec2f(PIXEL_OFFSET_CORRECTION, 0.0f);
	vertexRight.mPosition += advanceRight.mPosition * firstStepY - Vec2f(PIXEL_OFFSET_CORRECTION, 0.0f);
	vertexLeft.mUV  += advanceLeft.mUV * firstStepY;
	vertexRight.mUV += advanceRight.mUV * firstStepY;
	if (anyColorFade)
	{
		for (int k = 0; k < 4; ++k)
		{
			vertexLeft.mColor[k]  += advanceLeft.mColor[k] * firstStepY;
			vertexRight.mColor[k] += advanceRight.mColor[k] * firstStepY;
		}
	}

	if (mOptions.mSwapRedBlueChannels)
	{
		vertexLeft.mColor.swapRedBlue();
		vertexRight.mColor.swapRedBlue();
		advanceLeft.mColor.swapRedBlue();
		advanceRight.mColor.swapRedBlue();
	}

	int minY = (int)(firstIntegerY);
	int maxY = (int)(std::floor(endY));
	minY = clamp(minY, 0, mOutput.getSize().y);
	maxY = clamp(maxY, -1, mOutput.getSize().y - 1);

	for (int y = minY; y <= maxY; ++y)
	{
		int minX = (int)(std::ceil(vertexLeft.mPosition.x));
		int maxX = (int)(std::floor(vertexRight.mPosition.x));
		minX = clamp(minX, 0, mOutput.getSize().x);
		maxX = clamp(maxX, -1, mOutput.getSize().x - 1);

		if (minX <= maxX)
		{
			uint32* outputData = mOutput.getPixelPointer(minX, y);
			uint8* outputBytes = reinterpret_cast<uint8*>(outputData);
			const int numPixels = maxX - minX + 1;

			const float factor1 = 1.0f / std::max(vertexRight.mPosition.x - vertexLeft.mPosition.x, 1.0f);
			const float factor2 = (float)minX - vertexLeft.mPosition.x;

			const Vec2f diffUV = (vertexRight.mUV - vertexLeft.mUV) * factor1;
			Vec2f currentUV = vertexLeft.mUV + diffUV * factor2;

			if (anyColorFade)
			{
				Color diffColor;
				Color currentColor;
				for (int k = 0; k < 4; ++k)
				{
					diffColor[k] = (vertexRight.mColor[k] - vertexLeft.mColor[k]) * factor1;
					currentColor[k] = vertexLeft.mColor[k] + diffColor[k] * factor2;
				}

				processTexturedLine<2>(outputBytes, numPixels, currentColor, diffColor, currentUV, diffUV, scaleUV, texture, mOptions);
			}
			else if (useVertexColors)
			{
				processTexturedLine<1>(outputBytes, numPixels, vertexLeft.mColor, Color::TRANSPARENT, currentUV, diffUV, scaleUV, texture, mOptions);
			}
			else
			{
				processTexturedLine<0>(outputBytes, numPixels, vertexLeft.mColor, Color::TRANSPARENT, currentUV, diffUV, scaleUV, texture, mOptions);
			}
		}

		vertexLeft.mPosition  += advanceLeft.mPosition;
		vertexRight.mPosition += advanceRight.mPosition;
		vertexLeft.mUV  += advanceLeft.mUV;
		vertexRight.mUV += advanceRight.mUV;
	}
}

void SoftwareRasterizer::drawTrapezoid(const Vertex& vertex00, const Vertex& vertex10, const Vertex& vertex01, const Vertex& vertex11)
{
	const float startY = vertex00.mPosition.y - PIXEL_OFFSET_CORRECTION;	// Can be assumed to be the same as vertex10.mPosition.y
	const float endY = vertex01.mPosition.y - PIXEL_OFFSET_CORRECTION;		// Can be assumed to be the same as vertex11.mPosition.y

	// Any integer line in between at all?
	if (std::floor(endY) == std::floor(startY))
		return;

	const float rangeY = endY - startY;
	Vertex vertexLeft  = vertex00;
	Vertex vertexRight = vertex10;
	const bool anyColorFade = (vertex00.mColor != vertex01.mColor || vertex00.mColor != vertex10.mColor || vertex00.mColor != vertex11.mColor);

	// Changes of left and right vertex when advancing by one in y-direction
	Vertex advanceLeft;
	Vertex advanceRight;
	advanceLeft.mPosition  = (vertex01.mPosition - vertex00.mPosition) / rangeY;
	advanceRight.mPosition = (vertex11.mPosition - vertex10.mPosition) / rangeY;
	if (anyColorFade)
	{
		for (int k = 0; k < 4; ++k)
		{
			advanceLeft.mColor[k]  = (vertex01.mColor[k] - vertex00.mColor[k]) / rangeY;
			advanceRight.mColor[k]  = (vertex11.mColor[k] - vertex10.mColor[k]) / rangeY;
		}
	}

	// Move to the first integer line
	const float firstIntegerY = std::ceil(startY);
	const float firstStepY = firstIntegerY - startY;
	vertexLeft.mPosition  += advanceLeft.mPosition * firstStepY - Vec2f(PIXEL_OFFSET_CORRECTION, 0.0f);
	vertexRight.mPosition += advanceRight.mPosition * firstStepY - Vec2f(PIXEL_OFFSET_CORRECTION, 0.0f);
	if (anyColorFade)
	{
		for (int k = 0; k < 4; ++k)
		{
			vertexLeft.mColor[k] += advanceLeft.mColor[k] * firstStepY;
			vertexRight.mColor[k] += advanceRight.mColor[k] * firstStepY;
		}
	}

	if (mOptions.mSwapRedBlueChannels)
	{
		vertexLeft.mColor.swapRedBlue();
		vertexRight.mColor.swapRedBlue();
		advanceLeft.mColor.swapRedBlue();
		advanceRight.mColor.swapRedBlue();
	}

	int minY = (int)(firstIntegerY);
	int maxY = (int)(std::floor(endY));
	minY = clamp(minY, 0, mOutput.getSize().y);
	maxY = clamp(maxY, -1, mOutput.getSize().y - 1);

	for (int y = minY; y <= maxY; ++y)
	{
		int minX = (int)(std::ceil(vertexLeft.mPosition.x));
		int maxX = (int)(std::floor(vertexRight.mPosition.x));
		minX = clamp(minX, 0, mOutput.getSize().x);
		maxX = clamp(maxX, -1, mOutput.getSize().x - 1);

		if (minX <= maxX)
		{
			uint32* outputData = mOutput.getPixelPointer(minX, y);
			uint8* outputBytes = reinterpret_cast<uint8*>(outputData);
			const int numPixels = maxX - minX + 1;

			if (anyColorFade)
			{
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
					processUntexturedLine<true>(outputBytes, numPixels, currentColor, diffColor);
				}
				else
				{
					processUntexturedLine<false>(outputBytes, numPixels, currentColor, diffColor);
				}
			}
			else
			{
				// No blending, fixed color
				const uint32 fixedColorABGR = vertexLeft.mColor.getABGR32();
				for (int x = minX; x <= maxX; ++x)
				{
					*outputData = fixedColorABGR;
					++outputData;
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
