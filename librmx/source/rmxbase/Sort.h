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

	// generischer Quicksort
	template<typename TYPE> void quicksort(TYPE* list, int count, int(*compare)(TYPE,TYPE))
	{
		if (count < 2)
			return;
		int last = count-1;
		TYPE median = list[last];
		int left = 0;
		int right = last-1;
		while (left <= right)
		{
			while (left <= right && compare(list[left], median) <= 0)
				++left;
			while (left <= right && compare(median, list[right]) <= 0)
				--right;
			if (left > right)
				break;
			TYPE tmp = list[left];
			list[left] = list[right];
			list[right] = tmp;
		}
		list[last] = list[left];
		list[left] = median;
		quicksort<TYPE>(list, left, compare);
		quicksort<TYPE>(&list[left+1], last-left, compare);
	}

	template<typename TYPE> void quicksort(TYPE* list, int count, int(*compare)(const TYPE&, const TYPE&))
	{
		if (count < 2)
			return;
		int last = count-1;
		TYPE median = list[last];
		int left = 0;
		int right = last-1;
		while (left <= right)
		{
			while (left <= right && compare(list[left], median) <= 0)
				++left;
			while (left <= right && compare(median, list[right]) <= 0)
				--right;
			if (left > right)
				break;
			TYPE tmp = list[left];
			list[left] = list[right];
			list[right] = tmp;
		}
		list[last] = list[left];
		list[left] = median;
		quicksort<TYPE>(list, left, compare);
		quicksort<TYPE>(&list[left+1], last-left, compare);
	}

}
