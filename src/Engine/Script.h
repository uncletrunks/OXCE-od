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


namespace OpenXcom
{
class Surface;
class ScriptParserBase;
class ScriptContainerBase;

class ParserWriter;
struct SelectedToken;

template<typename T>
struct ArgSelector;

constexpr int ScriptMaxArg = 8;
constexpr int ScriptMaxReg = 256;

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

constexpr size_t ArgTypeReg = 0x1;
constexpr size_t ArgTypePtr = 0x2;
constexpr size_t ArgTypeEdit = 0x4;
constexpr size_t ArgTypeSize = 0x8;
/**
 * Args types.
 */
enum ArgEnum : Uint8
{
	ArgInvalid = ArgTypeSize * 0,
	ArgAny = ArgTypeSize * 0 + 1,

	ArgNull = ArgTypeSize * 1,
	ArgInt = ArgTypeSize * 2,
	ArgLabel = ArgTypeSize * 3,
	ArgMax = ArgTypeSize * 4,
};

constexpr ArgEnum ArgNext(ArgEnum arg)
{
	return static_cast<ArgEnum>(static_cast<Uint8>(arg) + ArgTypeSize);
}

/**
 * Base version of argument type.
 */
constexpr ArgEnum ArgBase(ArgEnum arg)
{
	return static_cast<ArgEnum>((static_cast<Uint8>(arg) | (ArgTypeSize - 1)) - ArgTypeSize + 1);
}
/**
 * Specialized version of argument type.
 */
constexpr ArgEnum ArgSpec(ArgEnum arg, bool reg, bool ptr = false, bool edit = false)
{
	return static_cast<ArgEnum>(static_cast<Uint8>(arg) | (reg ? ArgTypeReg : 0x0) | (ptr ? ArgTypePtr : 0x0) | (edit ? ArgTypeEdit : 0x0));
}
/**
 * Test if argumet type is register.
 */
constexpr bool ArgIsReg(ArgEnum arg)
{
	return static_cast<Uint8>(arg) & ArgTypeReg;
}
/**
 * Test if argumet type is pointer.
 */
constexpr bool ArgIsPtr(ArgEnum arg)
{
	return static_cast<Uint8>(arg) & ArgTypePtr;
}
/**
 * Test if argumet type is editable pointer.
 */
constexpr bool ArgIsEditable(ArgEnum arg)
{
	return static_cast<Uint8>(arg) & ArgTypeEdit;
}

/**
 * Avaiable regs.
 */
enum RegEnum : Uint8
{
	RegInvaild = (Uint8)-1,
	RegI0 = 0*sizeof(int),
	RegI1 = 1*sizeof(int),
	RegCond = 2*sizeof(int),

	RegMax = 3*sizeof(int),
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

/**
 * Struct that cache state of script data and is place of script write temporary data.
 */
struct ScriptWorker
{
	const Uint8* proc;
	typename std::aligned_storage<ScriptMaxReg, alignof(void*)>::type reg;

	/// Default constructor.
	ScriptWorker() : proc(nullptr) //reg not initialized
	{

	}

	/// Programmable bliting using script.
	void executeBlit(Surface* src, Surface* dest, int x, int y, int shade, bool half = false);
	/// Call script with two arguments.
	int execute(int i0, int i1);
	/// Clear all worker data.
	void clear()
	{
		proc = nullptr;
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

using FuncCommon = RetEnum (*)(ScriptWorker &, const Uint8 *, ProgPos &);

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

	void updateBase(ScriptWorker* ref) const
	{
		memset(&ref->reg, 0, ScriptMaxReg);
		if (*this)
		{
			ref->proc = this->_proc.data();
		}
		else
		{
			ref->proc = 0;
		}
	}

public:
	/// constructor.
	ScriptContainerBase() = default;
	/// copy constructor.
	ScriptContainerBase(const ScriptContainerBase&) = delete;
	/// move constructor.
	ScriptContainerBase(ScriptContainerBase&&) = default;

	/// copy.
	ScriptContainerBase &operator=(const ScriptContainerBase&) = delete;
	/// move.
	ScriptContainerBase &operator=(ScriptContainerBase&&) = default;

	/// Test if is any script there.
	explicit operator bool() const
	{
		return !_proc.empty();
	}
};

/**
 * Strong typed script.
 */
template<typename Parent, typename... Args>
class ScriptContainer : public ScriptContainerBase
{
	template<size_t offset, typename First, typename... Rest>
	void setReg(ScriptWorker* ref, First f, Rest... t) const
	{
		ref->ref<First>(offset) = f;
		setReg<offset + sizeof(First), Rest...>(ref, t...);
	}
	template<size_t offset>
	void setReg(ScriptWorker* ref) const
	{
		//end loop
	}

public:
	/// Update values in script.
	void update(ScriptWorker* ref, Args... args) const
	{
		updateBase(ref);
		if (*this)
		{
			setReg<RegMax>(ref, args...);
		}
	}

