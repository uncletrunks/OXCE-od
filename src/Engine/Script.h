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

class ScriptGlobal;
class ScriptParserBase;
class ScriptParserEventsBase;
class ScriptContainerBase;
class ScriptContainerEventsBase;

class ParserWriter;
class SelectedToken;
class ScriptWorkerBase;
class ScriptWorkerBlit;
template<typename...> class ScriptWorker;

namespace helper
{

template<typename T>
struct ArgSelector;

}

constexpr int ScriptMaxArg = 16;
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

public:
	/// Constructor.
	ScriptContainerBase() = default;
	/// Copy constructor.
	ScriptContainerBase(const ScriptContainerBase&) = delete;
	/// Move constructor.
	ScriptContainerBase(ScriptContainerBase&&) = default;

	/// Destructor.
	~ScriptContainerBase() = default;

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
	/// Load data string from YAML.
	void load(const std::string& type, const YAML::Node& node, const Parent& parent)
	{
		parent.parseNode(*this, type, node);
	}
};

/**
 * Common base of typed script with events.
 */
class ScriptContainerEventsBase
{
	friend class ScriptParserEventsBase;
	ScriptContainerBase _current;
	const ScriptContainerBase* _events;

public:
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
	const ScriptContainerBase* dataEvents() const
	{
		return _events;
	}
};

/**
 * Strong typed script with events.
 */
template<typename Parent, typename... Args>
class ScriptContainerEvents : public ScriptContainerEventsBase
{
public:
	/// Load data string form YAML.
	void load(const std::string& type, const YAML::Node& node, const Parent& parent)
	{
		parent.parseNode(*this, type, node);
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
				i0 = executeBase(ptr->data(), i0, i1);
				++ptr;
			}
			++ptr;
		}
		i0 = executeBase(c.data(), i0, i1);
		if (ptr)
		{
			while (*ptr)
			{
				i0 = executeBase(ptr->data(), i0, i1);
				++ptr;
			}
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
		const size_t totalSize = _end - _begin;
		if (p >= totalSize)
		{
			return ScriptRef{ };
		}

		const auto b = _begin + p;
		if (s > totalSize - p)
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

	/// Create temporary ref based on script.
	static ScriptRef tempFrom(const std::string& s)
	{
		return { s.data(), s.data() + s.size() };
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

	/// Copy constructor.
	template<typename T>
	inline ScriptValueData(const T& t);
	/// Copy constructor.
	inline ScriptValueData(const ScriptValueData& t);
	/// Default constructor.
	inline ScriptValueData() { }

	/// Assign operator.
	template<typename T>
	inline ScriptValueData& operator=(const T& t);
	/// Assign operator.
	inline ScriptValueData& operator=(const ScriptValueData& t);

	/// Get current stored value.
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
	ScriptGlobal* _shared;
	Uint8 _regUsed;
	std::string _name;
	std::string _defaultScript;
	std::vector<std::vector<char>> _strings;
	std::vector<ScriptTypeData> _typeList;
	std::vector<ScriptProcData> _procList;
	std::vector<ScriptRefData> _refList;

protected:
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
	ScriptParserBase(ScriptGlobal* shared, const std::string& name, const std::string& firstArg, const std::string& secondArg);
	/// Destructor.
	~ScriptParserBase();

	/// Common typeless part of parsing string.
	bool parseBase(ScriptContainerBase& scr, const std::string& parentName, const std::string& code) const;

	/// Prase string and return new script.
	void parseNode(ScriptContainerBase& container, const std::string& type, const YAML::Node& node) const;

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

	/// Load global data from YAML.
	virtual void load(const YAML::Node& node);

	/// Show all script informations.
	void logScriptMetadata(bool haveEvents) const;

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
	/// Get script shared data.
	ScriptGlobal* getGlobal() { return _shared; }
	/// Get script shared data.
	const ScriptGlobal* getGlobal() const { return _shared; }
};

/**
 * Copy constructor from pod type.
 */
template<typename T>
inline ScriptValueData::ScriptValueData(const T& t)
{
	static_assert(sizeof(T) <= sizeof(data), "Value have too big size!");
	type = ScriptParserBase::getArgType<T>();
	size = sizeof(T);
	memcpy(&data, &t, sizeof(T));
}

/**
 * Copy constructor.
 */
inline ScriptValueData::ScriptValueData(const ScriptValueData& t)
{
	*this = t;
}

/**
 * Assign operator from pod type.
 */
template<typename T>
inline ScriptValueData& ScriptValueData::operator=(const T& t)
{
	*this = ScriptValueData{ t };
	return *this;
}

/**
 * Assign operator.
 */
inline ScriptValueData& ScriptValueData::operator=(const ScriptValueData& t)
{
	type = t.type;
	size = t.size;
	memcpy(&data, &t.data, sizeof(data));
	return *this;
}

/**
 * Get current stored value.
 */
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
public:
	using Container = ScriptContainer<ScriptParser, Args...>;
	using Worker = ScriptWorker<Args...>;
	friend Container;

	/// Constructor.
	ScriptParser(ScriptGlobal* shared, const std::string& name, const std::string& firstArg, const std::string& secondArg, S<Args>... argNames) : ScriptParserBase(shared, name, firstArg, secondArg)
	{
		addRegImpl(argNames...);
	}
};

