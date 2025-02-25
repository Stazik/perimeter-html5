#ifndef __ENUM_WRAPPER_H__
#define __ENUM_WRAPPER_H__

#include <map>
#include <string>
#include <typeinfo>
#include "Handle.h"
#include "SerializationMacro.h"
#include "xutl.h"

extern unsigned int crc32(const unsigned char *address, unsigned int size, unsigned int crc);

//Does serialization of T into OArchive format and computes CRC of the output buffer
template<class OArchive, class T>
static uint32_t getSerializationCRC(T& t, uint32_t crc, int floatDigits = 0) {
    OArchive ar;
    ar.binary_friendly = true;
    XBuffer& buf = ar.buffer();
    if (floatDigits) {
        buf.SetDigits(floatDigits);
    }
    ar << makeObjectWrapper(t, 0, 0);
    crc = crc32(reinterpret_cast<const unsigned char*>(buf.address()), buf.tell(), crc);
    return crc;
}

//Previously typeid(CLASS_T).name() was used which is not portable, so we attempt to ignore the extra struct/class
static void extract_type_name(std::string& name) {
    if (startsWith(name, "struct ")) {
        name.erase(0, 7);
    } else if (startsWith(name, "class ")) {
        name.erase(0, 6);
    }
}

//Workaround for keeping MSVC outputted typeid format that is written into saves and who knows where else
static void process_type_name(std::string& name) {
    if (!startsWith(name, "struct ") && !startsWith(name, "class ")) {
        if (startsWith(name, "Attribute")) {
            name = "class " + name;
        } else {
            name = "struct " + name;
        }
    }
}

//We use ptr to specialize the type_id when only pointer is know at compile time (like "this")
template<class CLASS_T>
static std::string get_type_id(const CLASS_T* ptr = nullptr) {
    if (ptr) {}
	std::string name = ptr->type_class_name();
    process_type_name(name);
    //printf("get_type_id %s = %s\n", typeid(CLASS_T).name(), name.c_str());
    return name;
}

//Uses SERIALIZE_TYPE_NAME methods for getting runtime type name for class/structs
template<class CLASS_T>
static std::string get_type_id_runtime(const CLASS_T* ptr) {
    std::string name = ptr->type_name();
    process_type_name(name);
    //printf("get_type_id_runtime %s = %s\n", typeid(CLASS_T).name(), name.c_str());
    return name;
}

/////////////////////////////////////////////////
enum ArchiveType 
{
	ARCHIVE_TEXT = 1,
	ARCHIVE_EDIT = 2,
	ARCHIVE_BINARY = 4
};

/////////////////////////////////////////////////
#ifdef PERIMETER_DEBUG_ASSERT
#define xassertStr(exp, str) { std::string s = #exp; s += "\n"; s += str; xxassert(exp,s.c_str()); }
#else
#define xassertStr(exp, str) 
#endif

/////////////////////////////////////////////////
//	Обертка для всех объектов при архивировании
//  name == 0 - игнорируется обертывание полностью
//  nameAlt == 0 - не выводится в edit-архивах
/////////////////////////////////////////////////
template<class T>
class ObjectWrapper 
{
public:
	ObjectWrapper(T& t, const char* name, const char* nameAlt) :
	value_(&t), name_(name), nameAlt_(nameAlt)
    {}

	T& value() const {
        return *value_;
    }

	const char* name() const {
        return name_;
    }

	const char* nameAlt() const {
        return nameAlt_;
    }

private:
	T* value_;
	const char* name_;
	const char* nameAlt_;
};

template<class T>
inline const ObjectWrapper<T> makeObjectWrapper(const T& t, const char* name, const char* nameAlt){
    return ObjectWrapper<T>(const_cast<T&>(t), name, nameAlt);
}

/////////////////////////////////////////////////
// Traits для убирания скобок в текстовых архивах
/////////////////////////////////////////////////
template<class T>
struct SuppressBracket
{
	enum { value = false };
};

