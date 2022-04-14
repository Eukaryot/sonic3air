/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "Bitmap.h"


class IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const  { return false; }
	virtual bool canEncode(const String& format) const  { return false; }
	virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult)  { return false; }
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream)  { return false; }
};

class API_EXPORT BitmapCodecBMP : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecPNG : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecJPG : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};

class API_EXPORT BitmapCodecICO : public IBitmapCodec
{
public:
	virtual bool canDecode(const String& format) const;
	virtual bool canEncode(const String& format) const;
	virtual bool decode(Bitmap& bitmap, InputStream& stream, Bitmap::LoadResult& outResult);
	virtual bool encode(const Bitmap& bitmap, OutputStream& stream);
};
