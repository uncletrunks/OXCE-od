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
#ifndef OPENXCOM_SCRIPT_H
#define	OPENXCOM_SCRIPT_H

#include <map>
#include <limits>
#include <vector>
#include <string>
#include <yaml-cpp/yaml.h>
#include <SDL_stdinc.h>

#include "Logger.h"
#include "Exception.h"


namespace OpenXcom
{
class Surface;
class ScriptParserBase;
class ScriptContainerBase;

class ParserWriter;
struct SelectedToken;
class ScriptWorkerBase;
class ScriptWorkerBlit;
template<typename...> class ScriptWorker;

namespace helper
{

template<typename T>
struct ArgSelector;

}

constexpr int ScriptMaxArg = 8;
constexpr int ScriptMaxReg = 64*sizeof(void*);

/**
 * Script execution cunter.
 */
enum class ProgPos : size_t
{
	Unknown = (size_t)-1,
	Start = 0,
};

inline ProgPos& operator+=(ProgPos& pos, int offset)
{
	pos = static_cast<ProgPos>(static_cast<size_t>(pos) + offset);
	return pos;
}
inline ProgPos& operator++(ProgPos& pos)
{
	pos += 1;
	return pos;
}
inline ProgPos operator++(ProgPos& pos, int)
{
	ProgPos old = pos;
	++pos;
	return old;
}

/**
 * Args special types.
 */
enum ArgSpecEnum : Uint8
{
	ArgSpecNone = 0x0,
	ArgSpecReg = 0x1,
	ArgSpecVar = 0x3,
	ArgSpecPtr = 0x4,
	ArgSpecPtrE = 0xC,
	ArgSpecSize = 0x10,
};
constexpr ArgSpecEnum operator|(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<Uint8>(a) | static_cast<Uint8>(b));
}
constexpr ArgSpecEnum operator&(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<Uint8>(a) & static_cast<Uint8>(b));
}
constexpr ArgSpecEnum operator^(ArgSpecEnum a, ArgSpecEnum b)
{
	return static_cast<ArgSpecEnum>(static_cast<Uint8>(a) ^ static_cast<Uint8>(b));
}
/**
 * Args types.
 */
enum ArgEnum : Uint8
{
	ArgInvalid = ArgSpecSize * 0,

	ArgNull = ArgSpecSize * 1,
	ArgInt = ArgSpecSize * 2,
	ArgLabel = ArgSpecSize * 3,
	ArgMax = ArgSpecSize * 4,
};

/**
 * Next avaiable value for arg type.
 */
constexpr ArgEnum ArgNext(ArgEnum arg)
{
	return static_cast<ArgEnum>(static_cast<Uint8>(arg) + static_cast<Uint8>(ArgSpecSize));
}

/**
 * Base version of argument type.
 */
constexpr ArgEnum ArgBase(ArgEnum arg)
{
	return static_cast<ArgEnum>((static_cast<Uint8>(arg) & ~(static_cast<Uint8>(ArgSpecSize) - 1)));
}
/**
 * Specialized version of argument type.
 */
constexpr ArgEnum ArgSpec(ArgEnum arg, ArgSpecEnum spec)
{
	return ArgBase(arg) != ArgInvalid ? static_cast<ArgEnum>(static_cast<Uint8>(arg) | static_cast<Uint8>(spec)) : arg;
}
/**
 * Specialized version of argument type.
 */
constexpr ArgEnum ArgRemove(ArgEnum arg, ArgSpecEnum spec)
{
	return ArgBase(arg) != ArgInvalid ? static_cast<ArgEnum>(static_cast<Uint8>(arg) & ~static_cast<Uint8>(spec)) : arg;
}
/**
 * Test if argument is normal type or special.
 * @param arg
 * @return True if type is normal.
 */
constexpr bool ArgIsNormal(ArgEnum arg)
{
	return ArgBase(arg) != ArgInvalid;
}
/**
 * Test if argumet type is register.
 */