/////////////////////////////////////////////////

/////////////////////////////////////////////////
template<class T>
class ComboListDescriptor
{
public:
	ComboListDescriptor(const char* type_name) : typeName_(type_name) { add(0,""); }

	T objectByName(const char* name) const {
		typename NameToObject::const_iterator it = nameToObject_.find(name);
		if(it != nameToObject_.end())
			return it -> second;

		xassertStr(!strlen(name) && "Unknown object identificator will be ignored: ", (typeName_ + "::" + name).c_str());
		return T(0);
	}

	const char* name(const T& object) const {
		typename ObjectToName::const_iterator it = objectToName_.find(object);
		if(it != objectToName_.end())
			return it -> second.c_str();

		xassert(0 && "Unregistered combo list value object");
		return 0;
	}

	const char* comboList() const { return comboList_.c_str(); }

	const char* typeName() const { return typeName_.c_str(); }
	const char* defaultValue() const { xassert(!nameToObject_.empty()); return nameToObject_.begin()->first.c_str(); }

protected:
	void add(T object, const char* name) {
		nameToObject_.insert(NameToObject::value_type(name,object));
		objectToName_.insert(ObjectToName::value_type(object,name));

		if(!comboList_.empty())
			comboList_ += "|";

		comboList_ += name;
	}

private:
	typedef std::map<std::string, T> NameToObject;
	typedef std::map<T, std::string> ObjectToName;

	NameToObject nameToObject_;
	ObjectToName objectToName_;

	std::string comboList_;
	std::string typeName_;
};

/////////////////////////////////////////////////
//		Map: enum <-> name, nameAlt
/////////////////////////////////////////////////
template<class Enum>
class EnumDescriptor
{
public:
	EnumDescriptor(const char* typeName) : typeName_(typeName) {}

	const char* name(Enum key) const {
		typename KeyToName::const_iterator i = keyToName.find(key);
		if(i != keyToName.end())
			return i->second.c_str();
		xassert(0 && "Unregistered enum key");
		return "";
	}

	const char* nameAlt(Enum key) const {
		typename KeyToName::const_iterator i = keyToNameAlt.find(key);
		if(i != keyToNameAlt.end())
			return i->second.c_str();
		xassert(0 && "Unregistered enum key");
		return "";
	}

	Enum keyByName(const char* name) const {
		NameToKey::const_iterator i = nameToKey.find(name);
		if(i != nameToKey.end())
			return (Enum)i->second;
		//xassertStr(!strlen(name) && "Unknown enum identificator will be ignored: ", (typeName_ + "::" + name).c_str());
		return (Enum)0;
	}

	Enum keyByNameAlt(const char* nameAlt) const {
		NameToKey::const_iterator i = nameAltToKey.find(nameAlt);
		if(i != nameAltToKey.end())
			return (Enum)i->second;
		//xassertStr(!strlen(nameAlt) && "Unknown enum identificator will be ignored: ", (typeName_ + "::" + nameAlt).c_str());
		return (Enum)0;
	}

    template<typename BV>
	std::string nameCombination(BV bitVector) const {
		std::string value;
		for(typename KeyToName::const_iterator i = keyToName.begin(); i != keyToName.end(); ++i)
			if((bitVector & i->first) == i->first){
				bitVector &= ~i->first;
				if(!value.empty())
					value += " | ";
				value += i->second;
			}
		return value;
	}

	std::string nameAltCombination(int bitVector) const {
		std::string value;
		for(typename KeyToName::const_iterator i = keyToNameAlt.begin(); i != keyToNameAlt.end(); ++i)
			if((bitVector & i->first) == i->first){
				bitVector &= ~i->first;
				if(!value.empty())
					value += " | ";
				value += i->second;
			}
		return value;
	}

