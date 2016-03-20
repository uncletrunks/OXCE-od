/*
 * Copyright 2010-2015 OpenXcom Developers.
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
#ifndef OPENXCOM_SCRIPTBIND_H
#define	OPENXCOM_SCRIPTBIND_H

#include "Script.h"
#include "Exception.h"
#include "Logger.h"

namespace OpenXcom
{

/**
 * Type of code block
 */
enum BlockEnum
{
	BlockIf,
	BlockElse,
	BlockWhile,
	BlockLoop,
};

/**
 * Struct represents position of token in input string
 */
class SelectedToken : public ScriptRef
{
	/// type of this token.
	TokenEnum _type;
	/// custom value.
	int _tag;

public:

	/// Default constructor.
	SelectedToken() : ScriptRef{ }, _type{ TokenNone }, _tag{ }
	{

	}

	/// Constructor from range.
	SelectedToken(TokenEnum type, ScriptRef range) : ScriptRef{ range }, _type{ type }, _tag{ }
	{

	}

	/// Constructor from tag.
	SelectedToken(TokenEnum type, int tag) : ScriptRef{ }, _type{ type }, _tag{ tag }
	{

	}

	TokenEnum getType() const
	{
		return _type;
	}
	int getTag() const
	{
		return _tag;
	}
	void setTag(int i)
	{
		_tag = i;
	}
};

/**
 * Helper structure used by ScriptParser::parse
 */
struct ParserWriter
{
	struct Block
	{
		BlockEnum type;
		int nextLabel;
		int finalLabel;
	};

	template<typename T, typename = typename std::enable_if<std::is_pod<T>::value>::type>
	class ReservedPos
	{
		ProgPos _pos;
		ReservedPos(ProgPos pos) : _pos{ pos }
		{

		}
		ProgPos getPos()
		{
			return _pos;
		}

		friend class ParserWriter;
	};

	class ProcOp { };

	/// member pointer accessing script operations.
	ScriptContainerBase& container;
	/// all available data for script.
	const ScriptParserBase& parser;
	/// temporary script data.
	std::vector<ScriptContainerData> refListCurr;
	/// list of labels places of usage.
	std::vector<std::pair<ReservedPos<ProgPos>, int>> refLabelsUses;
	/// list of labels positions.
	std::vector<ProgPos> refLabelsList;

	/// index of used script registers.
	Uint8 regIndexUsed;
	/// negative index of used const values.
	int constIndexUsed;

	/// Store position of blocks of code like "if" or "while".
	std::vector<Block> codeBlocks;

	/// Constructor.
	ParserWriter(
			Uint8 regUsed,
			ScriptContainerBase& c,
			const ScriptParserBase& d);

	/// Finall fixes of data.
	void relese();

	/// Get referece based on name.
	ScriptContainerData getReferece(const ScriptRef& s) const;

	/// Add referece based.
	ScriptContainerData addReferece(const ScriptContainerData& data);

	/// Get current position in proc vector.
	ProgPos getCurrPos() const;

	/// Get distance betwean two positions in proc vector.
	size_t getDiffPos(ProgPos begin, ProgPos end) const;


	/**
	 * Preparing place and position on proc vector for some value and return position of it.
	 */
	template<typename T>
	ReservedPos<T> pushReserved()
	{
		auto curr = getCurrPos();
		container._proc.insert(container._proc.end(), sizeof(T), 0);
		return { curr };
	}

	/**
	 * Setting previosly prepared place with value.
	 */
	template<typename T>
	void updateReserved(ReservedPos<T> pos, T value)
	{
		memcpy(&container._proc[static_cast<size_t>(pos.getPos())], &value, sizeof(T));
	}

	/**
	 * Push value on proc vector.
	 * @param v Value to push.
	 */
	template<typename T>
	void pushValue(const T& v)
	{
		updateReserved<T>(pushReserved<T>(), v);
	}


	/// Push zeros to fill empty space.
	void pushPadding(size_t s);

	/// Pushing proc operation id on proc vector.
	ReservedPos<ProcOp> pushProc(Uint8 procId);

	/// Updating previosoly added proc operation id.
	void updateProc(ReservedPos<ProcOp> pos, int procOffset);

	/// Try pushing label arg on proc vector. Can't use this to create loop back label.
	bool pushLabelTry(const ScriptRef& s);

