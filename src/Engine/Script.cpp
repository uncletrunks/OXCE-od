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

#include <sstream>
#include <iomanip>

#include "Logger.h"
#include "Options.h"
#include "Script.h"
#include "Surface.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"

/**
 * Script (`ScriptBase`) is build of two parts: proc (`_proc`) and ref (`_procRefData`).
 *
 * proc: vector of bytes describing script operations and its arguments
 *
 * [op0][arg][arg] [op1] [op2][arg] [op3] ...
 *
 * every operation have varied numbers of arguments.
 * every argument is simple index in ref table, but script engine try keep
 * separate every possible typ of arg: label, data, reg and const.
 * all possible operations are stored in macro `MACRO_PROC_DEFINITION` with its definitions.
 *
 * ref: table with definition of all possible arguments in script and functions that extract data form object.
 * from ref is created array that script work on (`ScriptWorkRef`).
 * ref have now fixed (`ScriptMaxRef`) size that can be problematic if people will create more complex scripts.
 * hard limit is 255, after that proc type need be chanced to bigger int.
 *
 * Script have 4 different arguments types.
 *
 * label: position in proc table, read only.
 * data: some data read form object, read only.
 * const: const int value written in source code of script like "0", "42" or "0x13", read only, can be used in place of data arg.
 * reg: register that store values that script work on, write/read, can be used in place of data arg.
 *
 * Internal arguments, not accessible by end user.
 *
 * none: "empty" argument.
 * prog: current position of script execution.
 * test: variable that is used for conditions (alias of one of reg).
 * result: final value of script execution (alias of one of reg).
 */

namespace OpenXcom
{

namespace
{

using FuncCommon = bool (*)(ScriptWorker &, const Uint8 *, int &);
using FuncOffset = int (*)(int);

////////////////////////////////////////////////////////////
//						arg definition
////////////////////////////////////////////////////////////
#define MACRO_QUOTE(...) __VA_ARGS__

#define MACRO_ARG_None(i)
#define MACRO_ARG_Prog(i)	, int& Prog
#define MACRO_ARG_Test(i)	, int& Test
#define MACRO_ARG_Result(i)	, int& Result

#define MACRO_ARG_Reg(i)	, int& Reg##i
#define MACRO_ARG_Data(i)	, const int& Data##i
#define MACRO_ARG_Const(i)	, const int& Const##i
#define MACRO_ARG_Label(i)	, const int& Label##i

#define MACRO_ARG_Func(i)	, FuncCommon Func##i
#define MACRO_ARG_Raw(i)	, const Uint8* Raw##i

#define MACRO_ARG_LOOP_DEFAULT None, None, None, None, None, None, None

#define MACRO_ARG_LOOP_IMPL(LOOP, NAME, A0, A1, A2, A3, A4, A5, A6, A7, ...) \
	LOOP(NAME, 0, A0) \
	LOOP(NAME, 1, A1) \
	LOOP(NAME, 2, A2) \
	LOOP(NAME, 3, A3) \
	LOOP(NAME, 4, A4) \
	LOOP(NAME, 5, A5) \
	LOOP(NAME, 6, A6) \
	LOOP(NAME, 7, A7)
#define MACRO_ARG_LOOP_EXPAND(LOOP, NAME, ...) MACRO_ARG_LOOP_IMPL(LOOP, NAME, __VA_ARGS__)
#define MACRO_ARG_LOOP(LOOP, NAME, ...) MACRO_ARG_LOOP_EXPAND(LOOP, NAME, __VA_ARGS__, MACRO_ARG_LOOP_DEFAULT)

#define MACRO_ARG_MAX_COUNT_LOOP(NAME, POS, ARG) +1
static_assert((MACRO_ARG_LOOP(MACRO_ARG_MAX_COUNT_LOOP, , )) != (ScriptMaxArg - 1), "Arg marco length inconsistent with const length value!");
#undef MACRO_ARG_MAX_COUNT_LOOP

#define MACRO_COPY_4(Func, Pos) \
	Func((Pos) + 0x0) \
	Func((Pos) + 0x1) \
	Func((Pos) + 0x2) \
	Func((Pos) + 0x3)
#define MACRO_COPY_16(Func, Pos) \
	MACRO_COPY_4(Func, (Pos) + 0x00) \
	MACRO_COPY_4(Func, (Pos) + 0x04) \
	MACRO_COPY_4(Func, (Pos) + 0x08) \
	MACRO_COPY_4(Func, (Pos) + 0x0C)
#define MACRO_COPY_64(Func, Pos) \
	MACRO_COPY_16(Func, (Pos) + 0x00) \
	MACRO_COPY_16(Func, (Pos) + 0x10) \
	MACRO_COPY_16(Func, (Pos) + 0x20) \
	MACRO_COPY_16(Func, (Pos) + 0x30)
#define MACRO_COPY_256(Func, Pos) \
	MACRO_COPY_64(Func, (Pos) + 0x00) \
	MACRO_COPY_64(Func, (Pos) + 0x40) \
	MACRO_COPY_64(Func, (Pos) + 0x80) \
	MACRO_COPY_64(Func, (Pos) + 0xC0)

template<typename... Args>
struct AddIndex;

template<typename Arg, typename... Args>
struct AddIndex<Arg, Args...>
{
	static constexpr size_t index = AddIndex<Args...>::index + 1;
	static constexpr size_t size = Arg::size;
	static constexpr size_t offset = AddIndex<Args...>::offset + size;

	template<typename Func, typename... Rest>
	static bool func(ScriptWorker &sw, const Uint8 *procArgs, int &curr, Rest&&... r)
	{
		return AddIndex<Args...>::template func<Func>(sw, procArgs, curr, r..., Arg::get(sw, procArgs - offset, curr));
	}

	static constexpr int backArgOffset(int i)
	{
		return i == index ? offset : AddIndex<Args...>::backArgOffset(i);
	}
};

template<>
struct AddIndex<>
{
	static constexpr size_t index = -1;
	static constexpr size_t size = 0;
	static constexpr size_t offset = 0;

