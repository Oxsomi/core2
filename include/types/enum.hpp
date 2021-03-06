#pragma once
#include "types.hpp"
#include <algorithm>

//Everything in this file is to fix C++'s garbage enums, done with meta programming:
//- For exposing data from enums that C++ doesn't such as names, index in enum list
//- Adding missing functionality; such as looping over unique enums, looking them up by name, index or value and bitwise operations

//Exposed enums don't support comments

namespace oic {

	//The format the enum's name is going into
	//Example: MY_ENUM -> MY_ENUM, My_enum or My enum
	//
	//All enums should use UPPERCASE_NAMING so this format makes sense
	//
	enum class EnumNameFormat : u8 {
		NO_FORMAT,					//Don't format MY_ENUM
		LOWERCASE_UNDERSCORE,		//Format MY_ENUM as My_enum
		LOWERCASE_SPACE				//Format MY_ENUM as My enum
	};

	//Helper functions for enums

	struct EnumHelper {

		//If it has all unique values (required for iterable enums)

		template<typename T, usz N>
		static inline constexpr usz isUnique(const Array<T, N> &t) {

			for (usz i = 0; i < N; ++i)
				for (usz j = 0; j < N; ++j)
					if (i != j && t[i] == t[j])
						return i;

			return N;
		}

		//Get locations and lengths of enum names

		template<usz positionSize, usz stringLen>
		static inline constexpr Array<usz, positionSize> getPositions(const c8(&str)[stringLen]) {

			Array<usz, positionSize> v{};

			bool shouldScan = true, started = false;
			usz prev{}, end{}, j = 0;

			for (usz i = 0; i < (stringLen - 1); ++i) {

				if (str[i] == '=' || str[i] == ' ') {

					if (started && shouldScan) {
						end = i;
						shouldScan = false;
					}

				} else if (str[i] == ',') {

					if (shouldScan)
						end = i;

					v[j << 1] = prev;
					v[1 + (j << 1)] = end - prev;
					++j;
					shouldScan = true;
					started = false;
				} else if (!started && shouldScan) {
					prev = i;
					started = true;
				}
			}

			v[j << 1] = prev;
			v[(j << 1) + 1] = (end < prev ? (stringLen - 1) : end) - prev;

			return v;
		}

		template<EnumNameFormat format>
		static inline constexpr c8 formatChar(usz i, c8 c) {
			return
				c == '_' ? (format == EnumNameFormat::LOWERCASE_SPACE ? ' ' : '_') :
				(
					i != 0 && format != EnumNameFormat::NO_FORMAT ?
					(c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c) : c
				);
		}

	};
}

//Base class of an enum; represents the functionality a basic enum should have
//A basic enum is just seen as a value that can be set and has values it can be set to