	/// Push label arg to proc vector.
	void pushLabel(int index);
	/// Create new label for proc vector.
	int addLabel();

	/// Setting offset of label on proc vector.
	bool setLabel(const ScriptRef& s, ProgPos offset);

	/// Setting offset of label on proc vector.
	bool setLabel(int index, ProgPos offset);

	/// Try pushing data arg on proc vector.
	bool pushDataTry(const ScriptRef& s);

	/// Try pushing const arg on proc vector.
	bool pushConstTry(const ScriptRef& s);

	/// Try pushing reg arg on proc vector.
	template<typename T>
	bool pushRegTry(const ScriptRef& s)
	{
		return pushRegTry(s, ScriptParserBase::registerType<T>());
	}

	/// Try pushing reg arg on proc vector.
	bool pushRegTry(const ScriptRef& s, ArgEnum type);

	/// Add new reg arg.
	template<typename T>
	bool addReg(const ScriptRef& s)
	{
		return addReg(s, ScriptParserBase::registerType<T>());
	}

	/// Add new reg arg.
	bool addReg(const ScriptRef& s, ArgEnum type);
}; //struct ParserWriter

////////////////////////////////////////////////////////////
//				Mata helper classes
////////////////////////////////////////////////////////////

template<int I>
struct PosTag
{
	static constexpr int value = I;
};

template<int... I>
struct ListTag
{
	static constexpr int size = sizeof...(I);
};

template<typename PT>
struct IncListTag;

template<int... I>
struct IncListTag<ListTag<I...>>
{
	using type = ListTag<I..., sizeof...(I)>;
};

template<int I>
struct PosTagToList
{
	using type = typename IncListTag<typename PosTagToList<I - 1>::type>::type;
};

template<>
struct PosTagToList<0>
{
	using type = ListTag<>;
};

template<int I>
using GetListTag = typename PosTagToList<I>::type;

template<typename T, int P>
using GetType = decltype(T::typeFunc(PosTag<P>{}));


template<int MaxSize, typename... T>
struct SumListIndexImpl;

template<int MaxSize, typename T1, typename... T>
struct SumListIndexImpl<MaxSize, T1, T...> : SumListIndexImpl<MaxSize, T...>
{
	using tag = PosTag<MaxSize - (1 + sizeof...(T))>;

	using SumListIndexImpl<MaxSize, T...>::typeFunc;
	static T1 typeFunc(tag);

	static constexpr FuncCommon getDynamic(int i)
	{
		return tag::value == i ? T1::func : SumListIndexImpl<MaxSize, T...>::getDynamic(i);
	}
};

template<int MaxSize>
struct SumListIndexImpl<MaxSize>
{
	struct End
	{
		static constexpr int offset = 0;

		[[gnu::always_inline]]
		static RetEnum func(ScriptWorker &, const Uint8 *, ProgPos &)
		{
			return RetError;
		}
	};
	static End typeFunc(...);
	static constexpr FuncCommon getDynamic(int i)
	{
		return End::func;
	}
};

template<typename... V>
struct SumList : SumListIndexImpl<sizeof...(V), V...>
{
	static constexpr size_t size = sizeof...(V);

	using SumListIndexImpl<sizeof...(V), V...>::typeFunc;
	using SumListIndexImpl<sizeof...(V), V...>::getDynamic;

	template<typename... F2>
	constexpr SumList<V..., F2...> operator+(SumList<F2...>) const { return {}; }
	constexpr SumList<V...> operator+() const { return {}; }
};

////////////////////////////////////////////////////////////
//						Arg class
////////////////////////////////////////////////////////////

template<bool Internal, typename... A>
struct Arg;

template<bool Internal, typename A1, typename... A2>
struct Arg<Internal, A1, A2...> : public Arg<Internal, A2...>
{
	static constexpr bool internal = Internal;

	using next = Arg<Internal, A2...>;
	using tag = PosTag<sizeof...(A2)>;

	static constexpr int offset(int i)
	{
		return i == tag::value ? A1::size : next::offset(i);
	}
	static constexpr int ver()
	{
		return tag::value + 1;
	}

	using next::typeFunc;
	static A1 typeFunc(tag);

	static int parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (A1::parse(ph, t))
		{
			return tag::value;
		}
		else
		{
			return next::parse(ph, t);
		}
	}
	static ArgEnum firstArgType()
	{
		return tag::value != 0 ? ArgInvalid : A1::type();
	}
};