	/// Load data string form YAML.
	void load(const std::string& type, const YAML::Node& node, const Parent& parent, const std::string& def = "")
	{
		if(node)
		{
			*this = parent.parse(type, node.as<std::string>());
		}
		if (!*this && !def.empty())
		{
			*this = parent.parse(type, def);
		}
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
 * Struct storing avaliable operation to scripts.
 */
struct ScriptParserData
{
	using argFunc = int (*)(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end);
	using getFunc = FuncCommon (*)(int version);
	using parserFunc = bool (*)(const ScriptParserData& spd, ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end);
	using displayFunc = std::string (*)(const ScriptParserBase* spb);

	ScriptRef name;
	ArgEnum firstArgType;

	parserFunc parser;
	argFunc arg;
	getFunc get;
	displayFunc display;

	bool operator()(ParserWriter& ph, const SelectedToken* begin, const SelectedToken* end) const
	{
		return parser(*this, ph, begin, end);
	}
};

/**
 * Struct used to store definition of used data by script.
 */
struct ScriptContainerData
{
	ScriptRef name;
	ArgEnum type;
	RegEnum index;

	int value;

	explicit operator bool() const
	{
		return name.size() > 0;
	}
};

/**
 * Common base of script parser.
 */
class ScriptParserBase
{
	Uint8 _regUsed;
	std::string _name;
	std::vector<std::vector<char>> _nameList;
	std::vector<ScriptTypeData> _typeList;
	std::vector<ScriptParserData> _procList;
	std::vector<ScriptContainerData> _refList;

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

	/// Common typeless part of parsing string.
	bool parseBase(ScriptContainerBase* scr, const std::string& parentName, const std::string& code) const;

	/// Test if name is free.
	bool haveNameRef(const std::string& s) const;
	/// Add new name that can be used in data lists.
	ScriptRef addNameRef(const std::string& s);

	/// Add name for standart reg.
	void addStandartReg(const std::string& s, RegEnum index);
	/// Add name for custom parameter.
	void addCustomReg(const std::string& s, ArgEnum type);
	/// Add parsing fuction.
	void addParserBase(const std::string& s, ArgEnum firstArgType, ScriptParserData::argFunc arg, ScriptParserData::getFunc get, ScriptParserData::displayFunc display);
	/// Add new type impl.
	void addTypeBase(const std::string& s, ArgEnum type, size_t size);
	/// Test if type was added impl.
	bool haveTypeBase(ArgEnum type);

public:
	/// Register type to get run time value representing it.
	template<typename T>
	static ArgEnum getArgType()
	{
		using info = TypeInfoImpl<T>;
		using t3 = typename info::t3;

		return ArgSpec(registeTypeImpl<t3>(), info::isRef, info::isPtr, info::isEditable);
	}
	/// Add const value.
	void addConst(const std::string& s, int i);
	/// Add line parsing function.
	template<typename T>
	void addParser(const std::string& s)
	{
		addParserBase(s, T::overloadType(), &T::parse, &T::getDynamic, &T::displayArgs);
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

	/// Get name of type.
	ScriptRef getTypeName(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ArgEnum type) const;
	/// Get type data.
	const ScriptTypeData* getType(ScriptRef name, ScriptRef postfix = {}) const;
	/// Get function data.
	ScriptRange<ScriptParserData> getProc(ScriptRef name, ScriptRef postfix = {}) const;
	/// Get arguments data.
	const ScriptContainerData* getRef(ScriptRef name, ScriptRef postfix = {}) const;
};

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
		addCustomReg(n.name, ScriptParserBase::getArgType<First>());
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

	/// Default constructor.
	ScriptParser(const std::string& name, const std::string& firstArg, const std::string& secondArg,  S<Args>... argNames) : ScriptParserBase(name, firstArg, secondArg)
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

