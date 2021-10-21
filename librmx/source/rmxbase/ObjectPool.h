/*
*	rmx Library
*	Copyright (C) 2008-2021 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


namespace rmx
{
	namespace detail
	{
		template<bool AUTOCONSTRUCT>
		struct AutoConstruct
		{
			template<typename T>
			static void construct(void* ptr)
			{
			}
		};

		template<>
		struct AutoConstruct<true>
		{
			template<typename T>
			static void construct(void* ptr)
			{
				new (ptr) T();
			}
		};
	}
}


// Base class (should not be used directly)
template<typename T, int RMX_PAGESIZE = 32, bool AUTOCONSTRUCT = false>
class ObjectPoolBase
{
public:
	inline explicit ObjectPoolBase(size_t pageSize = RMX_PAGESIZE) :
		mPageSize(pageSize)
	{
	}

	inline ~ObjectPoolBase()
	{
		// Free all reserved memory
		clear();
	}

	inline void clear()
	{
		// Free all pages
		for (Page& page : mPages)
		{
			for (size_t i = 0; i < page.mSize; ++i)
			{
				Item& item = page.mItems[i];
				if (item.mIsConstructed)
				{
					// Call object's destructor
					item.mObject.~T();
				}
			}
			free(page.mItems);
		}

		// Clear arrays of pages and free objects
		mPages.clear();
		mFreeItems.clear();
	}

	inline bool empty() const
	{
		// Get total number of reserved objects
		size_t totalNumberOfObjects = 0;
		for (Page& page : mPages)
		{
			totalNumberOfObjects += page.mSize;
		}

		// Compare to the number of unused objects
		return (mFreeItems.size() == totalNumberOfObjects);
	}

	inline bool isManaged(const T& object) const
	{
		// Check if the given object is managed by this pool
		for (const Page& page : mPages)
		{
			const int offset = static_cast<int>((char*)&object - (char*)&(page.mItems[0]));
			if (offset >= 0 && (size_t)offset < sizeof(T) * page.mSize && offset % sizeof(T) == 0)
				return true;
		}
		return false;
	}

	inline bool isUsed(const T& object) const
	{
		return (isManaged(object) && !isUnused(object));
	}

	inline bool isUnused(const T& object) const
	{
		const Item& item = reinterpret_cast<const Item&>(object);
		return !item.mIsUsed;
	}

protected:
	// An item is our little management struct that includes an object and some additional info needed
	struct Item
	{
		T mObject;
		bool mIsUsed;			// Set if object is currently used
		bool mIsConstructed;	// Set if constructor was called in the object
	};

	// A page is basically an array of items that all get allocated as one chunk of memory
	struct Page
	{
		size_t mSize;
		Item* mItems;
	};

protected:
	inline T& allocObject()
	{
		return allocItem().mObject;
	}

	inline Item& allocItem()
	{
		// Is there still a free object?
		if (mFreeItems.empty())
		{
			// Create a new page
			Page page;
			page.mSize = mPageSize;

			const size_t memorySize = sizeof(Item) * page.mSize;
			page.mItems = static_cast<Item*>(malloc(memorySize));

			// Initialize items
			for (size_t i = 0; i < page.mSize; ++i)
			{
				Item& item = page.mItems[i];
				item.mIsUsed = false;
				item.mIsConstructed = AUTOCONSTRUCT;
				rmx::detail::AutoConstruct<AUTOCONSTRUCT>::template construct<T>(static_cast<void*>(&item.mObject));
			}

			// Store page
			mPages.push_back(page);

			// Add newly created objects to the array of free objects, in inverse order
			mFreeItems.resize(page.mSize);
			for (size_t i = 0; i < page.mSize; ++i)
			{
				mFreeItems[i] = &page.mItems[page.mSize-i-1];
			}
		}

		// Get the next free object
		Item* result = mFreeItems.back();
		mFreeItems.pop_back();
		result->mIsUsed = true;

		// Done
		return *result;
	}

	inline T& rentObject()
	{
		return allocObject();
	}

	inline void freeObject(T& object)
	{
		Item& item = reinterpret_cast<Item&>(object);

		// Free an object by simply adding it to the array of free objects
		mFreeItems.push_back(&item);
		item.mIsUsed = false;
	}

	inline void returnObject(T& object)
	{
		freeObject(object);
	}

	inline T& createObject()
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T();
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	template<typename A>
	T& createObject(A&& a)
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T(a);
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	template<typename A, typename B>
	T& createObject(A&& a, B&& b)
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T(a, b);
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	template<typename A, typename B, typename C>
	T& createObject(A&& a, B&& b, C&& c)
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T(a, b, c);
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	template<typename A, typename B, typename C, typename D>
	T& createObject(A&& a, B&& b, C&& c, D&& d)
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T(a, b, c, d);
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	template<typename A, typename B, typename C, typename D, typename E>
	T& createObject(A&& a, B&& b, C&& c, D&& d, E&& e)
	{
		// Get an unused item
		Item& item = allocItem();

		// Call its constructor
		new (static_cast<void*>(&item.mObject)) T(a, b, c, d, e);
		item.mIsConstructed = true;

		// Done
		return item.mObject;
	}

	inline void destroyObject(T& object)
	{
		Item& item = reinterpret_cast<Item&>(object);

		// Call object's destructor
		object.~T();
		item.mIsConstructed = false;

		// Free the object
		freeObject(object);
	}

private:
	const size_t mPageSize;			// Number of items to be reserved per page; set by the constructor and must not be changed afterwards
	std::vector<Page>  mPages;		// Array of pages holding used and unused items
	std::vector<Item*> mFreeItems;	// Array of currently unused items
};



// Object pool variant for objects that don't need to be constructed (usually plain old data structs)
template<typename T, int RMX_PAGESIZE = 32>
class PodStructPool : public ObjectPoolBase<T, RMX_PAGESIZE, false>
{
public:
	using ObjectPoolBase<T, RMX_PAGESIZE, false>::allocObject;
	using ObjectPoolBase<T, RMX_PAGESIZE, false>::freeObject;
};


// Object pool variant for objects that need to be constructed / destructed on demand
template<typename T, int RMX_PAGESIZE = 32>
class ObjectPool : public ObjectPoolBase<T, RMX_PAGESIZE, false>
{
public:
	using ObjectPoolBase<T, RMX_PAGESIZE, false>::createObject;
	using ObjectPoolBase<T, RMX_PAGESIZE, false>::destroyObject;
};


// Object pool variant for objects that need to stay constructed all the time, but can be rented and returned
template<typename T, int RMX_PAGESIZE = 16>
class RentableObjectPool : public ObjectPoolBase<T, RMX_PAGESIZE, true>
{
public:
	using ObjectPoolBase<T, RMX_PAGESIZE, true>::rentObject;
	using ObjectPoolBase<T, RMX_PAGESIZE, true>::returnObject;
};