constexpr bool ArgIsReg(ArgEnum arg)
{
	return (static_cast<Uint8>(arg) & static_cast<Uint8>(ArgSpecReg)) == static_cast<Uint8>(ArgSpecReg);
}
/**
 * Test if argumet type is register.
 */
constexpr bool ArgIsVar(ArgEnum arg)
{
	return (static_cast<Uint8>(arg) & static_cast<Uint8>(ArgSpecVar)) == static_cast<Uint8>(ArgSpecVar);
}
/**
 * Test if argumet type is pointer.
 */
constexpr bool ArgIsPtr(ArgEnum arg)
{
	return (static_cast<Uint8>(arg) & static_cast<Uint8>(ArgSpecPtr)) == static_cast<Uint8>(ArgSpecPtr);
}
/**
 * Test if argumet type is editable pointer.
 */
constexpr bool ArgIsPtrE(ArgEnum arg)
{
	return (static_cast<Uint8>(arg) & static_cast<Uint8>(ArgSpecPtrE)) == static_cast<Uint8>(ArgSpecPtrE);
}
/**
 * Compatibility betwean operation argument type and reg type. Greater numbers mean bigger comatibility.
 * @param argType Type of operation argument.
 * @param regType Type of reg we try pass to operation.
 * @return Zero if incompatible, 255 if both types are same.
 */
constexpr int ArgCompatible(ArgEnum argType, ArgEnum regType, size_t overloadSize)
{
	return
		argType == ArgInvalid ? 0 :
		ArgIsVar(argType) && argType != regType ? 0 :
		ArgBase(argType) != ArgBase(regType) ? 0 :
		ArgIsReg(argType) != ArgIsReg(regType) ? 0 :
		ArgIsPtr(argType) != ArgIsPtr(regType) ? 0 :
			255 - (ArgIsPtrE(argType) != ArgIsPtrE(regType) ? 128 : 0) - (ArgIsVar(argType) != ArgIsVar(regType) ? 64 : 0) - (overloadSize > 8 ? 8 : overloadSize);
}

/**
 * Avaiable regs.
 */
enum RegEnum : Uint8
{
	RegInvaild = (Uint8)-1,

	RegI0 = 0*sizeof(int),
	RegI1 = 1*sizeof(int),

	RegMax = 2*sizeof(int),
};

/**
 * Return value from script operation.
 */
enum RetEnum : Uint8
{
	RetContinue = 0,
	RetEnd = 1,
	RetError = 2,
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

using FuncCommon = RetEnum (*)(ScriptWorkerBase &, const Uint8 *, ProgPos &);

/**
 * Common base of script execution.
 */
class ScriptContainerBase
{
	friend class ParserWriter;
	std::vector<Uint8> _proc;

protected:
	/// Protected destructor.
	~ScriptContainerBase() { }

public:
	/// Constructor.
	ScriptContainerBase() = default;
	/// Copy constructor.
	ScriptContainerBase(const ScriptContainerBase&) = delete;
	/// Move constructor.
	ScriptContainerBase(ScriptContainerBase&&) = default;

	/// Copy.
	ScriptContainerBase &operator=(const ScriptContainerBase&) = delete;
	/// Move.
	ScriptContainerBase &operator=(ScriptContainerBase&&) = default;

	/// Test if is any script there.
	explicit operator bool() const
	{
		return !_proc.empty();
	}

	/// Get pointer to proc data.
	const Uint8* data() const
	{
		return *this ? _proc.data() : nullptr;
	}
};

/**
 * Strong typed script.
 */
template<typename Parent, typename... Args>
class ScriptContainer : public ScriptContainerBase
{
public:
	/// Load data string form YAML.
	void load(const std::string& type, const YAML::Node& node, const Parent& parent)
	{
		if(const YAML::Node& scripts = node["scripts"])
		{
			if (const YAML::Node& curr = scripts[parent.getName()])
			{
				*this = parent.parse(type, curr.as<std::string>());
			}
		}
		if (!*this && !parent.getDefault().empty())
		{
			*this = parent.parse(type, parent.getDefault());
		}
	}
};

/**
 * Global version of strong typed script.
 */
template<typename Parent, typename... Args>
class ScriptContainerEvents
{
	using Contanier = ScriptContainer<Parent, Args...>;
	Contanier _current;
	Contanier* _events;

public:
	/// Load data string form YAML.
	void load(const std::string& type, const YAML::Node& node, const Parent& parent)
	{
		_current.load(type, node, parent);
		_events = parent.getEvents();
	}

