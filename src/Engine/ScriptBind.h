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

typedef std::string::const_iterator ite;

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
	TokenBuildinLabel,
};

/**
 * Struct represents position of token in input string
 */
struct SelectedToken
{
	///type of this token
	TokenEnum type;
	///iterator pointing place of first character
	ite begin;
	///iterator pointing place past of last character
	ite end;

	///custom value
	int tag;

	std::string toString() const { return std::string(begin, end); }
};

/**
 * Helper structure used by ScriptParser::parse
 */
struct ParserHelper
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
		int _index;
		ReservedPos(ProgPos pos, int index) : _pos{ pos }, _index{ index }
		{

		}
		ProgPos getPos()
		{
			return _pos;
		}
		int getIndex()
		{
			return _index;
		}

		friend class ParserHelper;
	};

	class ProcOp { };

	///member pointer accessing script operations
	std::vector<Uint8>& proc;
	///all available operations for script
	const std::map<std::string, ScriptParserData>& procList;
	///all available data for script
	const std::map<std::string, ScriptContainerData>& refList;
	///temporary script data
	std::map<std::string, ScriptContainerData> refListCurr;
	///list of variables uses
	std::vector<ReservedPos<ProgPos>> refLabelsUses;
	///list of variables uses
	std::vector<ProgPos> refLabelsList;

	///index of used script registers
	Uint8 regIndexUsed;
	///negative index of used const values
	int constIndexUsed;

	///Store position of blocks of code like "if" or "while"
	std::vector<Block> codeBlocks;

	/**
	 * Constructor
	 * @param scr parsed script
	 * @param proc member pointer accessing script operations
	 * @param procRefData member pointer accessing script data
	 * @param procList all available operations for script
	 * @param ref all available data for script
	 */
	ParserHelper(
			Uint8 regUsed,
			std::vector<Uint8>& proc,
			const std::map<std::string, ScriptParserData>& procList,
			const std::map<std::string, ScriptContainerData>& ref):
		proc(proc),
		procList(procList), refList(ref),
		refListCurr(),
		refLabelsUses(),
		refLabelsList(),
		regIndexUsed(regUsed),
		constIndexUsed(-1)
	{

	}

	/**
	 * Function finalizing parsing of script
	 */
	void relese()
	{
		pushProc(0);
		for (auto& p : refLabelsUses)
		{
			updateReserved<ProgPos>(p, refLabelsList[p.getIndex()]);
		}
	}

	/**
	 * Returns reference based on name.
	 * @param s name of referece.
	 * @return referece data.
	 */
	const ScriptContainerData* getReferece(const std::string& s) const
	{
		auto pos = refListCurr.find(s);
		if (pos == refListCurr.end())
		{
			pos = refList.find(s);
			if (pos == refList.end())
			{
				return nullptr;
			}
		}
		return &pos->second;
	}

	/**
	 * Add new referece.
	 * @param s Name of referece.
	 * @param data Data of reference.
	 * @return pointer to new created reference.
	 */
	ScriptContainerData* addReferece(const std::string& s, const ScriptContainerData& data)
	{
		return &refListCurr.insert(std::make_pair(s, data)).first->second;
	}

	/**
	 * Get current position in proc vector.
	 * @return Position in proc vector.
	 */
	ProgPos getCurrPos() const
	{
		return static_cast<ProgPos>(proc.size());
	}

	/**
	 * Get distance betwean two positions in proc vector.
	 */
	size_t getDiffPos(ProgPos begin, ProgPos end) const
	{
		if (begin > end)
		{
			throw Exception("Invaild ProgPos distance");
		}
		return static_cast<size_t>(end) - static_cast<size_t>(begin);
	}

	/**
	 * Preparing place and position on proc vector for some value and return position of it.
	 */
	template<typename T>
	ReservedPos<T> pushReserved(int index = -1)
	{
		auto curr = getCurrPos();
		proc.insert(proc.end(), sizeof(T), 0);
		return { curr, index };
	}

	/**
	 * Setting previosly prepared place with value.
	 */
	template<typename T>
	void updateReserved(ReservedPos<T> pos, T value)
	{
		memcpy(&proc[static_cast<size_t>(pos.getPos())], &value, sizeof(T));
	}

	/**
	 * Pushing proc operation id on proc vector.
	 */
	ReservedPos<ProcOp> pushProc(Uint8 procId)
	{
		auto curr = getCurrPos();
		proc.push_back(procId);
		return { curr, -1 };
	}

	/**
	 * Updating previosoly added proc operation id.
	 * @param pos Position of operation.
	 * @param procOffset Offset value.
	 */
	void updateProc(ReservedPos<ProcOp> pos, int procOffset)
	{
		proc[static_cast<size_t>(pos.getPos())] += procOffset;
	}

	/**
	 * Try pushing label arg on proc vector.
	 * @param s name of label.
	 * @return true if label was succefuly added.
	 */
	bool pushLabelTry(const std::string& s)
	{
		const ScriptContainerData* pos = getReferece(s);
		if (pos == nullptr)
		{
			pos = addReferece(s, { ArgLabel, -1, addLabel() });
		}
		if (pos->type != ArgLabel && refLabelsList[pos->value] != ProgPos::Unknown)
			return false;

		pushLabel(pos->value);
		return true;
	}

	void pushLabel(int index)
	{
		refLabelsUses.push_back(pushReserved<ProgPos>(index));
	}
	/**
	 * Create new label for proc vector.
	 * @return index of new label.
	 */
	int addLabel()
	{
		int pos = refLabelsList.size();
		refLabelsList.push_back(ProgPos::Unknown);
		return pos;
	}

	/**
	 * Setting offset of label on proc vector.
	 * @param s name of label
	 * @param offset set value where label is pointing
	 * @return true if operation success
	 */
	bool setLabel(const std::string& s, ProgPos offset)
	{
		const ScriptContainerData* pos = getReferece(s);
		if (pos == nullptr)
		{
			pos = addReferece(s, { ArgLabel, -1, addLabel() });
		}
		if (pos->type != ArgLabel || refLabelsList[pos->value] != ProgPos::Unknown)
			return false;

		return setLabel(pos->value, offset);
	}

	/**
	 * Setting offset of label on proc vector.
	 * @param index index of label.
	 * @param offset set value where label is pointing
	 * @return true if operation success
	 */
	bool setLabel(int index, ProgPos offset)
	{
		if (refLabelsList[index] != ProgPos::Unknown)
			return false;
		refLabelsList[index] = offset;
		return true;
	}

	/**
	 * Try pushing data arg on proc vector.
	 * @param s name of data
	 * @return true if data exists and is valid
	 */
	bool pushDataTry(const std::string& s)
	{
		const ScriptContainerData* pos = getReferece(s);
		if (pos && pos->type == ArgConst)
		{
			updateReserved<int>(pushReserved<int>(), pos->value);
			return true;
		}
		return false;
	}

	/**
	 * Try pushing const arg on proc vector.
	 * @param s name of const
	 * @return true if const exists and is valid
	 */
	bool pushConstTry(const std::string& s)
	{
		int value = 0;
		size_t offset = 0;
		std::stringstream ss(s);
		if (s[0] == '-' || s[0] == '+')
			offset = 1;
		if (s.size() > 2 + offset && s[offset] == '0' && (s[offset + 1] == 'x' || s[offset + 1] == 'X'))
			ss >> std::hex;
		if (!(ss >> value))
			return false;

		updateReserved<int>(pushReserved<int>(), value);
		return true;
	}

	/**
	 * Try pushing reg arg on proc vector.
	 * @param s name of reg
	 * @return true if reg exists and is valid
	 */
	template<typename T>
	bool pushRegTry(const std::string& s)
	{
		const ScriptContainerData* pos = getReferece(s);
		if (pos && pos->type == ScriptContainerData::Register<T>())
		{
			proc.push_back(pos->index);
			return true;
		}
		return false;
	}

	/**
	 * Add new reg arg.
	 * @param s name of reg
	 * @return true if reg exists and is valid
	 */
	template<typename T>
	bool addReg(const std::string& s)
	{
		return addReg(s, std::make_pair(ScriptContainerData::Register<T>(), sizeof(T)));
	}

	/**
	 * Add new reg arg.
	 * @param s name of reg
	 * @param type type of reg
	 * @return true if reg exists and is valid
	 */
	bool addReg(const std::string& s, std::pair<ArgEnum, size_t> type)
	{
		const ScriptContainerData* pos = getReferece(s);
		if (pos != nullptr)
		{
			return false;
		}
		if (regIndexUsed + type.second > ScriptMaxReg)
		{
			return false;
		}
		ScriptContainerData data = { type.first, regIndexUsed, 0 };
		regIndexUsed += type.second;
		refListCurr.insert(std::make_pair(s, data));
		return true;
	}

	/**
	 * Function finding next token in string
	 * @param pos current position in string
	 * @param end end of string
	 * @return true if pos pointing on next token
	 */
	bool findToken(ite& pos, const ite& end) const
	{
		bool coment = false;
		for(; pos != end; ++pos)
		{
			const char c = *pos;
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
	SelectedToken getToken(ite& pos, const ite& end, TokenEnum excepted = TokenNone) const
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

		SelectedToken token = { TokenNone, end, end };
		if (!findToken(pos, end))
			return token;

		token.begin = pos;
		bool hex = false;
		int off = 0;

		for (; pos != end; ++pos, ++off)
		{
			const Uint8 c = *pos;
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
					if (token.type != TokenNone)
						break;

					++pos;
					token.type = excepted == TokenColon ? TokenColon : TokenInvaild;
					break;
				}
				else if (c == ';')
				{
					//semicolon start new token, skip if we are in another one
					if (token.type != TokenNone)
						break;
					//semicolon wait for his turn, returning empty token
					if (excepted != TokenSemicolon)
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

			switch (token.type)
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
			token.type = TokenInvaild;
			break;
		}
		token.end = pos;
		return token;
	}
}; //struct ParserHelper

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

	static int parse(ParserHelper &ph, const SelectedToken &t)
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

	static int parse(ParserHelper &ph, const SelectedToken &begin)
	{
		return -1;
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
	static int parse(ParserHelper &ph, const SelectedToken *begin, const SelectedToken *end)
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
	static int parse(ParserHelper &ph, const SelectedToken *begin, const SelectedToken *end)
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
	static void typeFunc() = delete;
};