#define _oicEnumBase(EnumName, EnumType, ...)																		\
class EnumName {																									\
																													\
public:																												\
																													\
	enum _E : EnumType { __VA_ARGS__ };																				\
	using BaseType = EnumType;																						\
																													\
	_E value;																										\
																													\
public:																												\
																													\
	constexpr EnumName() : value(_E(0)) {}																			\
	constexpr EnumName(_E value) : value(value) {}																	\
	constexpr EnumName(BaseType value) : value(_E(value)) {}														\
	constexpr EnumName(const EnumName &other) : value(other.value) {}												\
	constexpr EnumName(EnumName &&other) : value(other.value) {}													\
																													\
	inline constexpr EnumName &operator=(const EnumName &other) {													\
		value = other.value;																						\
		return *this;																								\
	}																												\
																													\
	inline constexpr EnumName &operator=(EnumName &&other) {														\
		value = other.value;																						\
		return *this;																								\
	}																												\
																													\
	inline constexpr bool operator>(const EnumName &other) const { return value > other.value; }					\
	inline constexpr bool operator<(const EnumName &other) const { return value < other.value; }					\
	inline constexpr bool operator>=(const EnumName &other) const { return value >= other.value; }					\
	inline constexpr bool operator<=(const EnumName &other) const { return value <= other.value; }					\
	inline constexpr bool operator!=(const EnumName &other) const { return value != other.value; }					\
	inline constexpr bool operator==(const EnumName &other) const { return value == other.value; }					\
	inline constexpr isz operator-(const EnumName &other) const { return isz(value) - isz(other.value); }

//Add flags functionality to enum (allowing &, |, ^, <<, >>, ~)

#define _oicFlagInterface(EnumName, EnumType)																		\
public:																												\
	constexpr EnumName(EnumType val) : value(_E(val)) {}															\
	inline constexpr EnumName &operator&=(const EnumName &other) { return *this = operator&(other); }				\
	inline constexpr EnumName &operator|=(const EnumName &other) { return *this = operator|(other); }				\
	inline constexpr EnumName &operator^=(const EnumName &other) { return *this = operator^(other); }				\
	inline constexpr EnumName operator|(const EnumName &other) const { return _E(value | other.value); }			\
	inline constexpr EnumName operator&(const EnumName &other) const { return _E(value & other.value); }			\
	inline constexpr EnumName operator^(const EnumName &other) const { return _E(value ^ other.value); }			\
	inline constexpr EnumName operator~() const { return _E(~value); }												\
	inline constexpr EnumName operator|(const _E other) const { return _E(value | other); }							\
	inline constexpr EnumName operator&(const _E other) const { return _E(value & other); }							\
	inline constexpr EnumName operator^(const _E other) const { return _E(value ^ other); }							\
	inline constexpr EnumName operator~() const { return _E(~value); }

//Base class of an iterable class of an enum
//This allows to loop over all enums you have defined

#define _oicIterableEnumBase(EnumName, EnumType, ...) 																\
/* This is not allowed to be inside of the struct for some reason; it works when it is placed outside */			\
static constexpr auto _##EnumName##_getValues(){																	\
																													\
	struct Wrapper {																								\
																													\
		EnumType type;																								\
		bool isInitialized;																							\
																													\
		constexpr Wrapper(EnumType type) : type(type), isInitialized(true) {}										\
		constexpr Wrapper() : type(), isInitialized(false) {}														\
		constexpr Wrapper(const Wrapper &wr): type(wr.type), isInitialized(wr.isInitialized) {}						\
																													\
		constexpr operator EnumType() { return type; }																\
																													\
		inline constexpr EnumType operator=(const Wrapper &) const { return type; }									\
	};																												\
																													\
	Wrapper __VA_ARGS__;																							\
	Wrapper wrappers[] = { __VA_ARGS__ };																			\
																													\
	constexpr usz valueC = sizeof(wrappers) / sizeof(Wrapper);														\
																													\
	Array<EnumType, valueC> res{};																					\
																													\
	EnumType prev{};																								\
	usz i{};																										\
																													\
	for (auto &wrapper : wrappers) {																				\
																													\
		if (!wrapper.isInitialized)																					\
			prev = res[i] = prev;																					\
		else																										\
			prev = res[i] = wrapper.type;																			\
																													\
		++prev;																										\
		++i;																										\
	}																												\
																													\
	return res;																										\
}																													\
																													\
_oicEnumBase(EnumName, EnumType, __VA_ARGS__)																		\
																													\
private:																											\
																													\
	using Helper = oic::EnumHelper;																					\
																													\
public:																												\
																													\
	static constexpr auto values = _##EnumName##_getValues();														\
	static constexpr usz count = values.size();																		\
																													\
private:																											\
																													\
	static constexpr usz firstInvalid = Helper::isUnique(values);													\
	static constexpr EnumType invalidValue = firstInvalid == count ? EnumType(0) : values[firstInvalid];			\
																													\
	static void validate() {																						\
		static_assert(firstInvalid == count, "Iterable enums value is not unique but should be");					\
	}																												\
																													\
public:																												\
																													\
	static inline usz idByValue(_E t) {																				\
																													\
		usz i{};																									\
																													\
		for (; i < count; ++i)																						\
			if (values[i] == t)																						\
				return i;																							\
																													\
		return i;																									\
	}																												\
																													\
	template<_E t>																									\
	static inline constexpr usz idByValue() {																		\
																													\
		usz i{};																									\
																													\
		for (; i < count; ++i)																						\
			if (values[i] == t)																						\
				return i;																							\
																													\
		return i;																									\
	}

//Base class of an exposed class of an enum
//This allows to loop over all enums you have defined and get their names

#define _oicExposedEnumBase(EnumName, EnumType, ...) _oicIterableEnumBase(EnumName, EnumType, __VA_ARGS__) 			\
private:																											\
																													\
	static constexpr c8 args[] = #__VA_ARGS__;																		\
	static constexpr usz positionSize = count << 1;																	\
																													\
	static constexpr Array<usz, positionSize> positions = Helper::getPositions<positionSize>(args);					\
																													\