	/// Test if is any script there.
	explicit operator bool() const
	{
		return true;
	}

	/// Get pointer to proc data.
	const Uint8* data() const
	{
		return _current.data();
	}
	/// Get pointer to proc data.
	const Contanier* dataEvents() const
	{
		return _events;
	}
};

template<int size>
using ScriptRawMemory = typename std::aligned_storage<size, alignof(void*)>::type;

/**
 * Class that cache state of script data and is place of script write temporary data.
 */
class ScriptWorkerBase
{
	ScriptRawMemory<ScriptMaxReg> reg;

	template<size_t offset, typename First, typename... Rest>
	void setReg(First f, Rest... t)
	{
		ref<First>(offset) = f;
		setReg<offset + sizeof(First), Rest...>(t...);
	}
	template<size_t offset>
	void setReg()
	{
		//end loop
	}

protected:
	/// Update values in script.
	template<typename... Args>
	void updateBase(Args... args)
	{
		memset(&reg, 0, ScriptMaxReg);
		setReg<RegMax>(args...);
	}
	/// Call script with two arguments.
	int executeBase(const Uint8* proc, int i0, int i1);

public:
	/// Default constructor.
	ScriptWorkerBase() //reg not initialized
	{

	}

	/// Get value from reg.
	template<typename T>
	T& ref(size_t offset)
	{
		return *reinterpret_cast<typename std::decay<T>::type*>(reinterpret_cast<char*>(&reg) + offset);
	}
	/// Get value from proc vector.
	template<typename T>
	const T& const_val(const Uint8 *ptr, size_t offset = 0)
	{
		return *reinterpret_cast<const T*>(ptr + offset);
	}
};

template<typename... Args>
class ScriptWorker : public ScriptWorkerBase
{
public:

	/// Default constructor.
	ScriptWorker(Args... args) : ScriptWorkerBase()
	{
		updateBase(args...);
	}

	/// Execute standard script.
	template<typename Parent>
	int execute(const ScriptContainer<Parent, Args...>& c, int i0, int i1)
	{
		return executeBase(c.data(), i0, i1);
	}

	/// Execute standard script with global events.
	template<typename Parent>
	int execute(const ScriptContainerEvents<Parent, Args...>& c, int i0, int i1)
	{
		auto ptr = c.dataEvents();
		if (ptr)
		{
			while (*ptr)
			{
				i0 = execute(*ptr, i0, i1);
				++ptr;
			}
			++ptr;
		}
		i0 = executeBase(c.data(), i0, i1);
		if (ptr)
		{
			while (*ptr)
			{
				i0 = execute(*ptr, i0, i1);
				++ptr;
			}
			++ptr;
		}
		return i0;
	}
};

class ScriptWorkerBlit : public ScriptWorkerBase
{
	/// Current script set in worker.
	const Uint8* _proc;
public:

	/// Default constructor.
	ScriptWorkerBlit() : ScriptWorkerBase(), _proc(nullptr)
	{

	}

	/// Update data from container script.
	template<typename Parent, typename... Args>
	void update(const ScriptContainer<Parent, Args...>& c, typename std::decay<Args>::type... args)
	{
		if (c)
		{
			_proc = c.data();
			updateBase(args...);
		}
		else
		{
			clear();
		}
	}

	/// Programmable bliting using script.
	void executeBlit(Surface* src, Surface* dest, int x, int y, int shade, bool half = false);

	/// Clear all worker data.
	void clear()
	{
		_proc = nullptr;
	}
};

template<typename T>
class ScriptRange
{
protected:
	using ptr = const T*;