    template<typename BV>
    std::vector<Enum> keyCombination(BV bitVector) const {
        std::vector<Enum> keys;
        for(typename KeyToName::const_iterator i = keyToName.begin(); i != keyToName.end(); ++i) {
            if ((bitVector & i->first) == i->first) {
                bitVector &= ~i->first;
                keys.emplace_back(static_cast<Enum>(i->first.value()));
            }
        }
        return keys;
    }

    std::vector<Enum> keysFromNameCombination(const std::string& str) const {
        std::vector<Enum> vec;
        std::string buf;
        for (size_t i = 0; i < str.length(); ++i) {
            const char c = str[i];
            if (c == ' ') continue;
            if (c == '|') {
                Enum val = keyByName(buf.c_str());
                vec.emplace_back(val);
                buf.clear();
                continue;
            }
            buf += c;
        }
        if (!buf.empty()) {
            Enum val = keyByName(buf.c_str());
            vec.emplace_back(val);
        }
        return vec;
    }

	const char* comboList() const { return comboList_.c_str(); }
	const char* comboListAlt() const { return comboListAlt_.c_str(); }

	const char* typeName() const { return typeName_.c_str(); }
	Enum defaultValue() const { xassert(!keyToName.empty()); return (Enum)keyToName.begin()->first; }

protected:
	void add(Enum key, const char* name, const char* nameAlt){
		keyToName[key] = name;
		keyToNameAlt[key] = nameAlt;
		nameToKey[name] = key;
		nameAltToKey[nameAlt] = key;
		if(!comboList_.empty()){
			comboList_ += "|";
			comboListAlt_ += "|";
		}
		comboList_ += name;
		comboListAlt_ += nameAlt;
	}

private:
	class Key 
	{
	public: 
		Key(int value) : value_(value) 
		{
			value = (value & 0x55555555) + ((value >> 1) & 0x55555555);
			value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
			value = (value & 0x0f0f0f0f) + ((value >> 4) & 0x0f0f0f0f);
			value = (value & 0x00FF00FF) + ((value >> 8) & 0x00FF00FF);
			value = (value & 0x0000FFFF) + ((value >> 16) & 0x0000FFFF);
			bitsNumber_ = value;
		}

		bool operator<(const Key& rhs) const {
			return bitsNumber_ == rhs.bitsNumber_ ? value_ < rhs.value_ : bitsNumber_ > rhs.bitsNumber_;
		}

        int value() const {
            return value_;
        }

		operator int () const {
			return value_;
		}
			
	private:
		int value_;
		unsigned int bitsNumber_;
	};

	typedef std::map<Key, std::string> KeyToName;
	typedef std::map<std::string, int> NameToKey;

	KeyToName keyToName;
	KeyToName keyToNameAlt;
	NameToKey nameToKey;
	NameToKey nameAltToKey;

	std::string comboList_;
	std::string comboListAlt_;

	std::string typeName_;
};

/////////////////////////////////////////////////
//		Обертка для enums
/////////////////////////////////////////////////
template<class Enum>
class EnumWrapper
{
public:
	EnumWrapper() { value_ = (Enum)0; }
	EnumWrapper(Enum value) : value_(value) {}

	operator Enum() const { return value_; }

	Enum& value() { return value_;	}
	Enum value() const { return value_;	}
	
private:
	Enum value_;
};

/////////////////////////////////////////////////
//	Вектор энумерованных бит
/////////////////////////////////////////////////
template<class Enum, class Value = int>
class BitVector
{
public:
	BitVector(Value value = 0) : value_(value) {}

	operator Value() const { return value_; }

	Value& value() { return value_;	}
	Value value() const { return value_; }

private:
	Value value_;
};

/////////////////////////////////////////////////
//	Строка: const char* снаружи, string внутри
//	При редактировании "\0" означает нулевую строку
/////////////////////////////////////////////////
class PrmString
{
public:
	PrmString(const char* value = "") { 
		(*this) = value; 
	}
	PrmString& operator=(const char* value) { 
		if(value) { value_ = value; zeroPointer_ = false; } else { value_ = ""; zeroPointer_ = true; } return *this; 
	}
	PrmString& operator=(const std::string& value) {
		value_ = value; zeroPointer_ = false; return *this; 
	}
	operator const char*() const { 
		return zeroPointer_ ? nullptr : value_.c_str(); 
	}

