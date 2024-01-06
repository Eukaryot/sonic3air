/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#include "rmxmedia.h"


// Tables for YUV -> RGB conversion
bool VideoBuffer::mConversionTablesInitialized = false;
uint32 VideoBuffer::mYTable[256];
uint32 VideoBuffer::mRVTable[256];
uint32 VideoBuffer::mGUTable[256];
uint32 VideoBuffer::mGVTable[256];
uint32 VideoBuffer::mBUTable[256];


VideoBuffer::VideoBuffer()
{
	mFramesPerSecond = 25.0f;
}

VideoBuffer::~VideoBuffer()
{
	clear();
}

void VideoBuffer::clear()
{
	for (int i = 0; i < (signed)mFrames.size(); ++i)
		deleteFrame(mFrames[i]);
	mFrames.clear();
	mWidth = 0;
	mWidthUV = 0;
	mHeight = 0;
	mHeightUV = 0;
}

void VideoBuffer::clear(int width, int height)
{
	clear();
	mWidth = width;
	mHeight = height;
}

void VideoBuffer::clear(int width, int height, int widthUV, int heightUV)
{
	clear();
	mWidth = width;
	mWidthUV = widthUV;
	mHeight = height;
	mHeightUV = heightUV;
}

void VideoBuffer::deleteFrame(VideoFrame* frame)
{
	if (nullptr == frame)
		return;
	delete[] frame->bufferRGBA;
	for (int j = 0; j < 3; ++j)
		delete[] frame->bufferYUV[j];
	delete frame;
}

void VideoBuffer::addImageRGBA(uint32* data, int stride)
{
	if (stride <= 0)
		stride = mWidth;
	VideoFrame* frame = new VideoFrame;
	frame->bufferRGBA = new uint32[mWidth*mHeight];
	for (int line = 0; line < mHeight; ++line)
		memcpy(&frame->bufferRGBA[line*mWidth], &data[line*stride], mWidth*sizeof(uint32));
	frame->bufferYUV[0] = nullptr;
	frame->bufferYUV[1] = nullptr;
	frame->bufferYUV[2] = nullptr;
	mFrames.push_back(frame);
}

void VideoBuffer::addImageYUV(uint8* dataY, uint8* dataU, uint8* dataV, int strideY, int strideUV)
{
	if (strideY <= 0)
		strideY = mWidth;
	if (strideUV <= 0)
		strideUV = mWidthUV;
	VideoFrame* frame = new VideoFrame;
	frame->bufferRGBA = nullptr;
	frame->bufferYUV[0] = new uint8[mWidth*mHeight];
	for (int line = 0; line < mHeight; ++line)
		memcpy(&frame->bufferYUV[0][line*mWidth], &dataY[line*strideY], mWidth);
	frame->bufferYUV[1] = new uint8[mWidthUV*mHeightUV];
	for (int line = 0; line < mHeightUV; ++line)
		memcpy(&frame->bufferYUV[1][line*mWidthUV], &dataU[line*strideUV], mWidthUV);
	frame->bufferYUV[2] = new uint8[mWidthUV*mHeightUV];
	for (int line = 0; line < mHeightUV; ++line)
		memcpy(&frame->bufferYUV[2][line*mWidthUV], &dataV[line*strideUV], mWidthUV);
	mFrames.push_back(frame);
}

const uint32* VideoBuffer::getImageRGBA(int num)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return nullptr;
	return mFrames[num]->bufferRGBA;
}

const uint8* VideoBuffer::getImageY(int num)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return nullptr;
	return mFrames[num]->bufferYUV[0];
}

const uint8* VideoBuffer::getImageU(int num)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return nullptr;
	return mFrames[num]->bufferYUV[1];
}

const uint8* VideoBuffer::getImageV(int num)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return nullptr;
	return mFrames[num]->bufferYUV[2];
}

const uint8* VideoBuffer::getImageYUV(int num, int channel)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return nullptr;
	if (channel < 0 || channel > 2)
		return nullptr;
	return mFrames[num]->bufferYUV[channel];
}