template<bool Internal>
struct Arg<Internal>
{
	static constexpr bool internal = Internal;

	static constexpr int offset(int i)
	{
		return 0;
	}
	static constexpr int ver() = delete;
	static void typeFunc() = delete;

	static int parse(ParserWriter& ph, const SelectedToken& begin)
	{
		return -1;
	}
	static ArgEnum firstArgType()
	{
		return ArgInvalid;
	}
};

////////////////////////////////////////////////////////////
//					ArgColection class
////////////////////////////////////////////////////////////

template<int MaxSize, typename... T>
struct ArgColection;

template<int MaxSize, typename T1, typename... T2>
struct ArgColection<MaxSize, T1, T2...> : public ArgColection<MaxSize, T2...>
{
	using next = ArgColection<MaxSize, T2...>;
	using tag = PosTag<MaxSize - (sizeof...(T2) + 1)>;

	static constexpr int pos(int ver, int pos)
	{
		return (pos == tag::value ? ver % T1::ver() : next::pos(ver / T1::ver(), pos));
	}
	static constexpr int offset(int ver, int pos)
	{
		return (pos > tag::value ? T1::offset(ver % T1::ver()) : 0) + next::offset(ver / T1::ver(), pos);
	}
	static constexpr int ver()
	{
		return T1::ver() * next::ver();
	}
	static constexpr int arg()
	{
		return MaxSize;
	}
	static int parse(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
	{
		int curr = 0;
		if (!T1::internal)
		{
			if (begin == end)
			{
				Log(LOG_ERROR) << "Not enough args in operation";
				return -1;
			}
			else
			{
				curr = T1::parse(ph, *begin);
				if (curr < 0)
				{
					Log(LOG_ERROR) << "Incorrect argument '"<< begin->toString() <<"'";
					return -1;
				}
			}
			++begin;
		}
		int lower = next::parse(ph, begin, end);
		if (lower < 0)
		{
			return -1;
		}
		return lower * T1::ver() + curr;
	}
	static ArgEnum firstArgType()
	{
		return !T1::internal ? T1::firstArgType() : next::firstArgType();
	}

	using next::typeFunc;
	static T1 typeFunc(tag);
};

template<int MaxSize>
struct ArgColection<MaxSize>
{
	using tag = PosTag<-1>;

	static constexpr int pos(int ver, int pos)
	{
		return 0;
	}
	static constexpr int offset(int ver, int pos)
	{
		return 0;
	}
	static constexpr int ver()
	{
		return 1;
	}
	static constexpr int arg()
	{
		return 0;
	}
	static int parse(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
	{
		//we shoud have used all avaiable tokens.
		if (begin == end)
		{
			return 0;
		}
		else
		{
			Log(LOG_ERROR) << "Too many args in operation";
			return -1;
		}
	}
	static ArgEnum firstArgType()
	{
		return ArgInvalid;
	}
	static void typeFunc() = delete;
};

////////////////////////////////////////////////////////////
//						Arguments impl
////////////////////////////////////////////////////////////

struct ArgContextDef
{
	using ReturnType = ScriptWorker&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw;
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

struct ArgProgDef
{
	using ReturnType = ProgPos&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return curr;
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<typename T>
struct ArgRegDef
{
	using ReturnType = T&;
	static constexpr size_t size = sizeof(Uint8);
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.ref<ReturnType>(*arg);
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (t.getType() == TokenSymbol)
		{
			return ph.pushRegTry<ReturnType>(t);
		}
		return false;
	}

	static ArgEnum type()
	{
		return ScriptParserBase::registerType<ReturnType>();
	}
};

struct ArgConstDef
{
	using ReturnType = int;
	static constexpr size_t size = sizeof(int);
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<int>(arg);
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (t.getType() == TokenNumber)
		{
			return ph.pushConstTry(t);
		}
		else if (t.getType() == TokenSymbol)
		{
			return ph.pushDataTry(t);
		}
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

struct ArgLabelDef
{
	using ReturnType = ProgPos;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ProgPos>(arg);
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (t.getType() == TokenSymbol)
		{
			return ph.pushLabelTry(t);
		}
		else if (t.getType() == TokenBuildinLabel)
		{
			ph.pushLabel(t.getTag());
			return true;
		}
		return false;
	}