	std::string& value() { return value_; }
	const std::string& value() const { return value_; }
    bool empty() const { return zeroPointer_ ? true : value_.empty() ; }

private:
	std::string value_;
	bool zeroPointer_;
};


//////////////////////////////////////////////////////////////
//	Строка с вызовом пользовательской функции редактирования
//////////////////////////////////////////////////////////////
typedef const char* (*CustomValueFunc)(void* hWndParent, const char* initialString);

class CustomString
{
public:
	CustomString(CustomValueFunc customValueFunc, const char* value = "") : customValueFunc_(customValueFunc) { (*this) = value; }
	CustomString& operator=(const char* value) { if(value) { value_ = value; zeroPointer_ = false; } else { value_ = ""; zeroPointer_ = true; } return *this; }
	CustomString& operator=(const std::string& value) { value_ = value; zeroPointer_ = false; return *this; }
    const char* c_str() const { return !zeroPointer_ ? value_.c_str() : nullptr; }
    operator const char*() const { return c_str(); }
	
	CustomValueFunc customValueFunc() const { return customValueFunc_; }

	std::string& value() { return value_; }
	const std::string& value() const { return value_; }

private:
	std::string value_;
	bool zeroPointer_;
	CustomValueFunc customValueFunc_;
};

//////////////////////////////////////////////////////////////
//	Строка с редактируемыми значениями из списка
//////////////////////////////////////////////////////////////
class ComboListString
{
public:
	ComboListString(const char* comboList, const char* value = "") : comboList_(comboList), value_(value) {}
	ComboListString& operator=(const char* value) { value_ = value; return *this; }
	ComboListString& operator=(const std::string& value) { value_ = value; return *this; }

	operator const char*() const { return value_.c_str(); }
	const char* comboList() const { return comboList_; }
	void comboList(const char* _comboList) { comboList_ = _comboList; }

	std::string& value() { return value_; }
	const std::string& value() const { return value_; }

private:
	std::string value_;
	const char* comboList_;
};

/////////////////////////////////////////////////
// Map: typeid -> virtual constructor
/////////////////////////////////////////////////
template<class Base, class Derived>
class ObjectCreator // Для подстановки параметров в конструктор
{
public:
	static Base* create() { 
		return new Derived;
	}
};

template<class Base, class OArchive, class IArchive>
class ClassDescriptor
{
public:
	struct SerializerBase
	{
        virtual Base* create() const { return 0; }
		virtual void save(OArchive& ar, const Base* t) {}
		virtual void load(IArchive& ar, Base* t) {}
		virtual const class TreeNode* treeNode() { return 0; }
		virtual const char* nameAlt() const { return 0; }
		void create(IArchive& ar, Base*& t) {
			t = create();
			load(ar, t);
		}
	};

	template<class Derived>
	struct Serializer : SerializerBase
	{
		Serializer() {
		}
		Serializer(const char* nameAlt) {
			instance().add(*this, get_type_id<Derived>().c_str(), nameAlt);
		}
		void save(OArchive& ar, const Base* t) {
			ar << makeObjectWrapper(static_cast<const Derived&>(*t), 0, 0);
		}
		void load(IArchive& ar, Base* t) {
			ar >> makeObjectWrapper(static_cast<Derived&>(*t), 0, 0);
		}
        Base* create() const
        {
			return ObjectCreator<Base, Derived>::create();
        }
	};

	void add(SerializerBase& serializer, const char* name, const char* nameAlt) {
		xassertStr(map_.find(name) == map_.end() && "Class already registered:", name);
		map_[name] = &serializer;
	}