public:																												\
																													\
	template<usz i, oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>									\
	static inline constexpr auto nameById() {																		\
																													\
		constexpr usz nameLen = positions[(i << 1) + 1];															\
		Array<c8, nameLen + 1> name{};																				\
																													\
		for (usz j = positions[i << 1], k = j, l = j + nameLen; k < l; ++k)											\
			name[k - j] = Helper::formatChar<format>(k - j, args[k]);												\
																													\
		return name;																								\
	}																												\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>											\
	static inline const String nameById(usz i) {																	\
																													\
		usz nameLen = positions[(i << 1) + 1];																		\
		String name(nameLen, ' ');																					\
																													\
		for (usz j = positions[i << 1], k = j, l = j + nameLen; k < l; ++k)											\
			name[k - j] = Helper::formatChar<format>(k - j, args[k]);												\
																													\
		return name;																								\
	}																												\
																													\
	template<_E v, oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>										\
	static inline constexpr auto nameByValue() { return nameById<idByValue<v>(), format>(); }						\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>											\
	static inline auto nameByValue(_E v) { return nameById<format>(idByValue(v)); }									\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT, usz N>									\
	static inline constexpr usz idByName(const Array<c8, N> &t) {													\
																													\
		usz i{};																									\
																													\
		for (; i < count; ++i) {																					\
																													\
			usz j = positions[(i << 1) + 1];																		\
																													\
			if (j == N - 1) {																						\
																													\
				usz l = positions[i << 1];																			\
																													\
				bool match = true;																					\
																													\
				for (usz k {}; k < j; ++k)																			\
					if (Helper::formatChar<format>(k, args[l + k]) != t[k]) {										\
						match = false;																				\
						break;																						\
					}																								\
																													\
				if (match)																							\
					return i;																						\
			}																										\
		}																											\
																													\
		return i;																									\
	}																												\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT, usz N>									\
	static inline constexpr _E valueByName(const Array<c8, N> &t) {													\
		return _E(values[idByName<format>(t)]);																		\
	}																												\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>											\
	static inline usz idByName(const String &t) {																	\
																													\
		usz i{};																									\
																													\
		for (; i < count; ++i) {																					\
																													\
			usz j = positions[(i << 1) + 1];																		\
																													\
			if (j == t.size()) {																					\
																													\
				usz l = positions[i << 1];																			\
																													\
				bool match = true;																					\
																													\
				for (usz k {}; k < j; ++k)																			\
					if (Helper::formatChar<format>(k, args[l + k]) != t[k]) {										\
						match = false;																				\
						break;																						\
					}																								\
																													\
				if (match)																							\
					return i;																						\
			}																										\
		}																											\
																													\
		return i;																									\
	}																												\
																													\
	template<oic::EnumNameFormat format = oic::EnumNameFormat::NO_FORMAT>											\
	static inline _E valueByName(const String &t) {																	\
		return _E(values[idByName<format>(t)]);																		\
	}																												\
																													\
	template<bool replaceUnderscore = true>																			\
	static inline List<String> _getNames() {																		\
																													\
		List<String> names(count);																					\
																													\
		for (usz i = 0; i < count; ++i)																				\
			if constexpr(replaceUnderscore) {																		\
				String s = String(nameById(i));																		\
				std::replace(s.begin(), s.end(), '_', ' ');															\
				names[i] = s;																						\
			}																										\
			else																									\
				names[i] = nameById(i);																				\
																													\
		return names;																								\
	}																												\
																													\
	template<bool replaceUnderscore = true>																			\
	static inline const List<String> &getNames() {																	\
		static const List<String> names = _getNames<replaceUnderscore>();											\
		return names;																								\
	}																												\
																													\
	static inline List<const c8*> _getCNames(const List<String> &strings) {											\
																													\
		List<const c8*> cNames(count);																				\
																													\
		for(usz i = 0; i < count; ++i)																				\
			cNames[i] = strings[i].data();																			\
																													\
		return cNames;																								\
	}																												\
																													\
	template<bool replaceUnderscore = true>																			\
	static inline const List<const c8*> &getCNames() {																\
		static const List<const c8*> names = _getCNames(getNames<replaceUnderscore>());								\
		return names;																								\
	}

