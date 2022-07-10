/*
*	rmx Library
*	Copyright (C) 2008-2022 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once


template<typename TYPE>
class CArray
{
public:
	TYPE* list;			// List of elements
	size_t size;		// Reserved size of list
	size_t count;		// Number of elements

public:
	CArray() : list(nullptr), size(0), count(0)  {}
	CArray(TYPE* ptr, size_t cnt) : list(nullptr), count(0), size(0)  { add(ptr, cnt); }
	~CArray()  { SAFE_DELETE_ARRAY(list); }

	void clear()
	{
		SAFE_DELETE_ARRAY(list);
		size = 0;
		count = 0;
	}

	void resize(size_t minsize)
	{
		if (minsize <= size)
			return;
		size = 16;
		while (size < minsize)
			size *= 2;
		TYPE* newlist = new TYPE[size];
		if (list)
		{
			for (size_t i = 0; i < count; ++i)
				newlist[i] = list[i];
			delete[] list;
		}
		list = newlist;
	}

	void resizeTo(size_t exactsize)
	{
		if (exactsize == size)
			return;
		size = exactsize;
		TYPE* newlist = new TYPE[size];
		if (list)
		{
			for (size_t i = 0; i < count; ++i)
				newlist[i] = list[i];
			delete[] list;
		}
		list = newlist;
	}

	void copy(const CArray<TYPE>& source)
	{
		resizeTo(source.size);
		for (size_t i = 0; i < source.count; ++i)
			list[i] = source.list[i];
		count = source.count;
	}

	void grabContent(CArray<TYPE>& source)
	{
		if (list)
			delete[] list;
		list = source.list;
		size = source.size;
		count = source.count;
		source.list = nullptr;
		source.size = 0;
		source.count = 0;
	}

	void set(size_t num, TYPE value)
	{
		if (num >= count)
		{
			resize(num+1);
			count = num+1;
		}
		list[num] = value;
	}

	TYPE* get(size_t num)
	{
		if (num < 0 || num >= count)
			return 0;
		return &list[num];
	}

	const TYPE* get(size_t num) const
	{
		if (num < 0 || num >= count)
			return 0;
		return &list[num];
	}

	inline TYPE& front()			 { return list[0]; }
	inline const TYPE& front() const { return list[0]; }

	inline TYPE& back()				 { return list[count-1]; }
	inline const TYPE& back() const	 { return list[count-1]; }

	inline void push_back(const TYPE& value)  { add(value); }

	inline TYPE& popBack()			 { --count; return list[count]; }
	inline TYPE& pop_back()			 { --count; return list[count]; }

	inline bool empty() const		 { return count == 0; }

	TYPE* add()
	{
		resize(count+1);
		++count;
		return &list[count-1];
	}

	void add(const TYPE& value)
	{
		resize(count+1);
		list[count] = value;
		++count;
	}

	void add(const TYPE* ptr, size_t cnt)
	{
		if (!ptr)
			return;
		resize(count+cnt);
		for (size_t i = 0; i < cnt; ++i)
			list[count+i] = ptr[i];
		count += cnt;
	}

	int find(const TYPE& elem)
	{
		for (size_t i = 0; i < count; ++i)
			if (list[i] == elem)
				return i;
		return -1;
	}

	inline TYPE* operator*()			 { return list; }
	inline const TYPE* operator*() const { return list; }

	inline TYPE& operator[](size_t index)			  { return list[index]; }
	inline const TYPE& operator[](size_t index) const { return list[index]; }
};