/**
 * Common base for strong typed event parser.
 */
class ScriptParserEventsBase : public ScriptParserBase
{
	constexpr static size_t EventsMax = 64;
	constexpr static size_t OffsetScale = 100;
	constexpr static size_t OffsetMax = 100 * OffsetScale;

	struct EventData
	{
		int offset;
		ScriptContainerBase script;
	};

	/// Final list of events.
	std::vector<ScriptContainerBase> _events;
	/// Meta data of events.
	std::vector<EventData> _eventsData;

protected:
	/// Prase string and return new script.
	void parseNode(ScriptContainerEventsBase& container, const std::string& type, const YAML::Node& node) const;

public:
	/// Constructor.
	ScriptParserEventsBase(ScriptGlobal* shared, const std::string& name, const std::string& firstArg, const std::string& secondArg);

	/// Load global data from YAML.
	virtual void load(const YAML::Node& node) override;
	/// Get pointer to events.
	const ScriptContainerBase* getEvents() const;
	/// Relese event data.
	std::vector<ScriptContainerBase> releseEvents();
};

/**
 * Strong typed event parser.
 */
template<typename... Args>
class ScriptParserEvents : public ScriptParserEventsBase
{
public:
	using Container = ScriptContainerEvents<ScriptParserEvents, Args...>;
	using Worker = ScriptWorker<Args...>;
	friend Container;

	/// Constructor.
	ScriptParserEvents(ScriptGlobal* shared, const std::string& name, const std::string& firstArg, const std::string& secondArg, S<Args>... argNames) : ScriptParserEventsBase(shared, name, firstArg, secondArg)
	{
		addRegImpl(argNames...);
	}
};

/**
 * Strong typed tag.
 */
template<typename T, typename I>
struct ScriptTag
{
	static_assert(!std::numeric_limits<I>::is_signed, "Type should be unsigned");
	static_assert(sizeof(I) <= sizeof(size_t), "Type need be smaller than size_t");

	using Parent = T;

	/// Index that identify value in ScriptValues.
	I index;

	/// Get value.
	constexpr size_t get() const { return static_cast<size_t>(index); }
	/// Test if tag have valid value.
	constexpr explicit operator bool() const { return this->index; }

	/// Get run time value for type.
	static ArgEnum type() { return ScriptParserBase::getArgType<ScriptTag<T, I>>(); }
	/// Test if value can be used.
	static constexpr bool isValid(size_t i) { return i && i <= limit(); }
	/// Fake constructor.
	static constexpr ScriptTag make(size_t i) { return { static_cast<I>(i) }; }
	/// Max supprted value.
	static constexpr size_t limit() { return static_cast<size_t>(std::numeric_limits<I>::max()); }
};

/**
 * Global data shared by all scripts.
 */
class ScriptGlobal
{
protected:
	using LoadFunc = void (*)(const ScriptGlobal*, int&, const YAML::Node&);
	using SaveFunc = void (*)(const ScriptGlobal*, const int&, YAML::Node&);
	using CrateFunc = ScriptValueData (*)(size_t i);

	friend class ScriptValuesBase;

	struct TagValueType
	{
		ScriptRef name;
		LoadFunc load;
		SaveFunc save;
	};
	struct TagValueData
	{
		ScriptRef name;
		size_t valueType;
	};
	struct TagData
	{
		ScriptRef name;
		size_t limit;
		CrateFunc crate;
		std::vector<TagValueData> values;
	};

	template<typename ThisType, void (ThisType::* LoadValue)(int&, const YAML::Node&) const>
	static void loadHelper(const ScriptGlobal* base, int& value, const YAML::Node& node)
	{
		(static_cast<const ThisType*>(base)->*LoadValue)(value, node);
	}
	template<typename ThisType, void (ThisType::* SaveValue)(const int&, YAML::Node&) const>
	static void saveHelper(const ScriptGlobal* base, const int& value, YAML::Node& node)
	{
		(static_cast<const ThisType*>(base)->*SaveValue)(value, node);
	}