	template<typename Func, typename... Rest>
	static bool func(ScriptWorker &sw, const Uint8 *procArgs, int &curr, Rest&&... r)
	{
		return Func::func(sw, r...);
	}

	static constexpr int backArgOffset(int i)
	{
		return 0;
	}
};

template<typename... A>
struct ArgList
{
	template<typename Func>
	static bool func(ScriptWorker &sw, const Uint8 *proc, int &curr)
	{
		curr += sizeof(Uint8) + AddIndex<A...>::offset;
		return AddIndex<A...>::template func<Func>(sw, proc + curr, curr);
	}

	static constexpr int offset(int i) { return AddIndex<A...>::backArgOffset(sizeof...(A)) - AddIndex<A...>::backArgOffset(sizeof...(A) - i); }
};

template<FuncCommon... F>
struct FuncGroup
{
	static constexpr size_t size = sizeof...(F);
	static constexpr FuncCommon tab[256] =
	{
		F...
	};

	template<FuncCommon... F2>
	constexpr FuncGroup<F..., F2...> operator+(FuncGroup<F2...>) { return {}; }
	constexpr FuncGroup<F...> operator+() { return {}; }
};

template<typename Func, typename... AL>
struct ArgGroup
{
	static constexpr FuncGroup<AL::template func<Func>...> func = {};
	static constexpr FuncOffset offsets[] =
	{
		AL::offset...
	};
};


template<typename... AG>
struct FlattenerImp;

template<typename... AG>
using Flattener = typename FlattenerImp<AG...>::type;


template<typename Func, typename... AL1, typename... AL2, typename... AG>
struct FlattenerImp<ArgGroup<Func, AL1...>, ArgGroup<Func, AL2...>, AG...>
{
	using type = Flattener<ArgGroup<Func, AL1..., AL2...>, AG...>;
};

template<typename AG>
struct FlattenerImp<AG>
{
	using type = AG;
};


template<typename P, typename LL>
struct TransformImp;

template<typename P, typename LL>
using Transform = typename TransformImp<P, LL>::type;


template<typename P, typename... A>
struct TransformImp<P, ArgList<A...>>
{
	using type = ArgList<P, A...>;
};

template<typename P, typename Func, typename... AL>
struct TransformImp<P, ArgGroup<Func, AL...>>
{
	using type = ArgGroup<Func, Transform<P, AL>...>;
};

template<typename Func, typename... AL>
struct TransformImp<ArgList<>, ArgGroup<Func, AL...>>
{
	using type = ArgGroup<Func, AL...>;
};

template<typename... PP, typename Func, typename... AL>
struct TransformImp<ArgList<PP...>, ArgGroup<Func, AL...>>
{
	using type = Flattener<Transform<PP, ArgGroup<Func, AL...>>...>;
};

template<typename Func, typename... AL>
struct TransformImp<ArgGroup<Func>, ArgGroup<Func, AL...>>
{
	using type = ArgGroup<Func, AL...>;
};

template<typename Func, typename AL1Head, typename... AL1Tail, typename... AL2>
struct TransformImp<ArgGroup<Func, AL1Head, AL1Tail...>, ArgGroup<Func, AL2...>>
{
	using type = Transform<AL1Head, Transform<ArgGroup<Func, AL1Tail...>, ArgGroup<Func, AL2...>>>;
};

template<typename Func, typename... AL>
using FuncArray = Transform<ArgGroup<Func, AL...>, ArgGroup<Func, ArgList<>>>;

enum ArgEnum
{
	ArgNone,
	ArgProg,
	ArgTest,
	ArgResult,

	ArgReg,
	ArgData,
	ArgConst,
	ArgLabel,

	ArgFunc,
	ArgRaw,
};

struct ArgProgDef
{
	static constexpr size_t size = 0;
	static int& get(ScriptWorker &sw, const Uint8 *arg, int &curr)
	{
		return curr;
	}
	static bool set(std::vector<Uint8> &proc)
	{
		return true;
	}
};

struct ArgRegDef
{
	static constexpr size_t size = sizeof(Uint8);
	static int& get(ScriptWorker &sw, const Uint8 *arg, int &curr)
	{
		return sw.reg[*arg];
	}
	static bool set(std::vector<Uint8> &proc, Uint8 value)
	{
		proc.push_back(value);
		return true;
	}
};

template<int A>
struct ArgRegFixDef
{
	static constexpr size_t size = 0;
	static int& get(ScriptWorker &sw, const Uint8 *arg, int &curr)
	{
		return sw.reg[A];
	}
	static bool set(std::vector<Uint8> &proc)
	{
		return true;
	}
};

template<typename T>
struct ArgConstDef
{
	static constexpr size_t size = sizeof(T);
	static const T& get(ScriptWorker &sw, const Uint8 *arg, int &curr)
	{
		return *reinterpret_cast<const T*>(arg);
	}
	static bool set(std::vector<Uint8> &proc, const T &value)
	{
		auto pos = proc.size();
		proc.resize(pos + size, 0);
		*reinterpret_cast<const T*>(&proc[pos]) = value;
		return true;
	}
};

template<int Size>
struct ArgRawDef
{
	static constexpr size_t size = Size;
	static const unsigned char* get(ScriptWorker &sw, const Uint8 *arg, int &curr)
	{
		return arg;
	}
	static bool set(std::vector<Uint8> &proc, const Uint8 (&value)[size])
	{
		auto pos = proc.size();
		proc.resize(pos + size, 0);

		memcpy(&proc[pos], value, size);

		return true;
	}
};

////////////////////////////////////////////////////////////
//						reg definition
////////////////////////////////////////////////////////////
enum RegEnum
{
	RegIn,
	RegCond,

	RegR0,
	RegR1,
	RegR2,
	RegR3,