////////////////////////////////////////////////////////////
//						Arguments impl
////////////////////////////////////////////////////////////

struct ArgContextDef
{
	using ReturnType = ScriptWorker&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return sw;
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		return false;
	}
};

struct ArgProgDef
{
	using ReturnType = ProgPos&;
	static constexpr size_t size = 0;
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return curr;
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		return false;
	}
};

struct ArgRegDef
{
	using ReturnType = int&;
	static constexpr size_t size = sizeof(Uint8);
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return sw.ref<int>(*arg);
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		if (t.type == TokenSymbol)
		{
			return ph.pushRegTry<int>(t.toString());
		}
		return false;
	}
};

struct ArgConstDef
{
	using ReturnType = int;
	static constexpr size_t size = sizeof(int);
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return *reinterpret_cast<const int*>(arg);
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		if (t.type == TokenNumber)
		{
			return ph.pushConstTry(t.toString());
		}
		else if (t.type == TokenSymbol)
		{
			return ph.pushDataTry(t.toString());
		}
		return false;
	}
};

struct ArgLabelDef
{
	using ReturnType = ProgPos;
	static constexpr size_t size = sizeof(ReturnType);
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return *reinterpret_cast<const ProgPos*>(arg);
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		if (t.type == TokenSymbol)
		{
			return ph.pushLabelTry(t.toString());
		}
		else if (t.type == TokenBuildinLabel)
		{
			ph.pushLabel(t.tag);
			return true;
		}
		return false;
	}
};

