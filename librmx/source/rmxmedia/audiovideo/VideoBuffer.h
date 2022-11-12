/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*
*	VideoBuffer
*		Buffers single images of a video.
*/

#pragma once


class VideoBuffer
{
public:
	VideoBuffer();
	~VideoBuffer();

	void clear();
	void clear(int width, int height);
	void clear(int width, int height, int widthUV, int heightUV);
	inline void setFramesPerSecond(float framerate) { mFramesPerSecond = framerate; }

	void addImageRGBA(uint32* data, int stride = 0);
	void addImageYUV(uint8* dataY, uint8* dataU, uint8* dataV, int strideY = 0, int strideUV = 0);

	const uint32* getImageRGBA(int num);
	const uint8* getImageY(int num);
	const uint8* getImageU(int num);
	const uint8* getImageV(int num);
	const uint8* getImageYUV(int num, int channel);

	void convertYUVtoRGBA(int num);

	int getFrameNumber(float time) const { return (int)(time * mFramesPerSecond); }

	inline int getWidth() const			{ return mWidth; }
	inline int getHeight() const		{ return mHeight; }
	inline int getWidthUV() const		{ return mWidthUV; }
	inline int getHeightUV() const		{ return mHeightUV; }
	inline int getFrameCount() const	{ return (int)mFrames.size(); }
	inline float getLengthInSec() const	{ return (float)mFrames.size() / mFramesPerSecond; }

	inline const float* getCropRect() const { return mCropRect; }
	void getCropRect(float* croprect);
	void setCropRect(float left, float top, float right, float bottom);
	void setCropRect(int left, int top, int width, int height);

	void setObsoleteFrames(int count);
	inline bool isPersistent() const { return false; }

private:
	struct VideoFrame
	{
		uint32* bufferRGBA = nullptr;
		uint8* bufferYUV[3] = { nullptr };
	};

private:
	void deleteFrame(VideoFrame* frame);

private:
	std::vector<VideoFrame*> mFrames;
	int mWidth = 0;
	int mWidthUV = 0;
	int mHeight = 0;
	int mHeightUV = 0;
	float mFramesPerSecond = 25.0f;
	float mCropRect[4];

	// Tables for YUV -> RGB conversion
	static bool mConversionTablesInitialized;
	static uint32 mYTable[256];
	static uint32 mRVTable[256];
	static uint32 mGUTable[256];
	static uint32 mGVTable[256];
	static uint32 mBUTable[256];
};