	RegC0,
	RegC1,
	RegC2,
	RegC3,
	RegC4,
	RegC5,

	RegCustom,
	RegEnumMax = RegCustom + ScriptMaxRefCustom,
};

using ArgNoneList = ArgList<>;
using ArgProgList = ArgList<ArgProgDef>;
using ArgTestList = ArgList<ArgRegFixDef<RegCond>>;
using ArgResultList = ArgList<ArgRegFixDef<RegIn>>;

using ArgRegList = ArgList<ArgRegDef>;
using ArgDataList = ArgList<ArgRegDef, ArgConstDef<int>>;
using ArgConstList = ArgList<ArgConstDef<int>>;
using ArgLabelList = ArgList<ArgConstDef<int>>;
using ArgFuncList = ArgList<ArgConstDef<FuncCommon>>;
using ArgRawList = ArgList<ArgRawDef<0>,ArgRawDef<4>,ArgRawDef<8>,ArgRawDef<16>,ArgRawDef<32>,ArgRawDef<64>>;

////////////////////////////////////////////////////////////
//						proc definition
////////////////////////////////////////////////////////////
inline void addShade_h(int& reg, const int& var)
{
	const int newShade = (reg & 0xF) + var;
	if (newShade > 0xF)
	{
		// so dark it would flip over to another color - make it black instead
		reg = 0xF;
		return;
	}
	else if(newShade > 0)
	{
		reg = (reg & 0xF0) | newShade;
		return;
	}
	reg &= 0xF0;
	//prevent overflow to 0 or another color - make it white instead
	if(!reg || newShade < 0)
		reg = 0x01;
}

inline bool mulAddMod_h(int& reg, const int& mul, const int& add, const int& mod)
{
	const int a = reg * mul + add;
	if(mod)
		reg = (a % mod + mod) % mod;
	return !mod;
}

inline bool wavegen_rect_h(int& reg, const int& period, const int& size, const int& max)
{
	if(period <= 0)
		return true;
	reg %= period;
	if(reg < 0)
		reg += reg;
	if(reg > size)
		reg = 0;
	else
		reg = max;
	return false;
}

inline bool wavegen_saw_h(int& reg, const int& period, const int& size, const int& max)
{
	if(period <= 0)
		return true;
	reg %= period;
	if(reg < 0)
		reg += reg;
	if(reg > size)
		reg = 0;
	else if(reg > max)
		reg = max;
	return false;
}

inline bool wavegen_tri_h(int& reg, const int& period, const int& size, const int& max)
{
	if(period <= 0)
		return true;
	reg %= period;
	if(reg < 0)
		reg += reg;
	if(reg > size)
		reg = 0;
	else
	{
		if(reg > size/2)
			reg = size - reg;
		if(reg > max)
			reg = max;
	}
	return false;
}

template<typename... A>
inline bool fillConst(ScriptWorker& Context, A... a)
{
	int array[] =
	{
		a...
	};
	memcpy(&Context.reg[RegC0], array, sizeof(int) * sizeof...(A));

	return false;
}

/**
 * Main macro defining all available operation in script engine.
 * @param IMPL macro function that access data. Take 6 args: Name, type of 4 proc args and definition of operation
 */
#define MACRO_PROC_DEFINITION(IMPL) \
	/*	Name,		Implementation,										End excecution,			Arg0,	Arg1, Arg2, Arg3, ..., Arg7 */ \
	IMPL(exit,		MACRO_QUOTE({										return true; }),		None) \
	\
	IMPL(ret,		MACRO_QUOTE({				Result = Data1;			return true; }),		Result, Data) \
	IMPL(ret_gt,	MACRO_QUOTE({ if(Test > 0)	Result = Data1;			return Test > 0; }),	Result, Data, Test) \
	IMPL(ret_lt,	MACRO_QUOTE({ if(Test < 0)	Result = Data1;			return Test < 0; }),	Result, Data, Test) \
	IMPL(ret_eq,	MACRO_QUOTE({ if(Test == 0)	Result = Data1;			return Test == 0; }),	Result, Data, Test) \
	IMPL(ret_neq,	MACRO_QUOTE({ if(Test != 0)	Result = Data1;			return Test != 0; }),	Result, Data, Test) \
	\
	IMPL(skip,		MACRO_QUOTE({				Prog = Label1;			return false; }),		Prog, Label) \
	IMPL(skip_gt,	MACRO_QUOTE({ if(Test > 0)	Prog = Label1;			return false; }),		Prog, Label, Test) \
	IMPL(skip_lt,	MACRO_QUOTE({ if(Test < 0)	Prog = Label1;			return false; }),		Prog, Label, Test) \
	IMPL(skip_eq,	MACRO_QUOTE({ if(Test == 0)	Prog = Label1;			return false; }),		Prog, Label, Test) \
	IMPL(skip_neq,	MACRO_QUOTE({ if(Test != 0)	Prog = Label1;			return false; }),		Prog, Label, Test) \
	\
	IMPL(set,		MACRO_QUOTE({				Reg0 = Data1;			return false; }),		Reg, Data) \
	IMPL(set_gt,	MACRO_QUOTE({ if(Test > 0)	Reg0 = Data1;			return false; }),		Reg, Data, Test) \
	IMPL(set_lt,	MACRO_QUOTE({ if(Test < 0)	Reg0 = Data1;			return false; }),		Reg, Data, Test) \
	IMPL(set_eq,	MACRO_QUOTE({ if(Test == 0)	Reg0 = Data1;			return false; }),		Reg, Data, Test) \
	IMPL(set_neq,	MACRO_QUOTE({ if(Test != 0)	Reg0 = Data1;			return false; }),		Reg, Data, Test) \
	\
	IMPL(test,		MACRO_QUOTE({ Test = Data1 - Data2;					return false; }),		Test, Data, Data) \
	\
	IMPL(swap,		MACRO_QUOTE({ std::swap(Reg0, Reg1);				return false; }),		Reg, Reg) \
	IMPL(add,		MACRO_QUOTE({ Reg0 += Data1;						return false; }),		Reg, Data) \
	IMPL(sub,		MACRO_QUOTE({ Reg0 -= Data1;						return false; }),		Reg, Data) \
	IMPL(mul,		MACRO_QUOTE({ Reg0 *= Data1;						return false; }),		Reg, Data) \
	\
	IMPL(muladd,	MACRO_QUOTE({ Reg0 = Reg0 * Data1 + Data2;			return false; }),		Reg, Data, Data) \
	IMPL(muladdmod,	MACRO_QUOTE({ return mulAddMod_h(Reg0, Data1, Data2, Data3);		}),		Reg, Data, Data, Data) \
	\
	IMPL(div,		MACRO_QUOTE({ if(Data1) Reg0 /= Data1;				return !Data1; }),		Reg, Data) \
	IMPL(mod,		MACRO_QUOTE({ if(Data1) Reg0 %= Data1;				return !Data1; }),		Reg, Data) \
	\
	IMPL(shl,		MACRO_QUOTE({ Reg0 <<= Data1;						return false; }),		Reg, Data) \
	IMPL(shr,		MACRO_QUOTE({ Reg0 >>= Data1;						return false; }),		Reg, Data) \
	\
	IMPL(abs,		MACRO_QUOTE({ Reg0 = std::abs(Reg0);				return false; }),		Reg) \
	IMPL(min,		MACRO_QUOTE({ Reg0 = std::min(Reg0, Data1);			return false; }),		Reg, Data) \
	IMPL(max,		MACRO_QUOTE({ Reg0 = std::max(Reg0, Data1);			return false; }),		Reg, Data) \
	\
	IMPL(wavegen_rect,	MACRO_QUOTE({ return wavegen_rect_h(Reg0, Data1, Data2, Data3);		}),	Reg, Data, Data, Data) \
	IMPL(wavegen_saw,	MACRO_QUOTE({ return wavegen_saw_h(Reg0, Data1, Data2, Data3);		}),	Reg, Data, Data, Data) \
	IMPL(wavegen_tri,	MACRO_QUOTE({ return wavegen_tri_h(Reg0, Data1, Data2, Data3);		}),	Reg, Data, Data, Data) \
	\
	IMPL(get_color,		MACRO_QUOTE({ Reg0 = Data1 >> 4;					return false; }),	Reg, Data) \
	IMPL(set_color,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF) | (Data1 << 4);	return false; }),	Reg, Data) \
	IMPL(get_shade,		MACRO_QUOTE({ Reg0 = Data1 & 0xF;					return false; }),	Reg, Data) \
	IMPL(set_shade,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF0) | (Data1 & 0xF); return false; }),	Reg, Data) \
	IMPL(add_shade,		MACRO_QUOTE({ addShade_h(Reg0, Data1);				return false; }),	Reg, Data) \
	\
	IMPL(_call,		MACRO_QUOTE({ return Func0(Context, Raw1, Prog);								}),	Func, Raw, Prog) \