	/// Pointer pointing place of first elemet.
	ptr _begin;
	/// pointer pointing place past of last elemet.
	ptr _end;

public:
	/// Defualt constructor.
	ScriptRange() : _begin{ nullptr }, _end{ nullptr }
	{

	}
	/// Constructor.
	ScriptRange(ptr b, ptr e) : _begin{ b }, _end{ e }
	{

	}

	/// Begining of string range.
	ptr begin() const
	{
		return _begin;
	}
	/// End of string range.
	ptr end() const
	{
		return _end;
	}
	/// Size of string range.
	size_t size() const
	{
		return _end - _begin;
	}
	/// Bool operator.
	explicit operator bool() const
	{
		return _begin != _end;
	}
};

class ScriptRef : public ScriptRange<char>
{
public:
	/// Default constructor.
	ScriptRef() : ScriptRange{ }
	{

	}

	/// Copy constructor.
	ScriptRef(const ScriptRef&) = default;

	/// Constructor from pointer.
	explicit ScriptRef(ptr p) : ScriptRange{ p , p + strlen(p) }
	{

	}
	/// Constructor from range of pointers.
	ScriptRef(ptr b, ptr e) : ScriptRange{ b, e }
	{

	}

	/// Extract new token from current object.
	SelectedToken getNextToken(TokenEnum excepted = TokenNone);

	/// Find first orrucace of character in string range.
	size_t find(char c) const
	{
		for (auto &curr : *this)
		{
			if (curr == c)
			{
				return &curr - _begin;
			}
		}
		return std::string::npos;
	}

	/// Return sub range of current range.
	ScriptRef substr(size_t p, size_t s = std::string::npos) const
	{
		auto b = _begin + p;
		if (b > _end)
		{
			return ScriptRef{ };
		}
		else if (s == std::string::npos || b + s > _end)
		{
			return ScriptRef{ b, _end };
		}
		else
		{
			return ScriptRef{ b, b + s };
		}
	}

	/// Create string based on current range.
	std::string toString() const
	{
		return *this ? std::string(_begin, _end) : std::string{ };
	}

	/// Compare two ranges.
	static int compare(ScriptRef a, ScriptRef b)
	{
		const auto size_a = a.size();
		const auto size_b = b.size();
		if (size_a == size_b)
		{
			return memcmp(a._begin, b._begin, size_a);
		}
		else if (size_a < size_b)
		{
			return -1;
		}
		else
		{
			return 1;
		}
	}
	/// Equal operator.
	bool operator==(const ScriptRef& s) const
	{
		return compare(*this, s) == 0;
	}
	/// Less operator.
	bool operator<(const ScriptRef& s) const
	{
		return compare(*this, s) < 0;
	}
};

/**
 * Struct storing storing type data.
 */
struct ScriptTypeData
{
	ScriptRef name;
	ArgEnum type;
	size_t size;
};

/**
 * Struct storing value used by string.
 */
struct ScriptValueData
{
	ScriptRawMemory<sizeof(void*)> data;
	ArgEnum type = ArgInvalid;
	Uint8 size = 0;

	template<typename T>
	inline ScriptValueData(const T& t);
	inline ScriptValueData() { }

	template<typename T>
	inline ScriptValueData& operator=(const T& t);

	template<typename T>
	inline const T& getValue() const;
};

/**
 * Struct used to store named definition used by script.
 */
struct ScriptRefData
{
	ScriptRef name;
	ArgEnum type;
	ScriptValueData value;

	explicit operator bool() const
	{
		return name.size() > 0;
	}
};

/**
 * Struct storing avaliable operation to scripts.
 */
struct ScriptProcData
{
	using argFunc = int (*)(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end);
	using getFunc = FuncCommon (*)(int version);
	using parserFunc = bool (*)(const ScriptProcData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end);
	using overloadFunc = int (*)(const ScriptProcData& spd, const ScriptRefData* begin, const ScriptRefData* end);

	ScriptRef name;

	overloadFunc overload;
	ScriptRange<ScriptRange<ArgEnum>> overloadArg;

	parserFunc parser;
	argFunc parserArg;
	getFunc parserGet;