	static ArgEnum type()
	{
		return ArgLabel;
	}
};

struct ArgFuncDef
{
	using ReturnType = FuncCommon;
	static constexpr size_t size = sizeof(ReturnType);
	static FuncCommon get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<FuncCommon>(arg);
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

template<int Size>
struct ArgRawDef
{
	using ReturnType = const Uint8*;
	static constexpr size_t size = Size;
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return arg;
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

struct ArgNullDef
{
	using ReturnType = nullptr_t;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return nullptr;
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (t.getType() == TokenSymbol)
		{
			return t == ScriptRef{ "null" };
		}
		return false;
	}

	static ArgEnum type()
	{
		return ArgNull;
	}
};

template<typename T, typename I>
struct ArgTagDef
{
	using ReturnType = ScriptTag<T, I>;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorker& sw, const Uint8* arg, ProgPos& curr)
	{
		return sw.const_val<ScriptTag<T, I>>(arg);
	}

	static bool parse(ParserWriter& ph, const SelectedToken& t)
	{
		if (t.getType() == TokenSymbol)
		{
			auto tag = ScriptValues<T, I>::getTag(t.toString());
			if (tag)
			{
				ph.pushValue(tag);
				return true;
			}
		}
		return false;
	}

	static ArgEnum type()
	{
		return ArgInvalid;
	}
};

////////////////////////////////////////////////////////////
//					ArgSelector class
////////////////////////////////////////////////////////////

template<>
struct ArgSelector<ScriptWorker&>
{
	using type = Arg<true, ArgContextDef>;
};

template<>
struct ArgSelector<int&>
{
	using type = Arg<false, ArgRegDef<int>>;
};

template<>
struct ArgSelector<int>
{
	using type = Arg<false, ArgConstDef, ArgRegDef<int>>;
};

template<>
struct ArgSelector<const int&>
{
	using type = Arg<false, ArgConstDef, ArgRegDef<int>>;
};

template<>
struct ArgSelector<ProgPos&>
{
	using type = Arg<true, ArgProgDef>;
};

template<>
struct ArgSelector<ProgPos>
{
	using type = Arg<false, ArgLabelDef>;
};

template<>
struct ArgSelector<FuncCommon>
{
	using type = Arg<false, ArgFuncDef>;
};

template<>
struct ArgSelector<const Uint8*>
{
	using type = Arg<true, ArgRawDef<64>, ArgRawDef<32>, ArgRawDef<16>, ArgRawDef<8>, ArgRawDef<4>, ArgRawDef<0>>;
};

template<typename T>
struct ArgSelector<T*>
{
	using type = Arg<false, ArgRegDef<T*>, ArgNullDef>;
};

template<typename T>
struct ArgSelector<T*&>
{
	using type = Arg<false, ArgRegDef<T*>>;
};

template<typename T>
struct ArgSelector<const T*>
{
	using type = Arg<false, ArgRegDef<const T*>, ArgRegDef<T*>, ArgNullDef>;
};

template<typename T>
struct ArgSelector<const T*&>
{
	using type = Arg<false, ArgRegDef<const T*>>;
};

template<typename T, typename I>
struct ArgSelector<ScriptTag<T, I>>
{
	using type = Arg<false, ArgTagDef<T, I>>;
};

template<typename T>
struct GetArgsImpl;

template<typename... Args>
struct GetArgsImpl<RetEnum(Args...)>
{
	using type = ArgColection<sizeof...(Args), typename ArgSelector<Args>::type...>;
};

template<typename Func>
using GetArgs = typename GetArgsImpl<decltype(Func::func)>::type;

////////////////////////////////////////////////////////////
//				FuncVer and FuncGroup class
////////////////////////////////////////////////////////////

template<typename Func, int Ver, typename PosList = GetListTag<GetArgs<Func>::arg()>>
struct FuncVer;

template<typename Func, int Ver, int... Pos>
struct FuncVer<Func, Ver, ListTag<Pos...>>
{
	using Args = GetArgs<Func>;
	static constexpr int offset = Args::offset(Ver, Args::arg());

	template<int CurrPos>
	using GetTypeAt = GetType<GetType<Args, CurrPos>, Args::pos(Ver, CurrPos)>;

