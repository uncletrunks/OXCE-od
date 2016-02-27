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
#include <tuple>
#include <algorithm>

#include "Logger.h"
#include "Options.h"
#include "Script.h"
#include "ScriptBind.h"
#include "Surface.h"
#include "ShaderDraw.h"
#include "ShaderMove.h"
#include "Exception.h"


namespace OpenXcom
{

////////////////////////////////////////////////////////////
//						arg definition
////////////////////////////////////////////////////////////
#define MACRO_QUOTE(...) __VA_ARGS__

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


////////////////////////////////////////////////////////////
//						proc definition
////////////////////////////////////////////////////////////
[[gnu::always_inline]]
static inline void addShade_h(int& reg, const int& var)
{
	const int newShade = (reg & 0xF) + var;
	if (newShade > 0xF)
	{
		// so dark it would flip over to another color - make it black instead
		reg = 0xF;
		return;
	}
	else if (newShade > 0)
	{
		reg = (reg & 0xF0) | newShade;
		return;
	}
	reg &= 0xF0;
	//prevent overflow to 0 or another color - make it white instead
	if (!reg || newShade < 0)
		reg = 0x01;
}

[[gnu::always_inline]]
static inline RetEnum mulAddMod_h(int& reg, const int& mul, const int& add, const int& mod)
{
	const int a = reg * mul + add;
	if (mod)
	{
		reg = (a % mod + mod) % mod;
		return RetContinue;
	}
	return RetError;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_rect_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += reg;
	if (reg > size)
		reg = 0;
	else
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_saw_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += reg;
	if (reg > size)
		reg = 0;
	else if (reg > max)
		reg = max;
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum wavegen_tri_h(int& reg, const int& period, const int& size, const int& max)
{
	if (period <= 0)
		return RetError;
	reg %= period;
	if (reg < 0)
		reg += reg;
	if (reg > size)
		reg = 0;
	else
	{
		if (reg > size/2)
			reg = size - reg;
		if (reg > max)
			reg = max;
	}
	return RetContinue;
}

[[gnu::always_inline]]
static inline RetEnum call_func_h(ScriptWorker& c, FuncCommon func, const Uint8* d, ProgPos& p)
{
	auto t = p;
	auto r = func(c, d, t);
	p = t;
	return r;
}

[[gnu::always_inline]]
static inline RetEnum debug_log_h(ProgPos& p, int i, int j)
{
	if (Options::debug)
	{
		static constexpr int offset = 3; //+1 for op, and +2*1 for two reg args.
		static int limit_count = 0;
		if (++limit_count < 500)
		{
			Logger log;
			log.get(LOG_DEBUG) << "Script debug log at " << std::hex << std::showbase << std::setw(8) << ((int)p - offset) << " values: " << std::setw(8) << i << std::setw(8) << j;
		}
	}
	return RetContinue;
}


/**
 * Main macro defining all available operation in script engine.
 * @param IMPL macro function that access data. Take 3 args: Name, definition of operation and delcaration of it's arguments.
 */
#define MACRO_PROC_DEFINITION(IMPL) \
	/*	Name,		Implementation,													End excecution,				Args */ \
	IMPL(exit,		MACRO_QUOTE({													return RetEnd;		}),		(ScriptWorker &)) \
	\
	IMPL(ret,		MACRO_QUOTE({								c.ref<int>(RegI0) = Data0;	return RetEnd;						}),		(ScriptWorker& c, int Data0)) \
	IMPL(ret_gt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) > 0)	{ c.ref<int>(RegI0) = Data0; return RetEnd; } return RetContinue; }),	(ScriptWorker& c, int Data0)) \
	IMPL(ret_lt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) < 0)	{ c.ref<int>(RegI0) = Data0; return RetEnd; } return RetContinue; }),	(ScriptWorker& c, int Data0)) \
	IMPL(ret_eq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) == 0)	{ c.ref<int>(RegI0) = Data0; return RetEnd; } return RetContinue; }),	(ScriptWorker& c, int Data0)) \
	IMPL(ret_neq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) != 0)	{ c.ref<int>(RegI0) = Data0; return RetEnd; } return RetContinue; }),	(ScriptWorker& c, int Data0)) \
	\
	IMPL(goto,		MACRO_QUOTE({								Prog = Label1;			return RetContinue; }),		(ScriptWorker& c, ProgPos& Prog, ProgPos Label1)) \
	IMPL(goto_gt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) > 0)	Prog = Label1;			return RetContinue; }),		(ScriptWorker& c, ProgPos& Prog, ProgPos Label1)) \
	IMPL(goto_lt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) < 0)	Prog = Label1;			return RetContinue; }),		(ScriptWorker& c, ProgPos& Prog, ProgPos Label1)) \
	IMPL(goto_eq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) == 0)	Prog = Label1;			return RetContinue; }),		(ScriptWorker& c, ProgPos& Prog, ProgPos Label1)) \
	IMPL(goto_neq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) != 0)	Prog = Label1;			return RetContinue; }),		(ScriptWorker& c, ProgPos& Prog, ProgPos Label1)) \
	\
	IMPL(set,		MACRO_QUOTE({								Reg0 = Data1;			return RetContinue; }),		(ScriptWorker& c, int& Reg0, int Data1)) \
	IMPL(set_gt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) > 0)	Reg0 = Data1;			return RetContinue; }),		(ScriptWorker& c, int& Reg0, int Data1)) \
	IMPL(set_lt,	MACRO_QUOTE({ if (c.ref<int>(RegCond) < 0)	Reg0 = Data1;			return RetContinue; }),		(ScriptWorker& c, int& Reg0, int Data1)) \
	IMPL(set_eq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) == 0)	Reg0 = Data1;			return RetContinue; }),		(ScriptWorker& c, int& Reg0, int Data1)) \
	IMPL(set_neq,	MACRO_QUOTE({ if (c.ref<int>(RegCond) != 0)	Reg0 = Data1;			return RetContinue; }),		(ScriptWorker& c, int& Reg0, int Data1)) \
	\
	IMPL(test,		MACRO_QUOTE({ c.ref<int>(RegCond) = Data0 - Data1;				return RetContinue; }),		(ScriptWorker& c, int Data0, int Data1)) \
	IMPL(test_le,	MACRO_QUOTE({ Prog = (A <= B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse)) \
	IMPL(test_eq,	MACRO_QUOTE({ Prog = (A == B) ? LabelTrue : LabelFalse;			return RetContinue; }),		(ProgPos& Prog, int A, int B, ProgPos LabelTrue, ProgPos LabelFalse)) \
	\
	IMPL(swap,		MACRO_QUOTE({ std::swap(Reg0, Reg1);							return RetContinue; }),		(int& Reg0, int& Reg1)) \
	IMPL(add,		MACRO_QUOTE({ Reg0 += Data1;									return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(sub,		MACRO_QUOTE({ Reg0 -= Data1;									return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(mul,		MACRO_QUOTE({ Reg0 *= Data1;									return RetContinue; }),		(int& Reg0, int Data1)) \
	\
	IMPL(aggregate,	MACRO_QUOTE({ Reg0 = Reg0 + Data1 * Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2)) \
	IMPL(offset,	MACRO_QUOTE({ Reg0 = Reg0 * Data1 + Data2;						return RetContinue; }),		(int& Reg0, int Data1, int Data2)) \
	IMPL(offsetmod,	MACRO_QUOTE({ return mulAddMod_h(Reg0, Data1, Data2, Data3);						}),		(int& Reg0, int Data1, int Data2, int Data3)) \
	\
	IMPL(div,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 /= Data1;		return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(mod,		MACRO_QUOTE({ if (!Data1) return RetError; Reg0 %= Data1;		return RetContinue; }),		(int& Reg0, int Data1)) \
	\
	IMPL(shl,		MACRO_QUOTE({ Reg0 <<= Data1;									return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(shr,		MACRO_QUOTE({ Reg0 >>= Data1;									return RetContinue; }),		(int& Reg0, int Data1)) \
	\
	IMPL(abs,			MACRO_QUOTE({ Reg0 = std::abs(Reg0);							return RetContinue; }),		(int& Reg0)) \
	IMPL(limit,			MACRO_QUOTE({ Reg0 = std::max(std::min(Reg0, Data2), Data1);	return RetContinue; }),		(int& Reg0, int Data1, int Data2)) \
	IMPL(limit_upper,	MACRO_QUOTE({ Reg0 = std::min(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(limit_lower,	MACRO_QUOTE({ Reg0 = std::max(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1)) \
	\
	IMPL(wavegen_rect,	MACRO_QUOTE({ return wavegen_rect_h(Reg0, Data1, Data2, Data3);					}),		(int& Reg0, int Data1, int Data2, int Data3)) \
	IMPL(wavegen_saw,	MACRO_QUOTE({ return wavegen_saw_h(Reg0, Data1, Data2, Data3);					}),		(int& Reg0, int Data1, int Data2, int Data3)) \
	IMPL(wavegen_tri,	MACRO_QUOTE({ return wavegen_tri_h(Reg0, Data1, Data2, Data3);					}),		(int& Reg0, int Data1, int Data2, int Data3)) \
	\
	IMPL(get_color,		MACRO_QUOTE({ Reg0 = Data1 >> 4;							return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(set_color,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF) | (Data1 << 4);			return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(get_shade,		MACRO_QUOTE({ Reg0 = Data1 & 0xF;							return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(set_shade,		MACRO_QUOTE({ Reg0 = (Reg0 & 0xF0) | (Data1 & 0xF);			return RetContinue; }),		(int& Reg0, int Data1)) \
	IMPL(add_shade,		MACRO_QUOTE({ addShade_h(Reg0, Data1);						return RetContinue; }),		(int& Reg0, int Data1)) \
	\
	IMPL(call,			MACRO_QUOTE({ return call_func_h(c, func, d, p);								}),		(FuncCommon func, const Uint8* d, ScriptWorker& c, ProgPos& p)) \
	\
	IMPL(debug_log,		MACRO_QUOTE({ return debug_log_h(p, Data1, Data2);								}),		(ProgPos& p, int Data1, int Data2)) \


////////////////////////////////////////////////////////////
//					function definition
////////////////////////////////////////////////////////////

namespace
{

/**
 * Macro returning name of function
 */
#define MACRO_FUNC_ID(id) Func_##id

/**
 * Macro used for creating functions from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_FUNC(NAME, Impl, Args) \
	struct MACRO_FUNC_ID(NAME) \
	{ \
		[[gnu::always_inline]] \
		static RetEnum func Args \
			Impl \
	};

MACRO_PROC_DEFINITION(MACRO_CREATE_FUNC)

#undef MACRO_CREATE_FUNC

} //namespace

////////////////////////////////////////////////////////////
//					Proc_Enum definition
////////////////////////////////////////////////////////////

/**
 * Macro returning enum form ProcEnum
 */
#define MACRO_PROC_ID(id) Proc_##id

/**
 * Macro used for creating ProcEnum from MACRO_PROC_DEFINITION
 */
#define MACRO_CREATE_PROC_ENUM(NAME, ...) \
	MACRO_PROC_ID(NAME), \
	Proc_##NAME##_end = MACRO_PROC_ID(NAME) + FuncGroup<MACRO_FUNC_ID(NAME)>::ver() - 1,

/**
 * Enum storing id of all avaliable operations in script engine
 */
enum ProcEnum : Uint8
{
	MACRO_PROC_DEFINITION(MACRO_CREATE_PROC_ENUM)
	Proc_EnumMax,
};

#undef MACRO_CREATE_PROC_ENUM

////////////////////////////////////////////////////////////
//					core loop function
////////////////////////////////////////////////////////////

/**
 * Core function in script engine used to executing scripts
 * @param i0 arg that is visible in script under name "i0"
 * @param i1 arg that is visible in script under name "i1"
 * @param proc array storing operation of script
 * @return Result of executing script
 */
static inline int scriptExe(int i0, int i1, ScriptWorker& data)
{
	data.ref<int>(RegI0) = i0;
	data.ref<int>(RegI1) = i1;
	ProgPos curr = ProgPos::Start;
	const Uint8 *const proc = data.proc;
	//--------------------------------------------------
	//			helper macros for this function
	//--------------------------------------------------
	#define MACRO_FUNC_ARRAY(NAME, ...) + FuncGroup<MACRO_FUNC_ID(NAME)>::FuncList{}
	#define MACRO_FUNC_ARRAY_LOOP(POS) \
		case (POS): \
		{ \
			using currType = GetType<func, POS>; \
			const auto p = proc + (int)curr; \
			curr += currType::offset; \
			const auto ret = currType::func(data, p, curr); \
			if (ret != RetContinue) \
			{ \
				if (ret == RetEnd) \
					goto endLabel; \
				else \
				{ \
					curr += - currType::offset - 1; \
					goto errorLabel; \
				} \
			} \
			else \
				continue; \
		}
	//--------------------------------------------------

	using func = decltype(MACRO_PROC_DEFINITION(MACRO_FUNC_ARRAY));

	while (true)
	{
		switch (proc[(int)curr++])
		{
		MACRO_COPY_256(MACRO_FUNC_ARRAY_LOOP, 0)
		}
	}

	//--------------------------------------------------
	//			removing helper macros
	//--------------------------------------------------
	#undef MACRO_FUNC_ARRAY_LOOP
	#undef MACRO_FUNC_ARRAY
	//--------------------------------------------------

	endLabel:
	return data.ref<int>(RegI0);
	errorLabel:
	static int bugCount = 0;
	if (++bugCount < 100)
	{
		Log(LOG_ERROR) << "Invaild script operation for OpId: " << std::hex << std::showbase << (int)proc[(int)curr] <<" at "<< (int)curr;
	}
	return 0;
}


////////////////////////////////////////////////////////////
//						Script class
////////////////////////////////////////////////////////////

namespace
{

/**
 * struct used to bliting script
 */
struct ScriptReplace
{
	static inline void func(Uint8& dest, const Uint8& src, ScriptWorker& ref)
	{
		if (src)
		{
			const int s = scriptExe(src, dest, ref);
			if (s) dest = s;
		}
	}
};

} //namespace



/**
 * Bliting one surface to another using script.
 * @param src source surface.
 * @param dest destination surface.
 * @param x x offset of source surface.
 * @param y y offset of source surface.
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
	if (proc)
		ShaderDraw<ScriptReplace>(ShaderSurface(dest, 0, 0), srcShader, ShaderScalar(*this));
	else
		ShaderDraw<helper::StandardShade>(ShaderSurface(dest, 0, 0), srcShader, ShaderScalar(shade));
}

/**
 * Execute script with two arguments.
 * @param i0 arg that is visible in script under name "i0"
 * @param i1 arg that is visible in script under name "i1"
 * @return Result value from script.
 */
int ScriptWorker::execute(int i0, int i1)
{
	return scriptExe(i0, i1, *this);
}

////////////////////////////////////////////////////////////
//					ScriptParser class
////////////////////////////////////////////////////////////

template<Uint8 procId, typename FuncGroup>
static bool parseLine(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
{
	auto opPos = ph.pushProc(procId);
	int ver = FuncGroup::parse(ph, begin, end);
	if (ver >= 0)
	{
		ph.updateProc(opPos, ver);
		return true;
	}
	else
	{
		return false;
	}
}

static inline bool parseCustomFunc(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
{
	using argFunc = typename ArgSelector<FuncCommon>::type;
	using argRaw = typename ArgSelector<const Uint8*>::type;
	static_assert(FuncGroup<Func_call>::ver() == argRaw::ver(), "Invalid size");
	static_assert(std::is_same<GetType<FuncGroup<Func_call>, 0>, argFunc>::value, "Invalid first argument");
	static_assert(std::is_same<GetType<FuncGroup<Func_call>, 1>, argRaw>::value, "Invalid second argument");

	auto opPos = ph.pushProc(Proc_call);

	auto funcPos = ph.pushReserved<FuncCommon>();
	auto argPosBegin = ph.getCurrPos();

	auto argType = spd.arg(ph, begin, end);

	if (argType < 0)
	{
		return false;
	}

	auto argPosEnd = ph.getCurrPos();
	ph.updateReserved<FuncCommon>(funcPos, spd.get(argType));

	size_t diff = ph.getDiffPos(argPosBegin, argPosEnd);
	for (int i = 0; i < argRaw::ver(); ++i)
	{
		size_t off = argRaw::offset(i);
		if (off >= diff)
		{
			//aligin proc to fit fixed size.
			ph.pushPadding(off-diff);
			ph.updateProc(opPos, i);
			return true;
		}
	}
	return false;
}

static bool parseConditionImpl(ParserWriter& ph, int nextPos, const SelectedToken* begin, const SelectedToken* end)
{
	constexpr size_t TempArgsSize = 4;
	constexpr size_t OperatorSize = 6;

	if (std::distance(begin, end) != 3)
	{
		Log(LOG_ERROR) << "invaild length of condition arguments";
		return false;
	}

	const auto currPos = ph.addLabel();

	SelectedToken conditionArgs[TempArgsSize] =
	{
		begin[1],
		begin[2],
		{ TokenBuildinLabel, currPos, }, //success
		{ TokenBuildinLabel, nextPos, }, //failure
	};

	const char* opNames[OperatorSize] =
	{
		"eq", "neq",
		"le", "gt",
		"ge", "lt",
	};

	bool equalFunc = false;
	size_t i = 0;
	for (; i < OperatorSize; ++i)
	{
		if (begin[0] == ScriptRef{ opNames[i] })
		{
			if (i < 2) equalFunc = true;
			if (i & 1) std::swap(conditionArgs[2], conditionArgs[3]); //negate condition result
			if (i >= 4) std::swap(conditionArgs[0], conditionArgs[1]); //swap condition args
			break;
		}
	}
	if (i == OperatorSize)
	{
		Log(LOG_ERROR) << "unknown condition: '" + begin[0].toString() + "'";
		return false;
	}

	auto* argType0 = ph.getReferece(conditionArgs[0]);
	auto* argType1 = ph.getReferece(conditionArgs[1]);
	if (!argType0)
	{
		Log(LOG_ERROR) << "invalid argument: '" + conditionArgs[0].toString() + "'";
		return false;
	}
	if (!argType1)
	{
		Log(LOG_ERROR) << "invalid argument: '" + conditionArgs[1].toString() + "'";
		return false;
	}

	// if any of arg is int then we use standart comparison
	if (argType0->type == ArgReg || argType0->type == ArgConst || argType1->type == ArgReg || argType1->type == ArgConst)
	{
		auto proc = ph.pushProc(equalFunc ? Proc_test_eq : Proc_test_le);
		auto argVer = 0;

		if (equalFunc)
		{
			argVer = FuncGroup<Func_test_eq>::parse(ph, std::begin(conditionArgs), std::end(conditionArgs));
		}
		else
		{
			argVer = FuncGroup<Func_test_le>::parse(ph, std::begin(conditionArgs), std::end(conditionArgs));
		}

		if (argVer < 0)
		{
			return false;
		}
		ph.updateProc(proc, argVer);
	}
	else if (equalFunc && ((argType0->type != ArgNone) ? (argType0->type == argType1->type || argType1->type == ArgNone) : (argType1->type != ArgNone)))
	{
		const ScriptRef eq = ScriptRef{ ".eq" };
		const ScriptParserData* proc = nullptr;
		if (argType0->type != ArgNone)
		{
			proc = ph.parser.getProc(ph.parser.getTypeName(argType0->type), eq);
		}
		else if (argType1->type != ArgNone)
		{
			proc = ph.parser.getProc(ph.parser.getTypeName(argType1->type), eq);
		}

		if (!proc || (*proc)(ph, std::begin(conditionArgs), std::end(conditionArgs)) == false)
		{
			Log(LOG_ERROR) << "unsupported operator:  '" + begin[0].toString() + "'";
			return false;
		}
	}
	else
	{
		Log(LOG_ERROR) << "incompatible arguments:  '" + conditionArgs[0].toString() + "' and '" + conditionArgs[1].toString() + "' for '" + begin[0].toString() + "' operator";
		return false;
	}

	ph.setLabel(currPos, ph.getCurrPos());

	return true;
}

static bool parseIf(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
{
	ParserWriter::Block block = { BlockIf, ph.addLabel(), ph.addLabel() };
	ph.codeBlocks.push_back(block);

	return parseConditionImpl(ph, block.nextLabel, begin, end);
}

static bool parseElse(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
{
	if (ph.codeBlocks.empty() || ph.codeBlocks.back().type != BlockIf)
	{
		Log(LOG_ERROR) << "unexpected 'else'";
		return false;
	}

	ParserWriter::Block& block = ph.codeBlocks.back();

	ph.pushProc(Proc_goto);
	ph.pushLabel(block.finalLabel);

	ph.setLabel(block.nextLabel, ph.getCurrPos());
	if (std::distance(begin, end) == 0)
	{
		block.nextLabel = block.finalLabel;
		block.type = BlockElse;
		return true;
	}
	else
	{
		block.nextLabel = ph.addLabel();
		return parseConditionImpl(ph, block.nextLabel, begin, end);
	}
}

static bool parseEnd(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end)
{
	if (ph.codeBlocks.empty())
	{
		Log(LOG_ERROR) << "unexpected 'end'";
		return false;
	}
	if (std::distance(begin, end) != 0)
	{
		Log(LOG_ERROR) << "unexpected symbols after 'end'";
		return false;
	}

	ParserWriter::Block block = ph.codeBlocks.back();
	ph.codeBlocks.pop_back();

	if (block.nextLabel != block.finalLabel)
	{
		ph.setLabel(block.nextLabel, ph.getCurrPos());
	}
	ph.setLabel(block.finalLabel, ph.getCurrPos());
	return true;
}

namespace
{

/**
 * Add new value to sorted vector by name.
 * @param vec Vector with data.
 * @param name Name of new data.
 * @param value Data to add.
 */
template<typename R>
void addSortHelper(std::vector<std::pair<ScriptRef, R>>& vec, ScriptRef name, R value)
{
	vec.push_back(std::make_pair(name, value));
	std::sort(vec.begin(), vec.end(), [](const std::pair<ScriptRef, R>& a, const std::pair<ScriptRef, R>& b) { return a.first < b.first; });
}

/**
 * Helper function finding data by name (that can be merge from two parts).
 * @param vec Vector with values.
 * @param prefix First part of name.
 * @param postfix Second part of name.
 * @return Finded data or null.
 */
template<typename R>
const R* findSortHelper(const std::vector<std::pair<ScriptRef, R>>& vec, ScriptRef prefix, ScriptRef postfix = {})
{
	if (postfix)
	{
		const auto size = prefix.size();
		auto f = std::partition_point(vec.begin(), vec.end(),
			[&](const std::pair<ScriptRef, R>& a)
			{
				const auto sub = a.first.substr(0, size);
				if (sub < prefix)
				{
					return true;
				}
				else if (sub == prefix)
				{
					return a.first.substr(size) < postfix;
				}
				else
				{
					return false;
				}
			}
		);
		if (f != vec.end() && f->first.substr(0, size) == prefix && f->first.substr(size) == postfix)
		{
			return &f->second;
		}
		return nullptr;
	}
	else
	{
		auto f = std::partition_point(vec.begin(), vec.end(), [&](const std::pair<ScriptRef, R>& a){ return a.first < prefix; });
		if (f != vec.end() && f->first == prefix)
		{
			return &f->second;
		}
		return nullptr;
	}
}

}

/**
 * Function extracting token form range
 * @param excepted what token type we expecting now
 * @return extracted token
 */
SelectedToken ScriptRef::getNextToken(TokenEnum excepted)
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
	if (init)
	{
		init = false;
		for(int i = 0; i < 256; ++i)
		{
			if (i == '#' || isspace(i))	charDecoder[i] = none;
			if (i == ':' || i == ';')	charDecoder[i] = spec;

			if (i == '+' || i == '-')	charDecoder[i] = digitSign;
			if (i >= '0' && i <= '9')	charDecoder[i] = digit;
			if (i >= 'A' && i <= 'F')	charDecoder[i] = charHex;
			if (i >= 'a' && i <= 'f')	charDecoder[i] = charHex;

			if (i >= 'G' && i <= 'Z')	charDecoder[i] = charRest;
			if (i >= 'g' && i <= 'z')	charDecoder[i] = charRest;
			if (i == '_' || i == '.')	charDecoder[i] = charRest;
		}
	}

	//find first nowithe character.
	bool coment = false;
	for(; _begin != _end; ++_begin)
	{
		const char c = *_begin;
		if (coment)
		{
			if (c == '\n')
			{
				coment = false;
			}
			continue;
		}
		else if (isspace(c))
		{
			continue;
		}
		else if (c == '#')
		{
			coment = true;
			continue;
		}
		break;
	}
	if (_begin == _end)
	{
		return SelectedToken{ };
	}

	auto type = TokenNone;
	auto begin = _begin;
	bool hex = false;
	int off = 0;

	for (; _begin != _end; ++_begin, ++off)
	{
		const Uint8 c = *_begin;
		const Uint8 decode = charDecoder[c];

		//end of string
		if (decode == none)
		{
			break;
		}
		else if (decode == spec)
		{
			if (c == ':')
			{
				//colon start new token, skip if we are in another one
				if (type != TokenNone)
					break;

				++_begin;
				type = excepted == TokenColon ? TokenColon : TokenInvaild;
				break;
			}
			else if (c == ';')
			{
				//semicolon start new token, skip if we are in another one
				if (type != TokenNone)
					break;
				//semicolon wait for his turn, returning empty token
				if (excepted != TokenSemicolon)
					break;

				++_begin;
				type = TokenSemicolon;
				break;
			}
			else
			{
				type = TokenInvaild;
				break;
			}
		}

		switch (type)
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
				type = TokenNumber;
				continue;

			//start of symbol
			case charHex:
			case charRest:
				type = TokenSymbol;
				continue;
			}
			break;

		//middle of number
		case TokenNumber:
			switch (decode)
			{
			case charRest:
				if (off != 1) break;
				if (c != 'x' && c != 'X') break; //X in "0x1"
			case charHex:
				if (!hex) break;
			case digit:
				if (off == 0) hex = c == '0'; //expecting hex number
				continue;
			}
			break;

		//middle of symbol
		case TokenSymbol:
			switch (decode)
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
		type = TokenInvaild;
		break;
	}
	auto end = _begin;
	return SelectedToken{ type, ScriptRef{ begin, end } };
}

/**
 * Default constructor
 */
ScriptParserBase::ScriptParserBase(const std::string& name) : _regUsed{ RegMax }, _name{ name }
{
	//--------------------------------------------------
	//					op_data init
	//--------------------------------------------------
	#define MACRO_ALL_INIT(NAME, ...) \
		addSortHelper(_procList, addNameRef(#NAME), { &parseLine<MACRO_PROC_ID(NAME), FuncGroup<MACRO_FUNC_ID(NAME)>> });

	MACRO_PROC_DEFINITION(MACRO_ALL_INIT)

	#undef MACRO_ALL_INIT

	addSortHelper(_procList, addNameRef("if"), { &parseIf });
	addSortHelper(_procList, addNameRef("else"), { &parseElse });
	addSortHelper(_procList, addNameRef("end"), { &parseEnd });

	addStandartReg("in0", RegI0);
	addStandartReg("in1", RegI1);
	addStandartReg("reg0", RegR0);
	addStandartReg("reg1", RegR1);
	addStandartReg("reg2", RegR2);
	addStandartReg("reg3", RegR3);

	addType<int>("int");

	auto nullName = addNameRef("null");
	addSortHelper(_typeList, nullName, { ArgNone, 0 });
	addSortHelper(_refList, nullName, { ArgNone, 0, 0 });
}

bool ScriptParserBase::haveNameRef(const std::string& s) const
{
	auto ref = ScriptRef{ s.data(), s.data() + s.size() };
	if (findSortHelper(_refList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_procList, ref) != nullptr)
	{
		return true;
	}
	if (findSortHelper(_typeList, ref) != nullptr)
	{
		return true;
	}
	return false;
}

ScriptRef ScriptParserBase::addNameRef(const std::string& s)
{
	std::vector<char> refData;
	refData.assign(s.begin(), s.end());
	ScriptRef ref{ refData.data(), refData.data() + refData.size() };

	//we need use char vector becasue its guarante that pointer in ref will not get invalidated when names list grown.
	_nameList.push_back(std::move(refData));
	return ref;
}

/**
 * Add new function parsing arguments of script operation.
 * @param s function name
 * @param parser parsing fu
 */
void ScriptParserBase::addParserBase(const std::string& s, ScriptParserData::argFunc arg, ScriptParserData::getFunc get)
{
	if (haveNameRef(s))
	{
		throw Exception("Function name '" + s + "' already used");
	}

	addSortHelper(_procList, addNameRef(s), { &parseCustomFunc, arg, get });
}

void ScriptParserBase::addTypeBase(const std::string& s, ArgEnum type, size_t size)
{
	if (haveNameRef(s))
	{
		throw Exception("Type name '" + s + "' already used");
	}

	addSortHelper(_typeList, addNameRef(s), { type, size });
}

bool ScriptParserBase::haveTypeBase(ArgEnum type)
{
	for (auto& v : _typeList)
	{
		if (v.second.type == type)
		{
			return true;
		}
	}
	return false;
}

void ScriptParserBase::addStandartReg(const std::string& s, RegEnum index)
{
	if (haveNameRef(s))
	{
		throw Exception("Reg name '" + s + "' already used");
	}

	addSortHelper(_refList, addNameRef(s), { ArgReg, index, 0 });
}
/**
 * Set name for custom script param
 * @param i what custom param
 * @param s name for first custom parameter
 */
void ScriptParserBase::addCustomReg(const std::string& s, ArgEnum type, size_t size)
{
	if (_regUsed + size <= ScriptMaxReg)
	{
		if (haveNameRef(s))
		{
			throw Exception("Reg name '" + s + "' already used");
		}

		auto old = _regUsed;
		_regUsed += size;
		addSortHelper(_refList, addNameRef(s), { type, old, 0 });
	}
	else
	{
		throw Exception("Custom arg limit reach for: '" + s + "'");
	}
}

/**
 * Add const value to script
 * @param s name for const
 * @param i value
 */
void ScriptParserBase::addConst(const std::string& s, int i)
{
	if (haveNameRef(s))
	{
		throw Exception("Const name '" + s + "' already used");
	}

	addSortHelper(_refList, addNameRef(s), { ArgConst, 0, i });
}

/**
 * Get name of type
 */
ScriptRef ScriptParserBase::getTypeName(ArgEnum type) const
{
	for (auto& t : _typeList)
	{
		if (t.second.type == type)
		{
			return t.first;
		}
	}
	return ScriptRef{ };
}

/**
 * Get type data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
const ScriptTypeData* ScriptParserBase::getType(ScriptRef prefix, ScriptRef postfix) const
{
	return findSortHelper(_typeList, prefix, postfix);
}

/**
 * Get function data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
const ScriptParserData* ScriptParserBase::getProc(ScriptRef prefix, ScriptRef postfix) const
{
	return findSortHelper(_procList, prefix, postfix);
}

/**
 * Get arguments data with name equal prefix + postfix.
 * @param prefix Beginig of name.
 * @param postfix End of name.
 * @return Pointer to data or null if not find.
 */
const ScriptContainerData* ScriptParserBase::getRef(ScriptRef prefix, ScriptRef postfix) const
{
	return findSortHelper(_refList, prefix, postfix);
}

/**
 * Parse string and write script to ScriptBase
 * @param src struct where final script is write to
 * @param src_code string with script
 * @return true if string have valid script
 */
bool ScriptParserBase::parseBase(ScriptContainerBase* destScript, const std::string& parentName, const std::string& srcCode) const
{
	std::string err = "Parsing script for '" + parentName + "' enconter error: ";
	ParserWriter help(
		_regUsed,
		*destScript,
		*this
	);

	ScriptRef range = ScriptRef{ srcCode.data(), srcCode.data() + srcCode.size() };
	if (!range)
	{
		return false;
	}

	while (true)
	{
		SelectedToken op = range.getNextToken();
		if (!op)
		{
			for (auto i = help.refListCurr.begin(); i != help.refListCurr.end(); ++i)
			{
				if (i->second.type == ArgLabel && i->second.value == -1)
				{
					Log(LOG_ERROR) << err << "invalid use of label: '" << i->first.toString() << "' without declaration";
					return false;
				}
			}
			help.relese();
			return true;
		}

		auto line_begin = op.begin();
		SelectedToken label = { };
		SelectedToken args[ScriptMaxArg];
		args[0] = range.getNextToken(TokenColon);
		if (args[0].getType() == TokenColon)
		{
			std::swap(op, label);
			op = range.getNextToken();
			args[0] = range.getNextToken();
		}

		// change form of "Reg.Function" to "Type.Function Reg".
		auto op_curr = getProc(op);
		if (!op_curr)
		{
			auto first_dot = op.find('.');
			if (first_dot != std::string::npos)
			{
				auto temp = op.substr(0, first_dot);
				auto ref = help.getReferece(temp);
				if (ref && ref->type >= ArgMax)
				{
					auto name = getTypeName(ref->type);
					if (name)
					{
						op_curr = getProc(name, op.substr(first_dot));
						if (!op_curr)
						{
							Log(LOG_ERROR) << err << "invalid operation name '"<< op.toString() <<"'";
							return false;
						}
						args[1] = args[0];
						args[0] = { TokenSymbol, temp };
					}
				}
			}
		}
		for (int i = (args[1] ? 2 : 1); i < ScriptMaxArg; ++i)
			args[i] = range.getNextToken();
		SelectedToken f = range.getNextToken(TokenSemicolon);

		//validation
		bool valid = true;
		valid &= label.getType() == TokenSymbol || label.getType() == TokenNone;
		valid &= op.getType() == TokenSymbol;
		for (int i = 0; i < ScriptMaxArg; ++i)
			valid &= args[i].getType() == TokenSymbol || args[i].getType() == TokenNumber || args[i].getType() == TokenNone;
		valid &= f.getType() == TokenSemicolon;

		if (!valid)
		{
			auto line_end = range.begin();
			if (f.getType() != TokenSemicolon)
			{
				// fixing `line_end` position.
				while(line_end != range.end() && *line_end != ';')
					++line_end;
				if (line_end != range.end())
					++line_end;
			}
			Log(LOG_ERROR) << err << "invalid line: '" << std::string(line_begin, line_end) << "'";
			return false;
		}
		else
		{
			ScriptRef line = ScriptRef{ line_begin, range.begin() };

			// matching args form operation definition with args avaliable in string
			int i = 0;
			while (i < ScriptMaxArg && args[i].getType() != TokenNone)
				++i;

			if (label && !help.setLabel(label, help.getCurrPos()))
			{
				Log(LOG_ERROR) << err << "invalid label '"<< label.toString() <<"' in line: '" << line.toString() << "'";
				return false;
			}

			// create normal operation.
			if (op_curr)
			{
				if ((*op_curr)(help, args, args+i) == false)
				{
					Log(LOG_ERROR) << err << "invalid line: '" << line.toString() << "'";
					return false;
				}

				continue;
			}

			// adding new custom variables of type selected type.
			auto type_curr = getType(op);
			if (type_curr)
			{
				for (int j = 0; j < i; ++j)
				{
					if (args[j].getType() != TokenSymbol || help.addReg(args[j], *type_curr) == false)
					{
						Log(LOG_ERROR) << err << "invalid variable name '"<< args[j].toString() <<"' in line: '" << line.toString() << "'";
						return false;
					}
				}

				continue;
			}

			Log(LOG_ERROR) << err << "invalid line: '" << line.toString() << "'";
			return false;
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
		if (printOp)
		{
			size_t offset = 0;
			printOp = false;
			Logger opLog;
			#define MACRO_STRCAT(...) #__VA_ARGS__
			#define MACRO_ALL_LOG(NAME, Impl, Args) \
				if (#NAME[0] != '_') opLog.get(LOG_DEBUG) \
					<< "Op:    " << std::setw(tabSize*2) << #NAME \
					<< "OpId:  " << std::setw(tabSize/2) << offset << "  + " <<  std::setw(tabSize) << FuncGroup<MACRO_FUNC_ID(NAME)>::ver() \
					<< "Impl:  " << std::setw(tabSize*10) << MACRO_STRCAT(Impl) \
					<< "Args:  " #Args << "\n"; \
				offset += FuncGroup<MACRO_FUNC_ID(NAME)>::ver();

			opLog.get(LOG_DEBUG) << "Available script operations:\n" << std::left << std::hex << std::showbase;
			MACRO_PROC_DEFINITION(MACRO_ALL_LOG)

			#undef MACRO_ALL_LOG
			#undef MACRO_STRCAT
		}

		Logger refLog;
		refLog.get(LOG_DEBUG) << "Script data for: " << _name << "\n" << std::left;
		for (auto ite = _refList.begin(); ite != _refList.end(); ++ite)
		{
			if (ite->second.type == ArgConst)
				refLog.get(LOG_DEBUG) << "Ref: " << std::setw(30) << ite->first.toString() << "Value: " << ite->second.value << "\n";
			else
				refLog.get(LOG_DEBUG) << "Ref: " << std::setw(30) << ite->first.toString() << "Type: " << getTypeName(ite->second.type).toString() << "\n";
		}
	}
}

} //namespace OpenXcom