	bool operator()(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end) const
	{
		return parser(*this, ph, begin, end);
	}
};

/**
 * Common base of script parser.
 */
class ScriptParserBase
{
	Uint8 _regUsed;
	std::string _name;
	std::string _defaultScript;
	std::vector<std::vector<char>> _nameList;
	std::vector<ScriptTypeData> _typeList;
	std::vector<ScriptProcData> _procList;
	std::vector<ScriptRefData> _refList;

protected:
	static ArgEnum registeTypeImplNextValue()
	{
		static ArgEnum curr = ArgMax;
		ArgEnum old = curr;
		curr = ArgNext(curr);
		return old;
	}
	template<typename T>
	static ArgEnum registeTypeImpl()
	{
		if (std::is_same<T, int>::value)
		{
			return ArgInt;
		}
		else if (std::is_same<T, std::nullptr_t>::value)
		{
			return ArgNull;
		}
		else
		{
			static ArgEnum curr = registeTypeImplNextValue();
			return curr;
		}
	}
	template<typename T>
	struct TypeInfoImpl
	{
		using t1 = typename std::decay<T>::type;
		using t2 = typename std::remove_pointer<t1>::type;
		using t3 = typename std::decay<t2>::type;

		static constexpr bool isRef = std::is_reference<T>::value;
		static constexpr bool isPtr = std::is_pointer<t1>::value;
		static constexpr bool isEditable = isPtr && !std::is_const<t2>::value;

		static constexpr size_t size = std::is_pod<t3>::value ? sizeof(t3) : 0;

		static_assert(size || isPtr, "Type need to be POD to be used as reg or const value.");
	};

	/// Default constructor.
	ScriptParserBase(const std::string& name, const std::string& firstArg, const std::string& secondArg);
	/// Destructor.
	~ScriptParserBase();

	/// Common typeless part of parsing string.
	bool parseBase(ScriptContainerBase* scr, const std::string& parentName, const std::string& code) const;

	/// Test if name is free.
	bool haveNameRef(const std::string& s) const;
	/// Add new name that can be used in data lists.
	ScriptRef addNameRef(const std::string& s);

	/// Add name for standart reg.
	void addStandartReg(const std::string& s, RegEnum index);
	/// Add name for custom parameter.
	void addScriptArg(const std::string& s, ArgEnum type);
	/// Add parsing fuction.
	void addParserBase(const std::string& s, ScriptProcData::overloadFunc overload, ScriptRange<ScriptRange<ArgEnum>> overloadArg, ScriptProcData::parserFunc parser, ScriptProcData::argFunc parserArg, ScriptProcData::getFunc parserGet);
	/// Add new type impl.
	void addTypeBase(const std::string& s, ArgEnum type, size_t size);
	/// Test if type was added impl.
	bool haveTypeBase(ArgEnum type);
	/// Set defulat script for type.
	void setDefault(const std::string& s) { _defaultScript = s; }

public:
	/// Register type to get run time value representing it.
	template<typename T>
	static ArgEnum getArgType()
	{
		using info = TypeInfoImpl<T>;
		using t3 = typename info::t3;

		auto spec = ArgSpecNone;

		if (info::isRef) spec = spec | ArgSpecVar;
		if (info::isPtr) spec = spec | ArgSpecPtr;
		if (info::isEditable) spec = spec | ArgSpecPtrE;

		return ArgSpec(registeTypeImpl<t3>(), spec);
	}
	/// Add const value.
	void addConst(const std::string& s, ScriptValueData i);
	/// Update const value.
	void updateConst(const std::string& s, ScriptValueData i);
	/// Add line parsing function.
	template<typename T>
	void addParser(const std::string& s)
	{
		addParserBase(s, nullptr, T::overloadType(), nullptr, &T::parse, &T::getDynamic);
	}
	/// Test if type was already added.
	template<typename T>
	bool haveType()
	{
		return haveTypeBase(getArgType<T>());
	}
	/// Add new type.
	template<typename T>
	void addType(const std::string& s)
	{
		using info = TypeInfoImpl<T>;
		using t3 = typename info::t3;

		addTypeBase(s, registeTypeImpl<t3>(), info::size);
	}
	/// Regised type in parser.
	template<typename P>
	void registerPointerType()
	{
		if (!haveType<P*>())
		{
			addType<P*>(P::ScriptName);
			P::ScriptRegister(this);
		}
	}