	SerializerBase& find(const std::string& name) {
		std::string input = name;
        process_type_name(input);
		typename Map::iterator i = map_.find(input.c_str());
		if(i == map_.end()){
			xassertStr(0 && "Unregistered class", name);
			ErrH.Abort("ClassDescriptor::find Unregistered class", XERR_USER, 0, name.c_str());
		}
		return *i->second;
	}

	typedef std::map<std::string, SerializerBase*> Map;

    Map getMap() const
    {
        return map_;
    }

	static ClassDescriptor& instance() {
		return Singleton<ClassDescriptor>::instance();
	}

protected:
	Map map_;
};

////////////////////////////////////////////////////////////////
//	Синглетон с загрузкой и редактированием, т.е. при первом
//  обрашении к нему он читает себя из файла, если в командной 
//  строке указать sectionName, то сразу запускается редактирование.
//  При разрушении он себя записывает (если были изменения).
//	Обязательное инстанцирование с помощью макроса:
//	SINGLETON_PRM(Type, "sectionName", "fileName") type;
//  Define вместе с частичной специализацией позволяет избежать
//  включения XPrm и EditArchive
////////////////////////////////////////////////////////////////
class EditArchive;
template<class T> void loadParameters(T& t);
template<class T> void saveParameters(T& t);
template<class T> bool editParameters(T& t, EditArchive& ea);
template<class T>
class SingletonPrm 
{
private:
    static T& get_instance(bool load) {
        static T* t;
        if(!t){
            static T tt;
            t = &tt;
            if (load) {
                loadParameters(*t);
            }
        }
        return *t;
    }
    
public:
    static void load() {
        return loadParameters(get_instance(false));
    }

    static void save() {
        return saveParameters(get_instance(true));
    }

	static bool edit(EditArchive& ea) {
		return editParameters(get_instance(true), ea);
	}

	T& operator()() const {
		return get_instance(false);
	}
    
    static T& instance() {
        return get_instance(false);
    }        
};

#if !defined(_MSC_VER) || (_MSC_VER >= 1900)
#define DEFINE_SINGLETON_PRM(Type) \
void loadParameters(Type& t); \
void saveParameters(Type& t); \
bool editParameters(Type& t, EditArchive& ea);
#endif


#define SINGLETON_PRM(Type, sectionName, fileName)						\
void loadParameters(Type& t) {											\
	XPrmIArchive ia;													\
	if(ia.open(fileName))												\
		ia >> makeObjectWrapper(t, sectionName, 0);						\
	if(check_command_line(sectionName)){								\
		EditArchive ea = EditArchive();                              	\
		editParameters(t, ea);											\
		ErrH.Exit();													\
	}																	\
}																		\
void saveParameters(Type& t) {											\
	XPrmOArchive(fileName) << makeObjectWrapper(t, sectionName, 0);		\
}																		\
bool editParameters(Type& t, EditArchive& ea) {							\
	if(ea.edit(t, sectionName)){										\
		saveParameters(t);												\
		return true;													\
	}																	\
	return false;														\
}																		\
SingletonPrm<Type> 

/////////////////////////////////////////////////
//    Заворачивание объектов для архивации
/////////////////////////////////////////////////
// Завернуть без перевода с указанием имени
#define WRAP_NAME(object, name) \
	makeObjectWrapper(object, name, 0)