struct ArgFuncDef
{
	using ReturnType = FuncCommon;
	static constexpr size_t size = sizeof(ReturnType);
	static FuncCommon get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return *reinterpret_cast<const FuncCommon*>(arg);
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		return false;
	}
};

template<int Size>
struct ArgRawDef
{
	using ReturnType = const Uint8*;
	static constexpr size_t size = Size;
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return arg;
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		return false;
	}
};

template<typename T>
struct ArgCustomPointerDef
{
	using ponter = T*;

	using ReturnType = ponter&;
	static constexpr size_t size = sizeof(Uint8);
	static ReturnType get(ScriptWorker &sw, const Uint8 *arg, ProgPos &curr)
	{
		return sw.ref<ponter>(*arg);
	}

	static bool parse(ParserHelper &ph, const SelectedToken &t)
	{
		if (t.type == TokenSymbol)
		{
			return ph.pushRegTry<ponter>(t.toString());
		}
		return false;
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
	using type = Arg<false, ArgRegDef>;
};

template<>
struct ArgSelector<int>
{
	using type = Arg<false, ArgConstDef, ArgRegDef>;
};

template<>
struct ArgSelector<const int&>
{
	using type = Arg<false, ArgConstDef, ArgRegDef>;
};

template<>
struct ArgSelector<ProgPos&>
{
	using type = Arg<true, ArgProgDef>;
};

template<>
struct ArgSelector<const ProgPos&>
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
	using type = Arg<false, ArgCustomPointerDef<T>>;
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
	static typename GetTypeAt<CurrPos>::ReturnType get(ScriptWorker &sw, const Uint8 *procArgs, ProgPos &curr)
	{
		constexpr int offset = Args::offset(Ver, CurrPos);
		return GetTypeAt<CurrPos>::get(sw, procArgs + offset, curr);
	}