////////////////////////////////////////////////////////////
//					function definition
////////////////////////////////////////////////////////////

/**
 * Macro returning name of function
 */
#define MACRO_FUNC_ID(id) Func##id

#define MACRO_CREATE_FUNC_ARG_LOOP(NAME, POS, ARG) MACRO_ARG_##ARG(POS)
/**
 * Macro used for creating functions from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_FUNC(NAME, Impl, ...) \
	struct MACRO_FUNC_ID(NAME) \
	{ \
		static bool func(ScriptWorker& Context MACRO_ARG_LOOP(MACRO_CREATE_FUNC_ARG_LOOP, NAME, __VA_ARGS__)) \
			Impl \
	};

MACRO_PROC_DEFINITION(MACRO_CREATE_FUNC)

#undef MACRO_CREATE_FUNC
#undef MACRO_CREATE_FUNC_ARG_LOOP

////////////////////////////////////////////////////////////
//					Proc_Enum definition
////////////////////////////////////////////////////////////

/**
 * Macro returning enum form ProcEnum
 */
#define MACRO_PROC_ID(id) Proc##id

#define MACRO_CREATE_PROC_ENUM_ARG_LOOP(NAME, POS, ARG) , Arg##ARG##List
/**
 * Macro used for creating ProcEnum from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_PROC_ENUM(NAME, Impl, ...) \
	MACRO_PROC_ID(NAME), \
	Proc##NAME##end = MACRO_PROC_ID(NAME) + FuncArray<MACRO_FUNC_ID(NAME) MACRO_ARG_LOOP(MACRO_CREATE_PROC_ENUM_ARG_LOOP, NAME, __VA_ARGS__)>::func.size - 1,

/**
 * Enum storing id of all avaliable operations in script engine
 */
enum ProcEnum
{
	MACRO_PROC_DEFINITION(MACRO_CREATE_PROC_ENUM)
	ProcEnumMax,
};

#undef MACRO_CREATE_PROC_ENUM
//#undef MACRO_CREATE_PROC_ENUM_ARG_LOOP

////////////////////////////////////////////////////////////
//					core loop function
////////////////////////////////////////////////////////////

/**
 * Core function in script engine used to executing scripts
 * @param in arg that is visible in script under name "in"
 * @param custom0 optional argument with is send to script under user defined name
 * @param custom1 optional argument with is send to script under user defined name
 * @param reg array of int storing data modified and read by script
 * @param proc array storing operation of script
 * @return Result of executing script
 */