	template<int CurrPos>
	[[gnu::always_inline]]
	static typename GetTypeAt<CurrPos>::ReturnType get(ScriptWorker& sw, const Uint8* procArgs, ProgPos& curr)
	{
		constexpr int offset = Args::offset(Ver, CurrPos);
		return GetTypeAt<CurrPos>::get(sw, procArgs + offset, curr);
	}

	[[gnu::always_inline]]
	static RetEnum func(ScriptWorker& sw, const Uint8* procArgs, ProgPos& curr)
	{
		return Func::func(get<Pos>(sw, procArgs, curr)...);
	}
};

template<typename Func, typename VerList = GetListTag<GetArgs<Func>::ver()>>
struct FuncGroup;

template<typename Func, int... Ver>
struct FuncGroup<Func, ListTag<Ver...>> : GetArgs<Func>
{
	using FuncList = SumList<FuncVer<Func, Ver>...>;
	using GetArgs<Func>::ver;
	using GetArgs<Func>::arg;
	using GetArgs<Func>::parse;
	using GetArgs<Func>::firstArgType;

	static constexpr FuncCommon getDynamic(int i) { return FuncList::getDynamic(i); }
};

////////////////////////////////////////////////////////////
//						Bind class
////////////////////////////////////////////////////////////

namespace helper
{

template<typename T>
struct BindSet
{
	static RetEnum func(T*& t, T* r)
	{
		t = r;
		return RetContinue;
	}
};
template<typename T>
struct BindEq
{
	static RetEnum func(ProgPos& Prog, const T* t1, const T* t2, ProgPos LabelTrue, ProgPos LabelFalse)
	{
		Prog = (t1 == t2) ? LabelTrue : LabelFalse;
		return RetContinue;
	}
};
template<typename T, typename P, P T::*X>
struct BindPropGet
{
	static RetEnum func(const T* t, P& p)
	{
		if (t) p = t->*X; else p = P{};
		return RetContinue;
	}
};
template<typename T, typename P, P T::*X>
struct BindPropSet
{
	static RetEnum func(T* t, P p)
	{
		if (t) t->*X = p;
		return RetContinue;
	}
};
template<typename T, ScriptValues<T> T::*X>
struct BindScriptValueGet
{
	static RetEnum func(const T* t, int& p, typename ScriptValues<T>::Tag st)
	{
		if (t) p = (t->*X).get(st); else p = int{};
		return RetContinue;
	}
};
template<typename T, ScriptValues<T> T::*X>
struct BindScriptValueSet
{
	static RetEnum func(T* t, typename ScriptValues<T>::Tag st, int p)
	{
		if (t) (t->*X).set(st, p);
		return RetContinue;
	}
};

template<typename T, typename N, typename P, N T::*X, P N::*XX>
struct BindPropNestGet
{
	static RetEnum func(const T* t, P& p)
	{
		if (t) p = (t->*X).*XX; else p = P{};
		return RetContinue;
	}
};
template<typename T, typename N, typename P, N T::*X, P N::*XX>
struct BindPropNestSet
{
	static RetEnum func(T* t, P p)
	{
		if (t) (t->*X).*XX = p;
		return RetContinue;
	}
};

template<typename T, typename P, P I>
struct BindValue
{
	static RetEnum func(T* t, P& p)
	{
		if (t) p = I; else p = P{};
		return RetContinue;
	}
};

template<typename T, T f>
struct BindFunc;

template<typename... Args, void(*X)(Args...)>
struct BindFunc<void(*)(Args...), X>
{
	static RetEnum func(Args... a)
	{
		X(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...)>
struct BindFunc<bool(T::*)(Args...), X>
{
	static RetEnum func(T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, int(T::*X)(Args...)>
struct BindFunc<int(T::*)(Args...), X>
{
	static RetEnum func(T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...) const>
struct BindFunc<bool(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, int(T::*X)(Args...) const>
struct BindFunc<int(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, int& r, Args... a)
	{
		if (t) r = (t->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, void(T::*X)(Args...)>
struct BindFunc<void(T::*)(Args...), X>
{
	static RetEnum func(T* t, Args... a)
	{
		if (t) (t->*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P *(T::*X)(Args...)>
struct BindFunc<P *(T::*)(Args...), X>
{
	static RetEnum func(T* t, P*& p, Args... a)
	{
		if (t) p = (t->*X)(std::forward<Args>(a)...); else p = nullptr;
		return RetContinue;
	}
};

template<typename T, typename P, typename... Args, P *(T::*X)(Args...) const>
struct BindFunc<P *(T::*)(Args...) const, X>
{
	static RetEnum func(const T* t, P*& p, Args... a)
	{
		if (t) p = (t->*X)(std::forward<Args>(a)...); else p = nullptr;
		return RetContinue;
	}
};

} //namespace helper

struct BindBase
{
	ScriptParserBase* parser;
	BindBase(ScriptParserBase* p) : parser{ p }
	{

	}

	template<typename X>
	void addCustomFunc(const std::string& name)
	{
		parser->addParser<FuncGroup<X>>(name);
	}
	void addCustomConst(const std::string& name, int i)
	{
		parser->addConst(name, i);
	}
};

template<typename T>
struct Bind : BindBase
{

	Bind(ScriptParserBase* p) : BindBase{ p }
	{
		parser->addParser<FuncGroup<helper::BindSet<T>>>(getName("setRef"));
		parser->addParser<FuncGroup<helper::BindSet<const T>>>(getName("setRef"));
		parser->addParser<FuncGroup<helper::BindEq<T>>>(getName("eq"));
	}

	std::string getName(const std::string& s)
	{
		return std::string{ T::ScriptName } + "." + s;
	}

	template<typename X>
	void addFunc(const std::string& name)
	{
		addCustomFunc<X>(getName(name));
	}
	template<int T::*X>
	void addField(const std::string& get, const std::string& set = "")
	{
		addCustomFunc<helper::BindPropGet<T, int, X>>(getName(get));
		if (!set.empty())
		{
			addCustomFunc<helper::BindPropSet<T, int, X>>(getName(set));
		}
	}
	template<ScriptValues<T> T::*X>
	void addScriptValue()
	{
		addCustomFunc<helper::BindScriptValueGet<T, X>>(getName("getCustom"));
		addCustomFunc<helper::BindScriptValueSet<T, X>>(getName("setCustom"));
	}
	template<int X>
	void addFake(const std::string& get)
	{
		addCustomFunc<helper::BindValue<T, int, X>>(getName(get));
	}

	#define MACRO_COPY_HELP_FUNC(Name, ...) \
		template<__VA_ARGS__> \
		void add(const std::string& func) \
		{ \
			addCustomFunc<helper::BindFunc<decltype(Name), Name>>(getName(func));\
		}

	MACRO_COPY_HELP_FUNC(X, bool (T::*X)() const)

	MACRO_COPY_HELP_FUNC(X, int (T::*X)() const)
	MACRO_COPY_HELP_FUNC(X, int (T::*X)())
	MACRO_COPY_HELP_FUNC(X, int (T::*X)(int) const)

	MACRO_COPY_HELP_FUNC(X, void (T::*X)(int))
	MACRO_COPY_HELP_FUNC(X, void (*X)(T*, int))
	MACRO_COPY_HELP_FUNC(X, void (*X)(T*, int&))
	MACRO_COPY_HELP_FUNC(X, void (*X)(T*, int&, int))

	MACRO_COPY_HELP_FUNC(X, void (*X)(const T*, int))
	MACRO_COPY_HELP_FUNC(X, void (*X)(const T*, int&))
	MACRO_COPY_HELP_FUNC(X, void (*X)(const T*, int&, int))

	MACRO_COPY_HELP_FUNC(X, typename P, P* (T::*X)())
	MACRO_COPY_HELP_FUNC(X, typename P, P* (T::*X)() const)

	#undef MACRO_COPY_HELP_FUNC
};

template<typename T, typename N, N T::*X>
struct BindNested
{
	Bind<T> *bind;
	BindNested(Bind<T>& b) : bind{ &b }
	{

	}

	template<int N::*XX>
	void addField(const std::string& get, const std::string& set = "")
	{
		bind->template addCustomFunc<helper::BindPropNestGet<T, N, int, X, XX>>(bind->getName(get));
		if (!set.empty())
		{
			bind->template addCustomFunc<helper::BindPropNestSet<T, N, int, X, XX>>(bind->getName(get));
		}
	}
};

} //namespace OpenXcom

#endif	/* OPENXCOM_SCRIPTBIND_H */