	/// Show all script informations.
	void logScriptMetadata() const;

	/// Get name of script.
	const std::string& getName() const { return _name; }
	/// Get defulat script.
	const std::string& getDefault() const { return _defaultScript; }

	/// Get name of type.
	ScriptRef getTypeName(ArgEnum type) const;
	/// Get full name of type.
	std::string getTypePrefix(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ScriptRef name, ScriptRef postfix = {}) const;
	/// Get function data.
	ScriptRange<ScriptProcData> getProc(ScriptRef name, ScriptRef postfix = {}) const;
	/// Get arguments data.
	const ScriptRefData* getRef(ScriptRef name, ScriptRef postfix = {}) const;
};

template<typename T>
inline ScriptValueData::ScriptValueData(const T& t)
{
	static_assert(sizeof(T) <= sizeof(data) || sizeof(T) > 255, "Value have too big size!");
	type = ScriptParserBase::getArgType<T>();
	size = sizeof(T);
	memcpy(&data, &t, size);
}

template<typename T>
inline ScriptValueData& ScriptValueData::operator=(const T& t)
{
	static_assert(sizeof(T) <= sizeof(data) || sizeof(T) > 255, "Value have too big size!");
	type = ScriptParserBase::getArgType<T>();
	size = sizeof(T);
	memcpy(&data, &t, size);
	return *this;
}

template<typename T>
inline const T& ScriptValueData::getValue() const
{
	if (type != ScriptParserBase::getArgType<T>())
	{
		throw Exception("Invalid cast of value");
	}
	return *reinterpret_cast<const T*>(&data);
}

/**
 * Strong typed parser.
 */
template<typename... Args>
class ScriptParser : public ScriptParserBase
{
	template<typename Z>
	struct S
	{
		S(const char *n) : name{ n } { }
		S(const std::string& n) : name{ n.data() } { }

		const char *name;
	};

	template<typename First, typename... Rest>
	void addRegImpl(S<First>& n, Rest&... t)
	{
		addTypeImpl(n);
		addScriptArg(n.name, ScriptParserBase::getArgType<First>());
		addRegImpl(t...);
	}
	void addRegImpl()
	{
		//end loop
	}

	template<typename First, typename = decltype(&First::ScriptRegister)>
	void addTypeImpl(S<First*>& n)
	{
		registerPointerType<First>();
	}
	template<typename First, typename = decltype(&First::ScriptRegister)>
	void addTypeImpl(S<const First*>& n)
	{
		registerPointerType<First>();
	}
	template<typename First>
	void addTypeImpl(S<First>& n)
	{
		//nothing to do for rest
	}

public:
	using Container = ScriptContainer<ScriptParser, Args...>;
	using Worker = ScriptWorker<Args...>;

	/// Default constructor.
	ScriptParser(const std::string& name, const std::string& firstArg, const std::string& secondArg, S<Args>... argNames) : ScriptParserBase(name, firstArg, secondArg)
	{
		addRegImpl(argNames...);
	}

	/// Prase string and return new script.
	Container parse(const std::string& parentName, const std::string& srcCode) const
	{
		auto scr = Container{};
		if (parseBase(&scr, parentName, srcCode))
		{
			return std::move(scr);
		}
		return {};
	}
};


/**
 * Strong typed tag.
 */
template<typename T, typename I>
struct ScriptTag
{
	static_assert(!std::numeric_limits<I>::is_signed, "Type should be unsigned");
	static_assert(sizeof(I) <= sizeof(size_t), "Type need be less than size_t");

	/// Index that identify value in ScriptValues.
	I index;

	/// Get value or max value if invalid.
	constexpr size_t get() const { return *this ? static_cast<size_t>(index) : std::numeric_limits<size_t>::max(); }
	/// Test if tag have valid value.
	constexpr explicit operator bool() const { return this->index != invalid().index; }