	[[gnu::always_inline]]
	static RetEnum func(ScriptWorker &sw, const Uint8 *procArgs, ProgPos &curr)
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

	static constexpr FuncCommon getDynamic(int i) { return FuncList::getDynamic(i); }
};

////////////////////////////////////////////////////////////
//						Bind class
////////////////////////////////////////////////////////////

namespace helper
{

template<typename T, typename P, P T::*X>
struct BindPropGet
{
	static RetEnum func(T* t, P& p)
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
template<typename T, typename N, typename P, N T::*X, P N::*XX>
struct BindPropNestGet
{
	static RetEnum func(T* t, P& p)
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
	static RetEnum func(T* p, int& r, Args... a)
	{
		if (p) r = (p->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, int(T::*X)(Args...)>
struct BindFunc<int(T::*)(Args...), X>
{
	static RetEnum func(T* p, int& r, Args... a)
	{
		if (p) r = (p->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, bool(T::*X)(Args...) const>
struct BindFunc<bool(T::*)(Args...) const, X>
{
	static RetEnum func(T* p, int& r, Args... a)
	{
		if (p) r = (p->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, int(T::*X)(Args...) const>
struct BindFunc<int(T::*)(Args...) const, X>
{
	static RetEnum func(T* p, int& r, Args... a)
	{
		if (p) r = (p->*X)(std::forward<Args>(a)...); else r = 0;
		return RetContinue;
	}
};

template<typename T, typename... Args, void(T::*X)(Args...)>
struct BindFunc<void(T::*)(Args...), X>
{
	static RetEnum func(T* p, Args... a)
	{
		if (p) (p->*X)(std::forward<Args>(a)...);
		return RetContinue;
	}
};

} //namespace helper

template<typename T>
struct Bind
{
	ScriptParserBase *parser;
	std::string name;

	Bind(ScriptParserBase *p, const std::string& n) : parser{ p }, name{ n }
	{
		parser->addType<T*>(n);
	}

	std::string getName(const std::string& s)
	{
		return name + "." + s;
	}

	template<int T::*X>
	void addField(const std::string& get, const std::string& set = "")
	{
		parser->addParser<FuncGroup<helper::BindPropGet<T, int, X>>>(getName(get));
		if (!set.empty())
		{
			parser->addParser<FuncGroup<helper::BindPropSet<T, int, X>>>(getName(set));
		}
	}
	template<int X>
	void addFake(const std::string& get)
	{
		parser->addParser<FuncGroup<helper::BindValue<T, int, X>>>(getName(get));
	}

	#define MACRO_COPY_HELP_FUNC(Type) \
		template<Type> \
		void add(const std::string& func) \
		{ \
			parser->addParser<FuncGroup<helper::BindFunc<decltype(X), X>>>(getName(func)); \
		}

	MACRO_COPY_HELP_FUNC(bool (T::*X)() const)
	MACRO_COPY_HELP_FUNC(int (T::*X)() const)
	MACRO_COPY_HELP_FUNC(int (T::*X)())
	MACRO_COPY_HELP_FUNC(int (T::*X)(int) const)
	MACRO_COPY_HELP_FUNC(void (T::*X)(int))
	MACRO_COPY_HELP_FUNC(void (*X)(T*, int))
	MACRO_COPY_HELP_FUNC(void (*X)(T*, int&))
	MACRO_COPY_HELP_FUNC(void (*X)(T*, int&, int))

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
		bind->parser->template addParser<FuncGroup<helper::BindPropNestGet<T, N, int, X, XX>>>(bind->getName(get));
		if (!set.empty())
		{
			bind->parser->template addParser<FuncGroup<helper::BindPropNestSet<T, N, int, X, XX>>>(bind->getName(get));
		}
	}
};

} //namespace OpenXcom

#endif	/* OPENXCOM_SCRIPTBIND_H */

