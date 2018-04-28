#pragma once
/*
 * Copyright 2010-2016 OpenXcom Developers.
 *
 * This file is part of OpenXcom.
 *
 * OpenXcom is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * OpenXcom is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenXcom.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <vector>
#include <map>
#include <list>
#include "Exception.h"

namespace OpenXcom
{

/**
 * Helper class for managing object colections
 */
class Collections
{
public:
	template<typename T>
	static void deleteAll(const T& p)
	{
		//nothing
	}
	template<typename T>
	static void deleteAll(T* p)
	{
		delete p;
	}
	template<typename T>
	static void deleteAll(std::vector<T>& vec)
	{
		for (auto p : vec)
		{
			deleteAll(p);
		}
		removeAll(vec);
	}
	template<typename T>
	static void deleteAll(std::list<T>& list)
	{
		for (auto p : list)
		{
			deleteAll(p);
		}
		removeAll(list);
	}
	template<typename K, typename V>
	static void deleteAll(std::map<K, V>& map)
	{
		for (auto p : map)
		{
			deleteAll(p.second);
		}
		removeAll(map);
	}

	/**
	 * Remove and delete (if pointer) items from colection with limit.
	 * @param colection Ccolection from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left to remove
	 */
	template<typename C, typename F>
	static int deleteIf(C& colection, int numberToRemove, F&& func)
	{
		return removeIf(colection, numberToRemove,
			[&](typename C::reference t)
			{
				if (func(t))
				{
					deleteAll(t);
					return true;
				}
				else
				{
					return false;
				}
			}
		);
	}

	/**
	 * Clear vector. It set capacity to zero too.
	 * @param vec
	 */
	template<typename T>
	static void removeAll(std::vector<T>& vec)
	{
		std::vector<T>{}.swap(vec);
	}

	/**
	 * Clear container.
	 * @param colection
	 */
	template<typename C>
	static void removeAll(C& colection)
	{
		colection.clear();
	}

	/**
	 * Remove items from vector with limit.
	 * @param vec Vector from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this vector
	 * @return Number of values left to remove
	 */
	template<typename T, typename F>
	static int removeIf(std::vector<T>& vec, int numberToRemove, F&& func)
	{
		if (numberToRemove <= 0)
		{
			return 0;
		}
		auto begin = vec.begin();
		auto newEnd = vec.begin();
		//similar to `std::remove_if` but it do not allow modyfy anything in `func`
		for (auto it = begin; it != vec.end(); ++it)
		{
			auto& value = *it;
			if (numberToRemove > 0 && func(value))
			{
				--numberToRemove;
			}
			else
			{
				*newEnd = std::move(value);
				++newEnd;
			}
		}
		vec.erase(newEnd, vec.end());
		return numberToRemove;
	}

	/**
	 * Remove items from colection with limit.
	 * @param list List from witch remove items
	 * @param numberToRemove Limit of removal
	 * @param func Test what should be removed, can modyfy everyting except this colection
	 * @return Number of values left to remove
	 */
	template<typename C, typename F>
	static int removeIf(C& colection, int numberToRemove, F&& func)
	{
		if (numberToRemove <= 0)
		{
			return 0;
		}
		for (auto it = colection.begin(); it != colection.end(); )
		{
			auto& value = *it;
			if (func(value))
			{
				it = colection.erase(it);
				if (--numberToRemove == 0)
				{
					return 0;
				}
			}
			else
			{
				++it;
			}
		}
		return numberToRemove;
	}
};

}
