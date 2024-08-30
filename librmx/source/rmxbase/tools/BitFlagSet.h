/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include "rmxbase/base/Types.h"


namespace rmx::detail
{
	template<int BYTES> struct TypeBySize
	{
		// Does not define the "Type" so there will be a compiler error
	};
	template<> struct TypeBySize<1>
	{
		typedef uint8 Type;
	};
	template<> struct TypeBySize<2>
	{
		typedef uint16 Type;
	};
	template<> struct TypeBySize<4>
	{
		typedef uint32 Type;
	};
	template<> struct TypeBySize<8>
	{
		typedef uint64 Type;
	};
}


template<typename ENUM>
class BitFlagSet
{
public:
	typedef ENUM Enum;
	typedef typename rmx::detail::TypeBySize<sizeof(ENUM)>::Type Storage;

public:
	inline BitFlagSet() : mFlags(0) {}
	inline BitFlagSet(Enum value) : mFlags(static_cast<Storage>(value)) {}
	inline BitFlagSet(Enum valueA, Enum valueB) : mFlags(static_cast<Storage>(valueA) | static_cast<Storage>(valueB)) {}
	inline BitFlagSet(Enum valueA, Enum valueB, Enum valueC) : mFlags(static_cast<Storage>(valueA) | static_cast<Storage>(valueB) | static_cast<Storage>(valueC)) {}
	inline BitFlagSet(Enum valueA, Enum valueB, Enum valueC, Enum valueD) : mFlags(static_cast<Storage>(valueA) | static_cast<Storage>(valueB) | static_cast<Storage>(valueC) | static_cast<Storage>(valueD)) {}
	inline explicit BitFlagSet(Storage flags) : mFlags(flags) {}
	inline BitFlagSet(const BitFlagSet& other) : mFlags(other.mFlags) {}

	inline bool isSet(Enum flag) const
	{
		return (mFlags & static_cast<Storage>(flag)) == static_cast<Storage>(flag);
	}

	inline bool isClear(Enum flag) const
	{
		return (mFlags & static_cast<Storage>(flag)) == 0;
	}

	inline bool allSet(BitFlagSet bitmask) const
	{
		return (mFlags & bitmask.mFlags) == bitmask;
	}

	inline bool anySet(BitFlagSet bitmask) const
	{
		return (mFlags & bitmask.mFlags) != 0;
	}

	inline bool anySet() const
	{
		return (mFlags != 0);
	}

	inline void set(Enum flag)
	{
		mFlags |= static_cast<Storage>(flag);
	}

	inline void set(Storage flags)
	{
		mFlags |= flags;
	}

	inline void set(BitFlagSet bitmask)
	{
		mFlags |= bitmask.mFlags;
	}

	inline void set(Enum flag, bool value)
	{
		if (value)
		{
			mFlags |= static_cast<Storage>(flag);
		}
		else
		{
			mFlags &= ~static_cast<Storage>(flag);
		}
	}

	inline void setOnly(Enum flag)
	{
		mFlags = static_cast<Storage>(flag);
	}

	inline void clearAll()
	{
		mFlags = 0;
	}

	inline void clear(Enum flag)
	{
		mFlags &= ~static_cast<Storage>(flag);
	}

	inline void clear(Storage flags)
	{
		mFlags &= ~flags;
	}

	inline void clear(BitFlagSet bitmask)
	{
		mFlags &= ~bitmask.mFlags;
	}

	inline void toggle(Enum flag)
	{
		mFlags ^= static_cast<Storage>(flag);
	}

	inline Storage getValue() const  { return mFlags; }
	inline Storage& accessValue()	 { return mFlags; }

	inline bool operator==(const BitFlagSet& other) const
	{
		return (mFlags == other.mFlags);
	}

	inline bool operator!=(const BitFlagSet& other) const
	{
		return (mFlags != other.mFlags);
	}

	inline BitFlagSet& operator=(const BitFlagSet& other)
	{
		mFlags = other.mFlags;
		return *this;
	}

	inline BitFlagSet operator|(const BitFlagSet& other) const	{ return BitFlagSet(mFlags | other.mFlags); }
	inline BitFlagSet operator&(const BitFlagSet& other) const	{ return BitFlagSet(mFlags & other.mFlags); }
	inline BitFlagSet operator^(const BitFlagSet& other) const	{ return BitFlagSet(mFlags ^ other.mFlags); }

	inline BitFlagSet operator|(Enum flag) const  { return BitFlagSet(mFlags | static_cast<Storage>(flag)); }
	inline BitFlagSet operator&(Enum flag) const  { return BitFlagSet(mFlags & static_cast<Storage>(flag)); }
	inline BitFlagSet operator^(Enum flag) const  { return BitFlagSet(mFlags ^ static_cast<Storage>(flag)); }

	inline void operator|=(const BitFlagSet& other)  { mFlags |= other.mFlags; }
	inline void operator&=(const BitFlagSet& other)  { mFlags &= other.mFlags; }
	inline void operator^=(const BitFlagSet& other)  { mFlags ^= other.mFlags; }

	inline void operator|=(Enum flag)  { mFlags |= static_cast<Storage>(flag); }
	inline void operator&=(Enum flag)  { mFlags &= static_cast<Storage>(flag); }
	inline void operator^=(Enum flag)  { mFlags ^= static_cast<Storage>(flag); }

private:
	Storage mFlags = 0;   // Bitmask of the currently set flags, composed using bitwise OR
};


template<typename ENUM> BitFlagSet<ENUM> makeBitFlagSet(ENUM valueA)										{ return BitFlagSet<ENUM>(valueA); }
template<typename ENUM> BitFlagSet<ENUM> makeBitFlagSet(ENUM valueA, ENUM valueB)							{ return BitFlagSet<ENUM>(valueA, valueB); }
template<typename ENUM> BitFlagSet<ENUM> makeBitFlagSet(ENUM valueA, ENUM valueB, ENUM valueC)				{ return BitFlagSet<ENUM>(valueA, valueB, valueC); }
template<typename ENUM> BitFlagSet<ENUM> makeBitFlagSet(ENUM valueA, ENUM valueB, ENUM valueC, ENUM valueD)	{ return BitFlagSet<ENUM>(valueA, valueB, valueC, valueD); }