void VideoBuffer::convertYUVtoRGBA(int num)
{
	if (num < 0 || num >= (signed)mFrames.size() || nullptr == mFrames[num])
		return;
	if (nullptr == mFrames[num]->bufferYUV[0] || nullptr == mFrames[num]->bufferYUV[1] || nullptr == mFrames[num]->bufferYUV[2])
		return;

	if (!mConversionTablesInitialized)
	{
		const int scale = 1L << 13;
		for (unsigned int i = 0; i < 256; ++i)
		{
			const int temp = i - 128;
			mYTable[i]  = (unsigned int)((1.164f * scale + 0.5f) * (i - 16));	// Calc Y component
			mRVTable[i] = (unsigned int)((1.596f * scale + 0.5f) * temp);		// Calc R component
			mGUTable[i] = (unsigned int)((0.391f * scale + 0.5f) * temp);		// Calc G u & v components
			mGVTable[i] = (unsigned int)((0.813f * scale + 0.5f) * temp);
			mBUTable[i] = (unsigned int)((2.018f * scale + 0.5f) * temp);		// Calc B component
		}
		mConversionTablesInitialized = true;
	}

	if (nullptr == mFrames[num]->bufferRGBA)
		mFrames[num]->bufferRGBA = new uint32[mWidth*mHeight];

	// Conversion YUV -> RGB
	bool half_uv_width = (mWidthUV < mWidth);
	bool half_uv_height = (mHeightUV < mHeight);
	uint8* ySrc2 = mFrames[num]->bufferYUV[0];
	uint8* uSrc2 = mFrames[num]->bufferYUV[1];
	uint8* vSrc2 = mFrames[num]->bufferYUV[2];
	uint32* output = mFrames[num]->bufferRGBA;
	int r, g, b, cu, cv, bU, gUV, rV, rgbY;

	for (int y = 0; y < mHeight; ++y)
	{
		uint8* ySrc = ySrc2;
		uint8* ySrcEnd = ySrc2 + mWidth;
		uint8* uSrc = uSrc2;
		uint8* vSrc = vSrc2;
		int t = 0;
		while (ySrc != ySrcEnd)
		{
			// Get corresponding lookup values
			rgbY = mYTable[*ySrc];
			if (!half_uv_width || ((t = !t) == 1))
			{
				cu = *uSrc;
				cv = *vSrc;
				rV  = mRVTable[cv];
				gUV = mGUTable[cu] + mGVTable[cv];
				bU  = mBUTable[cu];
				uSrc++;
				vSrc++;
			}

			// Scale down - brings values back into the 8 bits of a uint8
			r = (rgbY + rV)  >> 13;
			g = (rgbY - gUV) >> 13;
			b = (rgbY + bU)  >> 13;
			*output = ((uint32)clamp(b, 0, 255) << 16)
					+ ((uint32)clamp(g, 0, 255) << 8)
					+  (uint32)clamp(r, 0, 255) + 0xff000000;
			++output;
			++ySrc;
		}
		ySrc2 += mWidth;
		if (!half_uv_height || y%2 == 1)
		{
			uSrc2 += mWidthUV;
			vSrc2 += mWidthUV;
		}
	}
}

void VideoBuffer::getCropRect(float* croprect)
{
	if (nullptr == croprect)
		return;
	for (int i = 0; i < 4; ++i)
		croprect[i] = mCropRect[i];
}

void VideoBuffer::setCropRect(float left, float top, float right, float bottom)
{
	mCropRect[0] = left;
	mCropRect[1] = top;
	mCropRect[2] = right;
	mCropRect[3] = bottom;
}

void VideoBuffer::setCropRect(int left, int top, int width, int height)
{
	const float scaleX = 1.0f / (float)mWidth;
	const float scaleY = 1.0f / (float)mHeight;
	setCropRect((float)left * scaleX, (float)top * scaleY,
				(float)(left + width) * scaleX, (float)(top + height) * scaleY);
}

void VideoBuffer::setObsoleteFrames(int count)
{
	count = std::min(count, (int)mFrames.size());
	for (int i = 0; i < count; ++i)
	{
		deleteFrame(mFrames[i]);
		mFrames[i] = nullptr;
	}
}