	/// Get run time value for type.
	static ArgEnum type() { return ScriptParserBase::getArgType<T*>(); }
	/// Test if value can be used.
	static constexpr bool isValid(size_t i) { return i < static_cast<size_t>(invalid().index); }
	/// Fake constructor.
	static constexpr ScriptTag make(size_t i) { return { static_cast<I>(i) }; }
	/// Invaild tag.
	static constexpr ScriptTag invalid() { return { std::numeric_limits<I>::max() }; }
};

/**
 * Colection of values for script usage.
 */
class ScriptValuesBase
{
protected:
	/// Vector with all avaiable values for script.
	std::vector<int> values;

	/// Get map that store all tag names for all types.
	static std::map<ArgEnum, std::vector<std::string>> &getAll()
	{
		static std::map<ArgEnum, std::vector<std::string>> all;
		return all;
	}

public:
	/// Clean all tag names.
	static void unregisteAll()
	{
		for (auto& n : getAll())
		{
			n.second.clear();
		}
	}
};

/**
 * Strong typed colection of values for srcipt.
 */
template<typename T, typename I = Uint8>
class ScriptValues : ScriptValuesBase
{
	static std::vector<std::string> &nameVector()
	{
		static std::vector<std::string> &name = getAll()[Tag::type()];
		return name;
	}

public:
	using Tag = ScriptTag<T, I>;

	/// Get tag based on name for that type.
	static Tag getTag(const std::string& s)
	{
		for (size_t i = 0; i < nameVector().size(); ++i)
		{
			auto &name = nameVector()[i];
			if (name == s)
			{
				return Tag::make(i);
			}
		}
		return Tag::invalid();
	}
	/// Add new tag name for that type.
	static Tag addTag(const std::string& s)
	{
		auto tag = getTag(s);

		if (!tag)
		{
			// test to prevent warp of index value
			if (Tag::isValid(nameVector().size()))
			{
				nameVector().push_back(s);
				return Tag::make(nameVector().size() - 1u);
			}
			return Tag::invalid();
		}
		else
		{
			return tag;
		}
	}
	/// Regist avaiable tag names for that type.
	static void registerNames(const YAML::Node &parent, const std::string &reference)
	{
		const YAML::Node &node = parent[reference];
		if (node && node.IsMap())
		{
			for (YAML::const_iterator i = node.begin(); i != node.end(); ++i)
			{
				auto type = i->second.as<std::string>();
				auto name = i->first.as<std::string>();
				if (type == "int")
				{
					auto tag = addTag(name);
					if (!tag)
					{
						Log(LOG_ERROR) << "Script variable '" + name + "' exceeds limit of " << (int)tag.index << " avaiable variables in '" + reference + "'.";
						return;
					}
				}
				else
				{
					Log(LOG_ERROR) << "Invaild type def '" + type + "' for script variable '" + name + "' in '" + reference +"'.";
				}
			}
		}
	}

	/// Load values from yaml file.
	void load(const YAML::Node &node)
	{
		if (node && node.IsMap())
		{
			for (size_t i = 0; i < nameVector().size(); ++i)
			{
				if (const YAML::Node &n = node[nameVector()[i]])
				{
					set(Tag::make(i), n.as<int>());
				}
			}
		}
	}
	/// Save values to yaml file.
	YAML::Node save() const
	{
		YAML::Node node;
		for (size_t i = 0; i < nameVector().size(); ++i)
		{
			if (int v = get(Tag::make(i)))
			{
				node[nameVector()[i]] = v;
			}
		}
		return node;
	}

	/// Get value.
	int get(Tag t) const
	{
		if (t)
		{
			if (t.index < values.size())
			{
				return values[t.get()];
			}
		}
		return 0;
	}
	/// Set value.
	void set(Tag t, int i)
	{
		if (t)
		{
			if (t.get() >= values.size())
			{
				values.resize(t.get() + 1u);
			}
			values[t.get()] = i;
		}
	}
};

} //namespace OpenXcom

#endif	/* OPENXCOM_SCRIPT_H */