inline Uint8 scriptExe(int in, ScriptWorker &data)
{
	memset(data.reg, 0, RegCustom*sizeof(int));
	memcpy(data.reg + RegCustom, data.regCustom, ScriptMaxRefCustom*sizeof(int));
	data.reg[RegIn] = in;
	int curr = 0;
	const Uint8 *const proc = data.proc;
	//--------------------------------------------------
	//			helper macros for this function
	//--------------------------------------------------
	#define MACRO_FUNC_ARRAY(NAME, Impl, ...) + FuncArray<MACRO_FUNC_ID(NAME) MACRO_ARG_LOOP(MACRO_CREATE_PROC_ENUM_ARG_LOOP, NAME, __VA_ARGS__)>::func
	#define MACRO_FUNC_ARRAY_LOOP(POS) case (POS): if (func.tab[POS](data, proc, curr)) return data.reg[RegIn]; else continue;
	//--------------------------------------------------

	static constexpr auto func = MACRO_PROC_DEFINITION(MACRO_FUNC_ARRAY);

	while(true)
	{
		switch(proc[curr])
		{
		MACRO_COPY_256(MACRO_FUNC_ARRAY_LOOP, 0)
//		MACRO_PROC_DEFINITION(MACRO_OP)
		}
	}

	//--------------------------------------------------
	//			removing helper macros
	//--------------------------------------------------
	#undef MACRO_FUNC_ARRAY_LOOP
	#undef MACRO_FUNC_ARRAY
	//--------------------------------------------------
}

////////////////////////////////////////////////////////////
//						helper class
////////////////////////////////////////////////////////////
typedef std::string::const_iterator ite;
typedef typename std::vector<ScriptContainerData>::const_iterator vec_ite;
typedef typename std::map<std::string, ScriptParserData>::const_iterator cop_ite;
typedef typename std::map<std::string, ScriptContainerData>::const_iterator cref_ite;
typedef typename std::map<std::string, ScriptContainerData>::iterator ref_ite;

/**
 * Token type
 */
enum TokenEnum
{
	TokenNone,
	TokenInvaild,
	TokenColon,
	TokenSemicolon,
	TokenSymbol,
	TokenNumber,
};

/**
 * Struct represents position of token in input string
 */
struct SelectedToken
{
	///type of this token
	TokenEnum type;
	//iterator pointing place of first character
	ite begin;
	//iterator pointing place past of last character
	ite end;
};

/**
 * Helper structure used by ScriptParser::parse
 */
struct ParserHelper
{
	///member pointer accessing script operations
	std::vector<Uint8>& proc;
	///member pointer accessing script data
	std::vector<ScriptContainerData>& procRefData;
	///all available operations for script
	const std::map<std::string, ScriptParserData>& procList;
	///all available data for script
	const std::map<std::string, ScriptContainerData>& refList;
	///temporary script data
	std::map<std::string, ScriptContainerData> refListCurr;
	///list of variables uses
	std::vector<size_t> refVariableUses;

	///index of used script data
	Uint8 refIndexUsed;

	/**
	 * Constructor
     * @param scr parsed script
     * @param proc member pointer accessing script operations
     * @param procRefData member pointer accessing script data
     * @param procList all available operations for script
     * @param ref all available data for script
     */
	ParserHelper(
			std::vector<Uint8>& proc,
			std::vector<ScriptContainerData>& procRefData,
			const std::map<std::string, ScriptParserData>& procList,
			const std::map<std::string, ScriptContainerData>& ref):
		proc(proc), procRefData(procRefData),
		procList(procList), refList(ref),
		refListCurr(),
		refVariableUses(),
		refIndexUsed(RegEnumMax)
	{

	}

	/**
	 * Function finalizing parsing of script
     */
	void relese()
	{
		procRefData.reserve(RegEnumMax + refListCurr.size());
		procRefData.resize(RegEnumMax);
		for (ref_ite i = refListCurr.begin(); i != refListCurr.end(); ++i)
			procRefData.push_back(i->second);

		proc.push_back(MACRO_PROC_ID(exit));
		for (size_t p : refVariableUses)
		{
			Uint8 index = proc[p];
			for (ScriptContainerData& s : procRefData)
			{
				if (s.index == index)
				{
					memcpy(&proc[p], &s.value, sizeof(int));
					break;
				}
			}
		}
	}

	/**
	 * Function preparing place and position for intermediate value on proc vector.
     * @param index index in refListCurr
    */
	void pushProcValue(Uint8 index)
	{
		refVariableUses.push_back(proc.size());
		proc.push_back(index);
		for (size_t k = 1; k < sizeof(int); ++k)
			proc.push_back(0);
	}

	/**
	 * Function returning index of label
     * @param s name of label
     * @param index place where store found index of label
     * @return true if label exists and is valid
     */
	bool getLabel(const std::string& s, int& index)
	{
		ref_ite pos = refListCurr.find(s);
		if(pos == refListCurr.end())
		{
			ScriptContainerData data = { ArgLabel, refIndexUsed, -1 };
			refListCurr.insert(std::make_pair(s, data));
			++refIndexUsed;
			index = data.index;
			return true;
		}
		if(pos->second.type != ArgLabel || pos->second.value != -1)
			return false;
		index = pos->second.index;
		return true;
	}

	/**
	 * Function setting offset of label
     * @param s name of label
     * @param offset set value where label is pointing
     * @return true if operation success
     */
	bool setLabel(const std::string& s, int offset)
	{
		ref_ite pos = refListCurr.find(s);
		if(pos == refListCurr.end())
			return true;
		if(pos->second.value != -1)
			return false;
		pos->second.value = offset;
		return true;
	}

	/**
	 * Function returning index of data (can be reg and const too)
     * @param s name of data
     * @param index place where store found index of data
     * @return true if data exists and is valid
     */
	bool getData(const std::string& s, int& index)
	{
		if(getReg(s, index))
			return true;

		if(getConst(s, index))
			return true;

		cref_ite pos = refListCurr.find(s);
		if(pos == refListCurr.end())
		{
			pos = refList.find(s);
			if(pos == refList.end())
			{
				return false;
			}

			//change named constant to unnamed one
			if(pos->second.type == ArgConst)
			{
				std::stringstream ss;
				ss << pos->second.value;
				return getConst(ss.str(), index);
			}

			ScriptContainerData data = pos->second;
			if(data.type != ArgReg)
			{
				data.index = refIndexUsed;
				++refIndexUsed;
			}
			pos = refListCurr.insert(std::make_pair(s, data)).first;
		}
		if(pos->second.type == ArgLabel)
			return false;
		index = pos->second.index;
		return true;
	}

