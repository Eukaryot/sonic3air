/*
*	Part of the Oxygen Engine / Sonic 3 A.I.R. software distribution.
*	Copyright (C) 2017-2026 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <rmxbase.h>


namespace lemon
{
	namespace detail
	{
		class FlyweightStringManager
		{
		public:
			struct Entry
			{
				uint64 mHash = 0;
				std::string_view mString;
			};

		public:
			FlyweightStringManager();

		public:
			rmx::OneTimeAllocPool mAllocPool;
			std::unordered_map<uint64, Entry*> mEntryMap;
		};
	}


	class FlyweightString
	{
	public:
		inline FlyweightString() {}
		inline explicit FlyweightString(uint64 hash) { set(hash); }
		inline FlyweightString(const char* str) { set(str); }
		inline FlyweightString(std::string_view str) { set(str); }
		inline FlyweightString(const std::string& str) { set(str); }
		inline FlyweightString(const FlyweightString& other) : mEntry(other.mEntry) {}

		inline bool isValid() const  { return (nullptr != mEntry); }
		inline bool isEmpty() const  { return (nullptr == mEntry || mEntry->mString.empty()); }

		inline uint64 getHash() const				 { return (nullptr != mEntry) ? mEntry->mHash : 0; }
		std::string_view getString() const			 { return (nullptr != mEntry) ? mEntry->mString : std::string_view(); }
		const std::string_view& getStringRef() const { return (nullptr != mEntry) ? mEntry->mString : EMPTY_STRING_VIEW; }

		void set(uint64 hash);
		void set(uint64 hash, std::string_view name);
		void set(std::string_view name);

		void operator=(const FlyweightString& other)  { mEntry = other.mEntry; }

		bool operator==(const FlyweightString& other) const  { return (mEntry == other.mEntry); }
		bool operator!=(const FlyweightString& other) const  { return (mEntry != other.mEntry); }

		void serialize(VectorBinarySerializer& serializer);
		void write(VectorBinarySerializer& serializer) const;

	private:
		detail::FlyweightStringManager::Entry* mEntry = nullptr;

		inline static detail::FlyweightStringManager mManager;
		inline static std::string_view EMPTY_STRING_VIEW;
	};


	inline std::ostream& operator<<(std::ostream& stream, const FlyweightString& flyweightString)
	{
		return stream << flyweightString.getString();
	}
}
