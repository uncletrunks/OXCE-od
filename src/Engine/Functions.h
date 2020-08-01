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
#include <functional>
#include <assert.h>



namespace OpenXcom
{

/**
 * Low overhead callback.
 *
 * Base template, not defined any where.
 */
template<typename T>
class FuncRef;

/**
 * Low overhead callback.
 */
template<typename Ret, typename... Args>
class FuncRef<Ret(Args...)>
{
public:
	using ApiFunc = Ret(*)(Args...);
	using InternalFunc = Ret(*)(Args..., void*);

	/**
	 * Constructor that take refrerecne to some temporal callable object.
	 *
	 * To prevent possibility of unintended dangling reference you need spell full type.
	 */
	template<typename F>
	explicit FuncRef(F&& f) :FuncRef{ +[](Args... args, void* p){ return std::invoke(*static_cast<F*>(p), std::forward<Args>(args)...); }, &f }
	{

	}

	/**
	 * Construcotr that take pointer to some callabe object.
	 */
	template<typename P>
	constexpr FuncRef(P* f) : FuncRef{ +[](Args... args, void* p){ return std::invoke(*reinterpret_cast<P*>(p), std::forward<Args>(args)...); }, reinterpret_cast<void*>(f) }
	{
		static_assert(sizeof(f) == sizeof(_data), "This code work only if void pointer have same size as given pointer (in special case function pointer)");
	}

	/**
	 * Invalid construcotr from empty pointer.
	 */
	FuncRef(std::nullptr_t) = delete;

	/**
	 * Helper construcotr for minimum overhead functions.
	 * @param func Internal function to call.
	 * @param data State that is pass to interall function call.
	 */
	constexpr FuncRef(InternalFunc func, void* data) : _func{ func }, _data{ data }
	{

	}

	/**
	 * Call operator.
	 */
	constexpr Ret operator()(Args... args) const
	{
		return _func(std::forward<Args>(args)..., _data);
	}

	/**
	 * bool operator
	 * @return always true as it can't be empty.
	 */
	constexpr explicit operator bool()
	{
		return true;
	}

private:

	InternalFunc _func;
	void* _data;
};


} //namespace OpenXcom