	void addTagValueTypeBase(const std::string& name, LoadFunc loadFunc, SaveFunc saveFunc)
	{
		_tagValueTypes.push_back(TagValueType{ addNameRef(name), loadFunc, saveFunc });
	}
	template<typename ThisType, void (ThisType::* LoadValue)(int&, const YAML::Node&) const, void (ThisType::* SaveValue)(const int&, YAML::Node&) const>
	void addTagValueType(const std::string& name)
	{
		static_assert(std::is_base_of<ScriptGlobal, ThisType>::value, "Type must be derived");
		addTagValueTypeBase(name, &loadHelper<ThisType, LoadValue>, &saveHelper<ThisType, SaveValue>);
	}
private:
	std::vector<std::vector<char>> _strings;
	std::vector<std::vector<ScriptContainerBase>> _events;
	std::map<std::string, ScriptParserBase*> _parserNames;
	std::vector<ScriptParserEventsBase*> _parserEvents;
	std::map<ArgEnum, TagData> _tagNames;
	std::vector<TagValueType> _tagValueTypes;
	std::vector<ScriptRefData> _refList;

	/// Get tag value.
	size_t getTag(ArgEnum type, ScriptRef s) const;
	/// Get data of tag value.
	TagValueData getTagValueData(ArgEnum type, size_t i) const;
	/// Get tag value type.
	TagValueType getTagValueType(size_t valueType) const;
	/// Add new tag name.
	size_t addTag(ArgEnum type, ScriptRef s, size_t valueType);
	/// Add new name ref.
	ScriptRef addNameRef(const std::string& s);

public:
	/// Default constructor.
	ScriptGlobal();
	/// Destructor.
	virtual ~ScriptGlobal();

	/// Store parser.
	void pushParser(ScriptParserBase* parser);
	/// Store parser.
	void pushParser(ScriptParserEventsBase* parser);

	/// Add new const value.
	void addConst(const std::string& name, ScriptValueData i);
	/// Update const value.
	void updateConst(const std::string& name, ScriptValueData i);

	/// Get global ref data.
	const ScriptRefData* getRef(ScriptRef name, ScriptRef postfix = {}) const;

	/// Get tag based on it name.
	template<typename Tag>
	Tag getTag(ScriptRef s) const
	{
		return Tag::make(getTag(Tag::type(), s));
	}
	/// Get tag based on it name.
	template<typename Tag>
	Tag getTag(const std::string& s) const
	{
		return getTag<Tag>(ScriptRef::tempFrom(s));
	}
	/// Add new tag name.
	template<typename Tag>
	Tag addTag(const std::string& s, size_t valueType)
	{
		return Tag::make(addTag(Tag::type(), addNameRef(s), valueType));
	}
	/// Add new type of tag.
	template<typename Tag>
	void addTagType()
	{
		_tagNames.insert(
			std::make_pair(
				Tag::type(),
				TagData
				{
					ScriptRef{ Tag::Parent::ScriptName },
					Tag::limit(),
					[](size_t i) { return ScriptValueData{ Tag::make(i) }; },
					std::vector<TagValueData>{},
				}
			)
		);
	}

	/// Prepare for loading data.
	virtual void beginLoad();
	/// Finishing loading data.
	virtual void endLoad();

	/// Load global data from YAML.
	void load(const YAML::Node& node);
};

/**
 * Colection of values for script usage.
 */
class ScriptValuesBase
{
	/// Vector with all avaiable values for script.
	std::vector<int> values;

protected:
	/// Set value.
	void setBase(size_t t, int i);
	/// Get value.
	int getBase(size_t t) const;
	/// Load values from yaml file.
	void loadBase(const YAML::Node &node, const ScriptGlobal* shared, ArgEnum type);
	/// Save values to yaml file.
	void saveBase(YAML::Node &node, const ScriptGlobal* shared, ArgEnum type) const;
};

/**
 * Strong typed colection of values for srcipt.
 */
template<typename T, typename I = Uint8>
class ScriptValues : ScriptValuesBase
{
public:
	using Tag = ScriptTag<T, I>;

	/// Load values from yaml file.
	void load(const YAML::Node &node, const ScriptGlobal* shared)
	{
		loadBase(node, shared, Tag::type());
	}
	/// Save values to yaml file.
	void save(YAML::Node &node, const ScriptGlobal* shared) const
	{
		saveBase(node, shared, Tag::type());;
	}

	/// Get value.
	int get(Tag t) const
	{
		return getBase(t.get());
	}
	/// Set value.
	void set(Tag t, int i)
	{
		return setBase(t.get(), i);
	}
};

} //namespace OpenXcom

#endif	/* OPENXCOM_SCRIPT_H */

