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
	static void cleanMemory(std::vector<T>& vec)
	{
		std::vector<T>{}.swap(vec);
	}
	template<typename K, typename V>
	static void cleanMemory(std::map<K, V>& vec)
	{
		std::map<K, V>{}.swap(vec);
	}

	template<typename T, typename F>
	static void removeIf(std::vector<T>& vec, int numberToRemove, F& func)
	{
		if (numberToRemove <= 0)
		{
			return;
		}
		for (auto it = vec.begin(); it != vec.end(); )
		{
			auto value = *it;
			if (func(value))
			{
				if (value != *it)
				{
					throw Exception("removeIf error, someone else modified collection");
				}
				if (--numberToRemove == 0)
				{
					return;
				}
				it = vec.erase(it);
			}
			else
			{
				++it;
			}
		}
	}
};

}
