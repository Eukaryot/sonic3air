/*
*	rmx Library
*	Copyright (C) 2008-2024 by Eukaryot
*
*	Published under the GNU GPLv3 open source software license, see license.txt
*	or https://www.gnu.org/licenses/gpl-3.0.en.html
*/

#pragma once

#include <vector>
#include <map>
#include <unordered_map>


// Check if an std::vector, std::list, etc. contains a certain element
template<typename T, typename E>
bool containsElement(const T& container, E element)
{
	return (std::find(container.begin(), container.end(), element) != container.end());
}

// Check if an std::vector, std::list, etc. contains an element matching the predicate
template<typename T, class PRED>
bool containsByPredicate(const T& container, PRED predicate)
{
	return (std::find_if(container.begin(), container.end(), predicate) != container.end());
}


// Add a new empty element to an std::vector and return a reference
template<typename T>
T& vectorAdd(std::vector<T>& vec)
{
	vec.emplace_back();
	return vec.back();
}

// Remove element from an std::vector by swapping with the last one
template<typename T>
bool vectorRemoveSwap(std::vector<T>& vec, size_t index)
{
	if (index + 1 < vec.size())
		std::swap(vec[index], vec.back());
	else if (index >= vec.size())
		return false;
	vec.pop_back();
	return true;
}

// Check if an std::vector contains a certain element (actually only an alias for "containsElement")
template<typename T>
bool vectorContains(const std::vector<T>& vec, T element)
{
	return containsElement(vec, element);
}

// Get the index of a certain element in an std::vector, or -1 if not found
template<typename T>
int vectorIndexOf(const std::vector<T>& vec, T element)
{
	for (size_t index = 0; index < vec.size(); ++index)
	{
		if (vec[index] == element)
			return (int)index;
	}
	return -1;
}

// Find an element matching the predicate in an std::vector
template<typename T, class PRED>
T* vectorFindByPredicate(std::vector<T>& vec, PRED predicate)
{
	const auto it = std::find_if(vec.begin(), vec.end(), predicate);
	return (it == vec.end()) ? nullptr : &*it;
}

template<typename T, class PRED>
const T* vectorFindByPredicate(const std::vector<T>& vec, PRED predicate)
{
	const auto it = std::find_if(vec.begin(), vec.end(), predicate);
	return (it == vec.end()) ? nullptr : &*it;
}

// Remove all instances of an element from an std::vector
template<typename T, typename S>
void vectorRemoveAll(std::vector<T>& container, S value)
{
	container.erase(std::remove(container.begin(), container.end(), value), container.end());
}

// Remove all elements matching the predicate from an std::vector
template<typename T, class PRED>
void vectorRemoveByPredicate(std::vector<T>& container, PRED predicate)
{
	container.erase(std::remove_if(container.begin(), container.end(), predicate), container.end());
}


// Find an element in an std::map
template<typename K, typename V>
V* mapFind(std::map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
const V* mapFind(const std::map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
V* mapFind(std::unordered_map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}

template<typename K, typename V>
const V* mapFind(const std::unordered_map<K, V>& map, K key)
{
	const auto it = map.find(key);
	return (it == map.end()) ? nullptr : &it->second;
}