	/**
	 * Function returning index of const
     * @param s name of const
     * @param index place where store found index of const
     * @return true if const exists and is valid
     */
	bool getConst(const std::string& s, int& index)
	{
		cref_ite pos = refListCurr.find(s);
		if(pos == refListCurr.end())
		{
			int value = 0;
			size_t offset = 0;
			std::stringstream ss(s);
			if(s[0] == '-' || s[0] == '+')
				offset = 1;
			if(s.size() > 2 + offset && s[offset] == '0' && (s[offset + 1] == 'x' || s[offset + 1] == 'X'))
				ss >> std::hex;
			if(!(ss >> value))
				return false;

			//normalize string for constant value
			ss.str("");
			ss.clear();
			ss << std::dec;
			ss << value;
			std::string newS = ss.str();
			pos = refListCurr.find(newS);
			if(pos == refListCurr.end())
			{
				ScriptContainerData data = { ArgConst, refIndexUsed, value };
				refListCurr.insert(std::make_pair(newS, data));
				++refIndexUsed;
				index = data.index;
				return true;
			}
		}
		if(pos->second.type != ArgConst)
			return false;
		index = pos->second.index;
		return true;
	}

	/**
	 * Function returning index of reg
     * @param s name of reg
     * @param index place where store found index of reg
     * @return true if reg exists and is valid
     */
	bool getReg(const std::string& s, int& index)
	{
		if(s.compare("in") == 0)
		{
			index = RegIn;
			return true;
		}
		else if(s.compare("r0") == 0)
		{
			index = RegR0;
			return true;
		}
		else if(s.compare("r1") == 0)
		{
			index = RegR1;
			return true;
		}
		else if(s.compare("r2") == 0)
		{
			index = RegR2;
			return true;
		}
		else if(s.compare("r3") == 0)
		{
			index = RegR3;
			return true;
		}
		return false;
	}

	/**
	 * Function finding next token in string
     * @param pos current position in string
     * @param end end of string
     * @return true if pos pointing on next token
     */
	bool findToken(ite& pos, const ite& end)
	{
		bool coment = false;
		for(; pos != end; ++pos)
		{
			const char c = *pos;
			if(coment)
			{
				if(c == '\n')
				{
					coment = false;
				}
				continue;
			}
			else if(isspace(c))
			{
				continue;
			}
			else if(c == '#')
			{
				coment = true;
				continue;
			}
			return true;
		}
		return false;
	}

	/**
	 * Function extracting token form string
     * @param pos current position in string
     * @param end end of string
     * @param excepted what token type we expecting now
     * @return extracted token
     */
	SelectedToken getToken(ite& pos, const ite& end, TokenEnum excepted = TokenNone)
	{
		//groups of different types of ASCII characters
		const Uint8 none = 1;
		const Uint8 spec = 2;
		const Uint8 digit = 3;
		const Uint8 charHex = 4;
		const Uint8 charRest = 5;
		const Uint8 digitSign = 6;

		//array storing data about every ASCII character
		static Uint8 charDecoder[256] = { 0 };
		static bool init = true;
		if(init)
		{
			init = false;
			for(int i = 0; i < 256; ++i)
			{
				if(i == '#' || isspace(i))	charDecoder[i] = none;
				if(i == ':' || i == ';')	charDecoder[i] = spec;

				if(i == '+' || i == '-')	charDecoder[i] = digitSign;
				if(i >= '0' && i <= '9')	charDecoder[i] = digit;
				if(i >= 'A' && i <= 'F')	charDecoder[i] = charHex;
				if(i >= 'a' && i <= 'f')	charDecoder[i] = charHex;

				if(i >= 'G' && i <= 'Z')	charDecoder[i] = charRest;
				if(i >= 'g' && i <= 'z')	charDecoder[i] = charRest;
				if(i == '_' || i == '.')	charDecoder[i] = charRest;
			}
		}

		SelectedToken token = { TokenNone, end, end };
		if(!findToken(pos, end))
			return token;

		token.begin = pos;
		bool hex = false;
		int off = 0;

		for(; pos != end; ++pos, ++off)
		{
			const Uint8 c = *pos;
			const Uint8 decode = charDecoder[c];

			//end of string
			if(decode == none)
			{
				break;
			}
			else if(decode == spec)
			{
				if(c == ':')
				{
					//colon start new token, skip if we are in another one
					if(token.type != TokenNone)
						break;

					++pos;
					token.type = excepted == TokenColon ? TokenColon : TokenInvaild;
					break;
				}
				else if(c == ';')
				{
					//semicolon start new token, skip if we are in another one
					if(token.type != TokenNone)
						break;
					//semicolon wait for his turn, returning empty token
					if(excepted != TokenSemicolon)
						break;

					++pos;
					token.type = TokenSemicolon;
					break;
				}
				else
				{
					token.type = TokenInvaild;
					break;
				}
			}

			switch(token.type)
			{
			//begin of string
			case TokenNone:
				switch(decode)
				{
				//start of number
				case digitSign:
					--off; //skipping +- sign
				case digit:
					hex = c == '0'; //expecting hex number
					token.type = TokenNumber;
					continue;

				//start of symbol
				case charHex:
				case charRest:
					token.type = TokenSymbol;
					continue;
				}
				break;

			//middle of number
			case TokenNumber:
				switch(decode)
				{
				case charRest:
					if(off != 1) break;
					if(c != 'x' && c != 'X') break; //X in "0x1"
				case charHex:
					if(!hex) break;
				case digit:
					if(off == 0) hex = c == '0'; //expecting hex number
					continue;
				}
				break;

			//middle of symbol
			case TokenSymbol:
				switch(decode)
				{
				case charRest:
				case charHex:
				case digit:
					continue;
				}
				break;
			default:
				break;
			}
			//when decode == 0 or we find unexpected char we should end there
			token.type = TokenInvaild;
			break;
		}
		token.end = pos;
		return token;
	}
}; //struct ParserHelper

} //namespace