//These are all of our enum types:
//Enum = a value that can be set
//Flags = base2 (1, 2, 4, ...) that can use bitwise operators

//And modifiers:
//Iterable = The enum values are available to loop through and obtain index from a value (so every enum has to be unique)
//Exposed = Iterable + Unique + the enum's names are also available

#define oicEnum(EnumName, EnumType, ...) _oicEnumBase(EnumName, EnumType, __VA_ARGS__) };
#define oicIterableEnum(EnumName, EnumType, ...) _oicIterableEnumBase(EnumName, EnumType, __VA_ARGS__) };
#define oicExposedEnum(EnumName, EnumType, ...) _oicExposedEnumBase(EnumName, EnumType, __VA_ARGS__) };

#define oicFlags(EnumName, EnumType, ...) _oicEnumBase(EnumName, EnumType, __VA_ARGS__) _oicFlagInterface(EnumName, EnumType) };
#define oicIterableFlags(EnumName, EnumType, ...) _oicIterableEnumBase(EnumName, EnumType, __VA_ARGS__) _oicFlagInterface(EnumName, EnumType) };
#define oicExposedFlags(EnumName, EnumType, ...) _oicExposedEnumBase(EnumName, EnumType, __VA_ARGS__) _oicFlagInterface(EnumName, EnumType) };

//A cross platform way of overriding &=, &, |=, |, ^, ^=, ~ on enums
//oicEnumFlag(MyEnum, u8);

#define enumFlagOverloads(Enum) 																																			\
inline constexpr Enum operator|(const Enum &a, const Enum &b) { return Enum(std::underlying_type_t<Enum>(a) | std::underlying_type_t<Enum>(b)); }							\
inline constexpr Enum operator&(const Enum &a, const Enum &b) { return Enum(std::underlying_type_t<Enum>(a) & std::underlying_type_t<Enum>(b)); }							\
inline constexpr Enum operator^(const Enum &a, const Enum &b) { return Enum(std::underlying_type_t<Enum>(a) ^ std::underlying_type_t<Enum>(b)); }							\
inline constexpr Enum operator~(const Enum &a) { return Enum(~std::underlying_type_t<Enum>(a)); }																			\
inline constexpr Enum operator<<(const Enum &a, usz amount) { return Enum(std::underlying_type_t<Enum>(a) << amount); }																			\
																																											\
inline Enum &operator|=(Enum &a, const Enum &b) { *(std::underlying_type_t<Enum>*)&a = std::underlying_type_t<Enum>(a) | std::underlying_type_t<Enum>(b); return a; }		\
inline Enum &operator&=(Enum &a, const Enum &b) { *(std::underlying_type_t<Enum>*)&a = std::underlying_type_t<Enum>(a) & std::underlying_type_t<Enum>(b); return a; }		\
inline Enum &operator^=(Enum &a, const Enum &b) { *(std::underlying_type_t<Enum>*)&a = std::underlying_type_t<Enum>(a) ^ std::underlying_type_t<Enum>(b); return a; }		\
																																											\
inline constexpr std::underlying_type_t<Enum> AsValue(const Enum &a) { return std::underlying_type_t<Enum>(a); }															\
inline constexpr bool HasFlags(const Enum &a, const Enum &b) { return (a & b) == b; }

namespace oic {

	template<typename, typename T>
	struct is_exposed_enum {
		static constexpr bool value = false;
	};

	template<typename C>
	struct is_exposed_enum<C, void> {

	private:

		template<typename T>
		static constexpr auto check(T*)
			-> typename std::is_same<
			decltype(T::getCNames() ),
			const List<const c8*>& 
			>::type; 

		template<typename>
		static constexpr std::false_type check(...);

		typedef decltype(check<C>(0)) type;

	public:

		static constexpr bool value = type::value;
	};

	template<typename T>
	static constexpr bool is_exposed_enum_v = is_exposed_enum<T, void>::value;

}

//TODO: Perhaps not needed if the implicit cast to _E comes back

//Needs to be called in global namespace
#define oicEnumHash(MyEnum)											\
namespace std {														\
	template<>														\
	struct hash<MyEnum> {											\
		inline usz operator()(const MyEnum &e) const {				\
			return usz(e.value);									\
		}															\
	};																\
}