// Завернуть без перевода с именем == имени объекта
#define WRAP_OBJECT(object) \
	makeObjectWrapper(object, #object, 0)

// Завернуть с переводом и указанием основного имени
#define TRANSLATE_NAME(object, name, nameAlt) \
	makeObjectWrapper(object, name, nameAlt)
													
// Завернуть с переводом без указания основного имени
#define TRANSLATE_OBJECT(object, nameAlt) \
	makeObjectWrapper(object, #object, nameAlt)

/////////////////////////////////////////////////
//		Регистрация enums
/////////////////////////////////////////////////
//template<class Enum>
//const EnumDescriptor<Enum>& getEnumDescriptor(const Enum& key);

#define BEGIN_ENUM_DESCRIPTOR(enumType, enumName)	\
	struct Enum##enumType : EnumDescriptor<enumType> { Enum##enumType(); }; \
	Enum##enumType::Enum##enumType() : EnumDescriptor<enumType>(enumName) {

#define REGISTER_ENUM(enumKey, enumNameAlt)	\
	add(enumKey, #enumKey, enumNameAlt); 

#define END_ENUM_DESCRIPTOR(enumType)	\
	}  \
	const EnumDescriptor<enumType>& getEnumDescriptor(const enumType& key){	\
		static Enum##enumType descriptor;	\
		return descriptor;	\
	}

// Для enums, закрытых классами
#define BEGIN_ENUM_DESCRIPTOR_ENCLOSED(nameSpace, enumType, enumName)	\
	struct Enum##nameSpace##enumType : EnumDescriptor<nameSpace::enumType> { Enum##nameSpace##enumType(); }; \
	Enum##nameSpace##enumType::Enum##nameSpace##enumType() : EnumDescriptor<nameSpace::enumType>(enumName) {

#define REGISTER_ENUM_ENCLOSED(nameSpace, enumKey, enumNameAlt)	\
	add(nameSpace::enumKey, #enumKey, enumNameAlt); 

#define END_ENUM_DESCRIPTOR_ENCLOSED(nameSpace, enumType)	\
	}  \
	const EnumDescriptor<nameSpace::enumType>& getEnumDescriptor(const nameSpace::enumType& key){	\
		static Enum##nameSpace##enumType descriptor;	\
		return descriptor;	\
	}

#define DECLARE_ENUM_DESCRIPTOR(enumType)	\
	const EnumDescriptor<enumType>& getEnumDescriptor(const enumType& key);
    
#define DECLARE_ENUM_DESCRIPTOR_ENCLOSED(nameSpace, enumType)	\
	const EnumDescriptor<nameSpace::enumType>& getEnumDescriptor(const nameSpace::enumType& key);

/////////////////////////////////////////////////
//	Вспомогательные функции для отображения
/////////////////////////////////////////////////
template<class Enum>
const char* getEnumName(const Enum& key) {
	return getEnumDescriptor(Enum()).name(key);
}

template<class Enum>
const char* getEnumNameAlt(const Enum& key) {
	return getEnumDescriptor(Enum()).nameAlt(key);
}

template<class Enum>
const char* getEnumName(const EnumWrapper<Enum>& key) {
	return getEnumDescriptor(Enum()).name(key);
}

template<class Enum>
const char* getEnumNameAlt(const EnumWrapper<Enum>& key) {
	return getEnumDescriptor(Enum()).nameAlt(key);
}

template<class Enum>
const EnumDescriptor<Enum>& getEnumDescriptor(const Enum& key);

template<class T>
const ComboListDescriptor<T>& getComboListDescriptor(const T& p);

template<class T>
const char* getComboListDefaultValue(const T& p) {
	return getComboListDescriptor(p).defaultValue();
}

template<class T>
const char* getComboListString(const T& p) {
	return getComboListDescriptor(p).comboList();
}

template<class T>
const char* getComboListValue(const T& p) {
	return getComboListDescriptor(p).name(p);
}

#define BEGIN_COMBO_LIST_DESCRIPTOR(objectType, typeName)	\
	struct ComboListDescriptor##objectType : ComboListDescriptor<objectType> { ComboListDescriptor##objectType(); }; \
	ComboListDescriptor##objectType::ComboListDescriptor##objectType() : ComboListDescriptor<objectType>(typeName) {

#define REGISTER_COMBO_LIST_VALUE(ptr)	\
	add(ptr, #ptr); 

#define END_COMBO_LIST_DESCRIPTOR(objectType)	\
	}  \
	const ComboListDescriptor<objectType>& getComboListDescriptor(const objectType& p){	\
		static ComboListDescriptor##objectType descriptor;	\
		return descriptor;	\
	}

#endif //__ENUM_WRAPPER_H__