////////////////////////////////////////////////////////////
//						Script class
////////////////////////////////////////////////////////////
void ScriptContainerBase::updateBase(ScriptWorker* ref, const void* t, int (*cast)(const void*, ScriptContainerData::voidFunc)) const
{
	ref->proc = &(_proc[0]);
	for(vec_ite i = _procRefData.begin(); i != _procRefData.end(); ++i)
	{
		int& r = ref->reg[i->index];
		switch(i->type)
		{
		case ArgReg:
			r = 0;
			break;
		case ArgConst:
			r = i->value;
			break;
		case ArgLabel:
			r = i->value;
			break;
		case ArgData:
			r = cast(t, i->envFunc);
			break;
		}
	}
}

namespace
{

/**
 * struct used to bliting script
 */
struct ScriptReplace
{
	static inline void func(Uint8& dest, const Uint8& src, ScriptWorker& ref)
	{
		if(src)
		{
			const int s = scriptExe(src, ref);
			if (s) dest = s;
		}
	}
};

} //namespace



/**
 * Bliting one surface to another using script
 * @param src source surface
 * @param dest destination surface
 * @param x x offset of source surface
 * @param y y offset of source surface
 */
void ScriptWorker::executeBlit(Surface* src, Surface* dest, int x, int y, bool half)
{
	ShaderMove<Uint8> srcShader(src, x, y);
	if (half)
	{
		GraphSubset g = srcShader.getDomain();
		g.beg_x = g.end_x/2;
		srcShader.setDomain(g);
	}
	if(proc)
		ShaderDraw<ScriptReplace>(ShaderSurface(dest, 0, 0), srcShader, ShaderScalar(*this));
	else
		ShaderDraw<helper::StandardShade>(ShaderSurface(dest, 0, 0), srcShader, ShaderScalar(shade));
}
////////////////////////////////////////////////////////////
//					ScriptParser class
////////////////////////////////////////////////////////////

/**
 * Default constructor
 */
