/*
*	rmx Library
*	Copyright (C) 2008-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


struct SDL_RWops;

namespace rmx
{

	class API_EXPORT FileInputStreamSDL final : public InputStream
	{
	public:
		FileInputStreamSDL() {}
		FileInputStreamSDL(const String& filename);
		FileInputStreamSDL(const WString& filename);
		~FileInputStreamSDL();

		bool open(const String& filename);
		bool open(const WString& filename);

		const char* getType() const override { return "fileSDL"; }
		bool valid() const override { return nullptr != mContext; }
		void close() override;

		void  setPosition64(int64 pos);
		int64 getPosition64() const;
		int64 getSize64() const;

		void setPosition(size_t pos) override  { setPosition64((int64)pos); }
		size_t getPosition() const override  { return (size_t)getPosition64(); }
		size_t getSize() const override  { return (size_t)getSize64(); }

		using InputStream::read;
		size_t read(void* dst, size_t len) override;
		void skip(size_t len) override;
		bool tryRead(const void* data, size_t len) override;
		StreamingState getStreamingState() override;

	private:
		SDL_RWops* mContext = nullptr;
		StreamingState mLastStreamingState = StreamingState::COMPLETED;
	};

}