ScriptParserBase::ScriptParserBase(const std::string& name) : _name(name), _procList(), _refList()
{
	//--------------------------------------------------
	//					op_data init
	//--------------------------------------------------
	#define MACRO_ALL_INIT_ARG_LOOP_TYPES(NAME, POS, ARG) Arg##ARG ,
	#define MACRO_ALL_INIT_ARG_LOOP_OFFSET(NAME, POS, ARG) (Uint8)NAME(POS) ,
	#define MACRO_ALL_INIT(NAME, Impl, ...) \
		_procList[#NAME] = \
		{ \
			MACRO_PROC_ID(NAME), \
			{ \
				/* types */\
				MACRO_ARG_LOOP(MACRO_ALL_INIT_ARG_LOOP_TYPES, NAME, __VA_ARGS__) \
			}, \
			{ \
			/* args offsets */\
				MACRO_ARG_LOOP(MACRO_ALL_INIT_ARG_LOOP_OFFSET, (FuncArray<MACRO_FUNC_ID(NAME) MACRO_ARG_LOOP(MACRO_CREATE_PROC_ENUM_ARG_LOOP, NAME, __VA_ARGS__)>::offsets[0]), __VA_ARGS__) \
			}, \
		};

	MACRO_PROC_DEFINITION(MACRO_ALL_INIT)

	#undef MACRO_ALL_INIT
	#undef MACRO_ALL_INIT_ARG_LOOP_OFFSET
	#undef MACRO_ALL_INIT_ARG_LOOP_TYPES
}

/**
 * Add new function extracting data for script
 * @param s name for new data
 * @param f void function pointer to function extrating data
 */
void ScriptParserBase::addFunctionBase(const std::string& s, ScriptContainerData::voidFunc f)
{
	ref_ite pos = _refList.find(s);
	if(pos == _refList.end())
	{
		ScriptContainerData data = { ArgData, 0, 0, f };
		_refList.insert(std::make_pair(s, data));
	}
	else
	{
		pos->second.envFunc = f;
	}
}

/**
 * Set name for custom script param
 * @param i what custom param
 * @param s name for first custom parameter
 */
void ScriptParserBase::addCustom(int i, const std::string& s)
{
	if(i >= 0 && i < ScriptMaxRefCustom)
	{
		ScriptContainerData data = { ArgReg, (Uint8)(RegCustom + i) };
		_refList.insert(std::make_pair(s, data));
	}
}

/**
 * Add const value to script
 * @param s name for const
 * @param i value
 */
void ScriptParserBase::addConst(const std::string& s, int i)
{
	ScriptContainerData data = { ArgConst, 0, i };
	_refList.insert(std::make_pair(s, data));
}

/**
 * Parse string and write script to ScriptBase
 * @param src struct where final script is write to
 * @param src_code string with script
 * @return true if string have valid script
 */
bool ScriptParserBase::parseBase(ScriptContainerBase* destScript, const std::string& srcCode) const
{
	ParserHelper help(
		destScript->_proc, destScript->_procRefData,
		_procList, _refList
	);
	ite curr = srcCode.begin();
	ite end = srcCode.end();
	if(curr == end)
		return false;

	while(true)
	{
		if(!help.findToken(curr, end))
		{
			if(help.refIndexUsed > ScriptMaxRef)
			{
				Log(LOG_ERROR) << "script used to many references\n";
				return false;
			}
			for(ref_ite i = help.refListCurr.begin(); i != help.refListCurr.end(); ++i)
			{
				if(i->second.type == ArgLabel && i->second.value == -1)
				{
					Log(LOG_ERROR) << "invalid use of label: '" << i->first << "' without declaration\n";
					return false;
				}
			}
			help.relese();
			return true;
		}

		ite line_begin = curr;
		ite line_end;
		SelectedToken label = { TokenNone };

		SelectedToken op = help.getToken(curr, end);
		SelectedToken args[ScriptMaxArg];
		args[0] = help.getToken(curr, end, TokenColon);
		if(args[0].type == TokenColon)
		{
			std::swap(op, label);
			op = help.getToken(curr, end);
			args[0] = help.getToken(curr, end);
		}
		for(int i = 1; i < ScriptMaxArg; ++i)
			args[i] = help.getToken(curr, end);
		SelectedToken f = help.getToken(curr, end, TokenSemicolon);

		line_end = curr;
		//validation
		bool valid = true;
		valid &= label.type == TokenSymbol || label.type == TokenNone;
		valid &= op.type == TokenSymbol;
		for(int i = 0; i < ScriptMaxArg; ++i)
			valid &= args[i].type == TokenSymbol || args[i].type == TokenNumber || args[i].type == TokenNone;
		valid &= f.type == TokenSemicolon;

		if(!valid)
		{
			if(f.type != TokenSemicolon)
			{
				//fixing `line_end` position
				while(curr != end && *curr != ';')
					++curr;
				if(curr != end)
					++curr;
				line_end = curr;
			}
			Log(LOG_ERROR) << "invalid line: '" << std::string(line_begin, line_end) << "'\n";
			return false;
		}
		else
		{
			int temp = 0;
			std::string op_str(op.begin, op.end);
			std::string label_str(label.begin, label.end);
			cop_ite op_curr = _procList.find(op_str);
			if(op_curr == _procList.end())
			{
				Log(LOG_ERROR) << "invalid operation '"<< op_str <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
				return false;
			}

			if(!label_str.empty() && !help.setLabel(label_str, destScript->_proc.size()))
			{
				Log(LOG_ERROR) << "invalid label '"<< label_str <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
				return false;
			}

			//matching args form operation definition with args avaliable in string
			int i = 0, j = 0;
			destScript->_proc.push_back(op_curr->second.procId);
			while(i < ScriptMaxArg && j < ScriptMaxArg)
			{
				std::string arg_val(args[j].begin, args[j].end);
				switch(op_curr->second.argType[i])
				{
				//this is special args that arent visible in source, skip
				case ArgNone:
				case ArgProg:
				case ArgTest:
				case ArgResult:
					++i;
					continue;

				//arg that is label
				case ArgLabel:
					if(!help.getLabel(arg_val, temp))
					{
						Log(LOG_ERROR) << "invalid label argument '"<< arg_val <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
						return false;
					}
					help.pushProcValue(temp);
					break;

				//arg that is data like "solder_gender", "hp" etc. can be const or reg too.
				case ArgData:
					if(!help.getData(arg_val, temp))
					{
						Log(LOG_ERROR) << "invalid data argument '"<< arg_val <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
						return false;
					}
					destScript->_proc.push_back(temp);
					break;

				//arg that is const value e.g. "1", "-0x13"
				case ArgConst:
					if(!help.getConst(arg_val, temp))
					{
						Log(LOG_ERROR) << "invalid const argument '"<< arg_val <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
						return false;
					}
					help.pushProcValue(temp);
					break;

				//arg that is reg like "in", "r0"
				case ArgReg:
					if(!help.getReg(arg_val, temp))
					{
						Log(LOG_ERROR) << "invalid reg argument '"<< arg_val <<"' in line: '" << std::string(line_begin, line_end) << "'\n";
						return false;
					}
					destScript->_proc.push_back(temp);
					break;
				}
				++i;
				++j;
			}
			if(!(j == ScriptMaxArg || args[j].type == TokenNone))
			{
				Log(LOG_ERROR) << "wrong arguments in line: '" << std::string(line_begin, line_end) << "'\n";
				return false;
			}
		}
	}
}

/**
 * Print all metadata
 */
void ScriptParserBase::logScriptMetadata() const
{
	if (Options::debug)
	{
		const int tabSize = 8;
		static bool printOp = true;
		if(printOp)
		{
			printOp = false;
			Logger opLog;
			#define MACRO_STRCAT(A) #A
			#define MACRO_ALL_LOG_ARG_LOOP(NAME, POS, ARG) << std::setw(std::string("None").compare(#ARG) ? tabSize : 0) << (std::string("None").compare(#ARG) ? #ARG "," : "")
			#define MACRO_ALL_LOG(NAME, Impl, ...) \
				if (#NAME[0] != '_') opLog.get() \
					<< "Op:    " << std::setw(tabSize*2) << #NAME \
					<< "Impl:  " << std::setw(tabSize*10) << MACRO_STRCAT(Impl) \
					<< "Args:  " MACRO_ARG_LOOP(MACRO_ALL_LOG_ARG_LOOP, NAME, __VA_ARGS__) << FuncArray<MACRO_FUNC_ID(NAME) MACRO_ARG_LOOP(MACRO_CREATE_PROC_ENUM_ARG_LOOP, NAME, __VA_ARGS__)>::func.size << "\n";

			opLog.get() << "Available script operations:\n" << std::left;
			MACRO_PROC_DEFINITION(MACRO_ALL_LOG)

			#undef MACRO_ALL_LOG
			#undef MACRO_ALL_LOG_ARG_LOOP
			#undef MACRO_STRCAT
		}

		Logger refLog;
		refLog.get() << "Script data for: " << _name << "\n" << std::left;
		for(cref_ite ite = _refList.begin(); ite != _refList.end(); ++ite)
		{
			if(ite->second.type == ArgConst)
				refLog.get() << "Ref: " << std::setw(30) << ite->first << "Value: " << ite->second.value << "\n";
			else
				refLog.get() << "Ref: " << std::setw(30) << ite->first << "\n";
		}
	}
}

} //namespace OpenXcom
