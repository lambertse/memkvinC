
#pragma once

#ifndef _H_STRING_CAST_
#define _H_STRING_CAST_

/**
 *	@file string_cast.h
 *	@brief Defines our templated string_cast functions for string conversion
 *
 *	@brief Utility code for converting strings
 */

// C++17 standard says deprecated until a suitable replace is available. So keep
// using until then
#ifndef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#define STRING_CAST_DISABLE_CXX17_DEPRACATINO_WARNINGS
#endif

#include <algorithm>
#include <codecvt>
#include <functional>
#include <locale>
#include <string>

#pragma warning(push)
#pragma warning(disable : 4996)

/**
 *	@brief Defines our class for working with string utility functions
 *	@ingroup waapi_internal_strings
 */
namespace StringCast {

#if _MSC_VER >= 1900
// Silly, silly Microsoft
// https://social.msdn.microsoft.com/Forums/en-US/8f40dcd8-c67f-4eba-9134-a19b9178e481/vs-2015-rc-linker-stdcodecvt-error
typedef std::wstring_convert<std::codecvt_utf8<int32_t>, int32_t> utf32conv;
typedef std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t>
    utf16conv;
#else
typedef std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> utf32conv;
typedef std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>
    utf16conv;
#endif

#ifdef FORCE_UCS_2_FOR_WCHAR
static const bool forceUCS_2 = true;
#else
static const bool forceUCS_2 = false;
#endif

#if WCHAR_MAX > (1 << 17)
static const bool useLargeWchar = true;
#else
static const bool useLargeWchar = false;
#endif

/**
 *	@brief Will make the input string lowercase
 *	@param[in] _str The wide string to make lowercase
 */
template <typename Tchar>
static inline void toLower(std::basic_string<Tchar>& _str) {
  std::transform(_str.begin(), _str.end(), _str.begin(),
                 [](Tchar c) { return std::tolower(c, std::locale()); });
}

/**
 *	@brief Will make the input string uppercase
 *	@param[in] _str The string to make uppercase
 */
template <typename Tchar>
static inline void toUpper(std::basic_string<Tchar>& _str) {
  std::transform(_str.begin(), _str.end(), _str.begin(),
                 [](Tchar c) { return std::toupper(c, std::locale()); });
}

/**
 *	@brief Will trim left white space of string
 *	@param[in] _str The string to left trim
 */
template <typename Tchar>
static inline void ltrim(std::basic_string<Tchar>& _str) {
  _str.erase(_str.begin(), std::find_if(_str.begin(), _str.end(), [](Tchar c) {
               return !std::isspace(c, std::locale());
             }));
}

// forward decls for string_cast helper templates
template <typename Tdst, typename Tsrc>
struct string_cast_spec;
template <typename Tdst, typename Tsrc>
class string_cast_converter;
template <typename T>
struct char_type;

/**
 *	Primary template for casting between string types
 *	usage: Tdst foo = string_cast<Tdst>( Tsrc() );
 *	Tsrc type is inferred through specializations
 *	Each supported Tdst type needs a string_cast_converter defined to be
 *supported.
 */
template <typename Tdst, typename Tsrc>
Tdst string_cast(const Tsrc& _source) {
  return string_cast_spec<Tdst, Tsrc>::cast(_source);
}
// string_cast wrapper to allow pointers
template <typename Tdst, typename Tsrc>
Tdst string_cast(Tsrc* _source) {
  return string_cast_spec<
      Tdst, typename char_type<const Tsrc*>::str_type>::cast(_source);
}

// templates for each char pointer type to map to the appropriate string version
template <>
struct char_type<const char*> {
  typedef std::string str_type;
};
template <>
struct char_type<const wchar_t*> {
  typedef std::wstring str_type;
};
template <>
struct char_type<const char16_t*> {
  typedef std::u16string str_type;
};
template <>
struct char_type<const char32_t*> {
  typedef std::u32string str_type;
};

// specialization for when Tsrc == Tdst type
template <typename Tdst>
struct string_cast_spec<Tdst, Tdst> {
  static const Tdst& cast(const Tdst& _source) { return _source; }
};
// normal conversion, when Tsrc != Tdst type
template <typename Tdst, typename Tsrc>
struct string_cast_spec {
  static Tdst cast(const Tsrc& _source) {
    return string_cast_converter<Tdst, Tsrc>::convert(_source);
  }
};

/*
 *	Explicit specializations for supporting Tdst=u32string conversion.
 */
template <typename Tsrc>
class string_cast_converter<std::u32string, Tsrc> {
 public:
  template <typename Tstring>
  static std::u32string convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static std::u32string _convert(const std::u16string& _str) {
    return string_cast<std::u32string>(string_cast<std::string>(_str));
  }
  static std::u32string _convert(const std::wstring& _str) {
    if (useLargeWchar)
      return std::u32string(reinterpret_cast<const char32_t*>(_str.c_str()),
                            _str.size());
    else
      return string_cast<std::u32string>(string_cast<std::u16string>(_str));
  }
  static std::u32string _convert(const std::string& _str) {
    utf32conv conv;
    const auto& bytes = conv.from_bytes(_str);
    return std::u32string(reinterpret_cast<const char32_t*>(bytes.c_str()),
                          bytes.size());
  }
  static std::u32string _convert(const bool& _b) {
    if (_b) return U"true";
    return U"false";
  }
  template <typename Tstring>
  static std::u32string _convert(const Tstring& _num) {
    static_assert(std::is_integral<Tstring>::value ||
                      std::is_floating_point<Tstring>::value,
                  "Only numeric arguments are supported in this routine.");
    return string_cast<std::u32string>(std::to_string(_num));
  }
};

/*
 *	Explicit specializations for supporting Tdst=u16string conversion.
 */
template <typename Tsrc>
class string_cast_converter<std::u16string, Tsrc> {
 public:
  template <typename Tstring>
  static std::u16string convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static std::u16string _convert(const std::u32string& _str) {
    return string_cast<std::u16string>(string_cast<std::string>(_str));
  }
  static std::u16string _convert(const std::wstring& _str) {
    if (useLargeWchar)
      return string_cast<std::u16string>(string_cast<std::string>(_str));
    else
      return std::u16string(reinterpret_cast<const char16_t*>(_str.c_str()),
                            _str.size());
  }
  static std::u16string _convert(const std::string& _str) {
    utf16conv conv;
    const auto& bytes = conv.from_bytes(_str);
    return std::u16string(reinterpret_cast<const char16_t*>(bytes.c_str()),
                          bytes.size());
  }
  static std::u16string _convert(const bool& _b) {
    if (_b) return u"true";
    return u"false";
  }
  template <typename Tstring>
  static std::u16string _convert(const Tstring& _num) {
    static_assert(std::is_integral<Tstring>::value ||
                      std::is_floating_point<Tstring>::value,
                  "Only numeric arguments are supported in this routine.");
    return string_cast<std::u16string>(std::to_string(_num));
  }
};

/*
 *	Explicit specializations for supporting Tdst=wstring conversion.
 */
template <typename Tsrc>
class string_cast_converter<std::wstring, Tsrc> {
 public:
  template <typename Tstring>
  static std::wstring convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static std::wstring _convert(const std::u32string& _str) {
    if (useLargeWchar) {
      return std::wstring(reinterpret_cast<const wchar_t*>(_str.c_str()),
                          _str.size());
    } else {
      const auto& u16 = string_cast<std::u16string>(_str);
      return std::wstring(reinterpret_cast<const wchar_t*>(u16.c_str()),
                          u16.size());
    }
  }
  static std::wstring _convert(const std::u16string& _str) {
    if (useLargeWchar) {
      const auto& u32 = string_cast<std::u32string>(_str);
      return std::wstring(reinterpret_cast<const wchar_t*>(u32.c_str()),
                          u32.size());
    } else {
      return std::wstring(reinterpret_cast<const wchar_t*>(_str.c_str()),
                          _str.size());
    }
  }
  static std::wstring _convert(const std::string& _str) {
    if (useLargeWchar) {
      const auto& u32 = string_cast<std::u32string>(_str);
      return std::wstring(reinterpret_cast<const wchar_t*>(u32.c_str()),
                          u32.size());
    } else {
      if (forceUCS_2) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>
            utf16conv("Bad Convert", L"Bad Convert");
        return utf16conv.from_bytes(_str);
      }

      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
      const auto& bytes = conv.from_bytes(_str);
      return std::wstring(bytes.c_str(), bytes.size());
    }
  }
  static std::wstring _convert(const bool& _b) {
    if (_b) return L"true";
    return L"false";
  }
  template <typename Tstring>
  static std::wstring _convert(const Tstring& _num) {
    static_assert(std::is_integral<Tstring>::value ||
                      std::is_floating_point<Tstring>::value,
                  "Only numeric arguments are supported in this routine.");
    return std::to_wstring(_num);
  }
};

/*
 *	Explicit specializations for supporting Tdst=string conversion.
 */
template <typename Tsrc>
class string_cast_converter<std::string, Tsrc> {
 public:
  template <typename Tstring>
  static std::string convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static std::string _convert(const std::u32string& _str) {
    const auto p = reinterpret_cast<const utf32conv::wide_string::value_type*>(
        _str.c_str());
    utf32conv conv;
    return conv.to_bytes(p, p + _str.size());
  }
  static std::string _convert(const std::u16string& _str) {
    const auto p = reinterpret_cast<const utf16conv::wide_string::value_type*>(
        _str.c_str());
    utf16conv conv;
    return conv.to_bytes(p, p + _str.size());
  }
  static std::string _convert(const std::wstring& _str) {
    if (forceUCS_2) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> conv("Bad Convert",
                                                            L"Bad Convert");
      return conv.to_bytes(_str.data());
    }

    if (useLargeWchar) {
      std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> conv;
      const auto& bytes = conv.to_bytes(_str);
      return std::string(bytes.c_str(), bytes.size());
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> conv;
    const auto& bytes = conv.to_bytes(_str);
    return std::string(bytes.c_str(), bytes.size());
  }
  static std::string _convert(const bool& _b) {
    if (_b) return "true";
    return "false";
  }
  template <typename Tstring>
  static std::string _convert(const Tstring& _num) {
    static_assert(std::is_integral<Tstring>::value ||
                      std::is_floating_point<Tstring>::value,
                  "Only numeric arguments are supported in this routine.");
    return std::to_string(_num);
  }
};

/*
 *	Explicit specializations for supporting Tdst=int conversion.
 */
template <typename Tsrc>
class string_cast_converter<int, Tsrc> {
 public:
  template <typename Tstring>
  static int convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static int _convert(std::string _str) {
    return static_cast<int>(std::stoll(_str, 0, 0));
  }
  static int _convert(std::wstring _str) {
    return static_cast<int>(std::stoll(_str, 0, 0));
  }
  static int _convert(const std::u16string& _str) {
    return static_cast<int>(string_cast<long long>(_str));
  }
  static int _convert(const std::u32string& _str) {
    return static_cast<int>(string_cast<long long>(_str));
  }
};

/*
 *	Explicit specializations for supporting Tdst=bool conversion.
 */
template <typename Tsrc>
class string_cast_converter<bool, Tsrc> {
 public:
  template <typename Tstring>
  static bool convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static bool _convert(const std::string& _str) {
    std::string tempStr = _str;
    toLower(tempStr);
    if (tempStr == "true") return true;
    return false;
  }
  static bool _convert(const std::wstring& _str) {
    std::wstring tempStr = _str;
    toLower(tempStr);
    if (tempStr == L"true") return true;
    return false;
  }
};

/*
 *	Explicit specializations for supporting Tdst=long long conversion.
 */
template <typename Tsrc>
class string_cast_converter<long long, Tsrc> {
 public:
  template <typename Tstring>
  static long long convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static long long _convert(std::string _str) { return std::stoll(_str, 0, 0); }
  static long long _convert(std::wstring _str) {
    return std::stoll(_str, 0, 0);
  }
  static long long _convert(const std::u16string& _str) {
    if (useLargeWchar)
      return string_cast<long long>(string_cast<std::u32string>(_str));
    else
      return string_cast<long long>(
          reinterpret_cast<const wchar_t*>(_str.c_str()));
  }
  static long long _convert(const std::u32string& _str) {
    if (useLargeWchar)
      return string_cast<long long>(
          reinterpret_cast<const wchar_t*>(_str.c_str()));
    else
      return string_cast<long long>(string_cast<std::string>(_str));
  }
};

/*
 *	Explicit specializations for supporting Tdst=unsigned long long
 *conversion.
 */
template <typename Tsrc>
class string_cast_converter<unsigned long long, Tsrc> {
 public:
  template <typename Tstring>
  static unsigned long long convert(const Tstring& _source) {
    return _convert(_source);
  }

 private:
  static unsigned long long _convert(std::string _str) {
    return std::stoull(_str, 0, 0);
  }
  static unsigned long long _convert(std::wstring _str) {
    return std::stoull(_str, 0, 0);
  }
  static unsigned long long _convert(const std::u16string& _str) {
    if (useLargeWchar)
      return string_cast<unsigned long long>(string_cast<std::u32string>(_str));
    else
      return string_cast<unsigned long long>(
          reinterpret_cast<const wchar_t*>(_str.c_str()));
  }
  static long long _convert(const std::u32string& _str) {
    if (useLargeWchar)
      return string_cast<unsigned long long>(
          reinterpret_cast<const wchar_t*>(_str.c_str()));
    else
      return string_cast<unsigned long long>(string_cast<std::string>(_str));
  }
};

#ifdef UNIT_TEST

static bool UnitTest() {
  using namespace std;

  char u8_ucs[] = u8"OPSWAT来日(2016年4月)";
  wchar_t L_ucs[] = L"OPSWAT来日(2016年4月)";

  char u8_literal[] = u8"😁 😂 😒 😀😬😝OPSWAT来日(2016年4月)";
  char16_t u_literal[] = u"😁 😂 😒 😀😬😝OPSWAT来日(2016年4月)";
  char32_t U_literal[] = U"😁 😂 😒 😀😬😝OPSWAT来日(2016年4月)";
  wchar_t L_literal[] = L"😁 😂 😒 😀😬😝OPSWAT来日(2016年4月)";

  auto u8_size = sizeof(u8_literal) / sizeof(u8_literal[0]) - 1;
  auto u_size = sizeof(u_literal) / sizeof(u_literal[0]) - 1;
  auto U_size = sizeof(U_literal) / sizeof(U_literal[0]) - 1;
  auto L_size = sizeof(L_literal) / sizeof(L_literal[0]) - 1;

  auto u8 = string(u8_literal, u8_size);
  auto u = u16string(u_literal, u_size);
  auto U = u32string(U_literal, U_size);
  auto L = wstring(L_literal, L_size);

#if WCHAR_MAX > (1 << 17)
  assert(L_size == U_size);
#else
  assert(L_size == u_size);
#endif
  assert(string_cast<string>(u8_ucs) == u8_ucs);
  assert(string_cast<string>(L_ucs) == u8_ucs);
  assert(string_cast<wstring>(u8_ucs) == L_ucs);
  assert(string_cast<wstring>(L_ucs) == L_ucs);

  assert(string(u8_literal) == string_cast<string>(u8_literal));
  assert(string(u8_literal) == string_cast<string>(u_literal));
  assert(string(u8_literal) == string_cast<string>(U_literal));
#ifndef PASS_UCS_2_FOR_WCHAR
  auto test = string_cast<string>(L_literal);
  string test2(u8_literal);

  assert(test2 == test);
#endif

  assert(u8 == string_cast<string>(string_cast<string>(u8)));
  assert(u8 == string_cast<string>(string_cast<u32string>(u8)));
  assert(u8 == string_cast<string>(string_cast<u16string>(u8)));
#ifndef PASS_UCS_2_FOR_WCHAR
  assert(u8 == string_cast<string>(string_cast<wstring>(u8)));
#endif

  assert(u8 == string_cast<string>(u));
  assert(u8 == string_cast<string>(string_cast<string>(u)));
  assert(u8 == string_cast<string>(string_cast<u32string>(u)));
  assert(u8 == string_cast<string>(string_cast<u16string>(u)));
#ifndef PASS_UCS_2_FOR_WCHAR
  assert(u8 == string_cast<string>(string_cast<wstring>(u)));
#endif

  assert(u8 == string_cast<string>(string_cast<string>(U)));
  assert(u8 == string_cast<string>(string_cast<u32string>(U)));
  assert(u8 == string_cast<string>(string_cast<u16string>(U)));
#ifndef PASS_UCS_2_FOR_WCHAR
  assert(u8 == string_cast<string>(string_cast<wstring>(U)));

  assert(u8 == string_cast<string>(string_cast<string>(L)));
  assert(u8 == string_cast<string>(string_cast<wstring>(L)));
#endif
  assert(u8 == string_cast<string>(string_cast<u32string>(L)));
  assert(u8 == string_cast<string>(string_cast<u16string>(L)));

  assert(U == string_cast<u32string>(string_cast<string>(U)));
  assert(U == string_cast<u32string>(string_cast<u32string>(U)));
  assert(U == string_cast<u32string>(string_cast<u16string>(U)));
  assert(U == string_cast<u32string>(string_cast<wstring>(U)));

  assert(U == string_cast<u32string>(string_cast<string>(u)));
  assert(U == string_cast<u32string>(string_cast<u32string>(u)));
  assert(U == string_cast<u32string>(string_cast<u16string>(u)));
  assert(U == string_cast<u32string>(string_cast<wstring>(u)));

  assert(U == string_cast<u32string>(string_cast<string>(U)));
  assert(U == string_cast<u32string>(string_cast<u32string>(U)));
  assert(U == string_cast<u32string>(string_cast<u16string>(U)));
  assert(U == string_cast<u32string>(string_cast<wstring>(U)));

#ifndef PASS_UCS_2_FOR_WCHAR
  assert(U == string_cast<u32string>(string_cast<string>(L)));
#endif
  assert(U == string_cast<u32string>(string_cast<u32string>(L)));
  assert(U == string_cast<u32string>(string_cast<u16string>(L)));
  assert(U == string_cast<u32string>(string_cast<wstring>(L)));

  assert(u == string_cast<u16string>(string_cast<string>(U)));
  assert(u == string_cast<u16string>(string_cast<u32string>(U)));
  assert(u == string_cast<u16string>(string_cast<u16string>(U)));
  assert(u == string_cast<u16string>(string_cast<wstring>(U)));

  assert(u == string_cast<u16string>(string_cast<string>(u)));
  assert(u == string_cast<u16string>(string_cast<u32string>(u)));
  assert(u == string_cast<u16string>(string_cast<u16string>(u)));
  assert(u == string_cast<u16string>(string_cast<wstring>(u)));

  assert(u == string_cast<u16string>(string_cast<string>(U)));
  assert(u == string_cast<u16string>(string_cast<u32string>(U)));
  assert(u == string_cast<u16string>(string_cast<u16string>(U)));
  assert(u == string_cast<u16string>(string_cast<wstring>(U)));

#ifndef PASS_UCS_2_FOR_WCHAR
  assert(u == string_cast<u16string>(string_cast<string>(L)));
#endif
  assert(u == string_cast<u16string>(string_cast<u32string>(L)));
  assert(u == string_cast<u16string>(string_cast<u16string>(L)));
  assert(u == string_cast<u16string>(string_cast<wstring>(L)));

  assert(string_cast<string>(42) == "42");
  assert(string_cast<u32string>(42) == U"42");
  assert(string_cast<u16string>(42) == u"42");
  assert(string_cast<wstring>(42) == L"42");

  assert(string_cast<string>(false) == "false");

  assert(string_cast<string>(true) == "true");
  assert(string_cast<u32string>(true) == U"true");
  assert(string_cast<u16string>(true) == u"true");
  assert(string_cast<wstring>(true) == L"true");

  assert(string_cast<int>("0010 bob") == 0xA);
  assert(string_cast<int>(" 0010 bob") == 0xA);
  assert(string_cast<int>(U" 000010 bob") == 0xA);
  assert(string_cast<int>(u" 010 bob") == 0xA);
  assert(string_cast<int>(L"   000010 bob") == 0xA);

  assert(string_cast<int>(" 10 bob") == 0xA);
  assert(string_cast<int>(U" 10 bob") == 0xA);
  assert(string_cast<int>(u" 10 bob") == 0xA);
  assert(string_cast<int>(L" 10 bob") == 0xA);

  assert(string_cast<int32_t>("0x7fffffff") == 0x7fffffff);
  assert(string_cast<int32_t>(U"0x7fffffff") == 0x7fffffff);
  assert(string_cast<int32_t>(u"0x7fffffff") == 0x7fffffff);
  assert(string_cast<int32_t>(L"0x7fffffff") == 0x7fffffff);

  assert(string_cast<long long>("0x7fffffffffffffff") == 0x7fffffffffffffff);
  assert(string_cast<long long>(U"0x7fffffffffffffff") == 0x7fffffffffffffff);
  assert(string_cast<long long>(u"0x7fffffffffffffff") == 0x7fffffffffffffff);
  assert(string_cast<long long>(L"0x7fffffffffffffff") == 0x7fffffffffffffff);

  assert(string_cast<unsigned long long>("0xffffffffffffffff") ==
         0xffffffffffffffff);
  assert(string_cast<unsigned long long>(U"0xffffffffffffffff") ==
         0xffffffffffffffff);
  assert(string_cast<unsigned long long>(u"0xffffffffffffffff") ==
         0xffffffffffffffff);
  assert(string_cast<unsigned long long>(L"0xffffffffffffffff") ==
         0xffffffffffffffff);

  assert(string_cast<unsigned long long>("0xfffffffffffffffff") == 0);
  assert(string_cast<unsigned long long>(U"0xfffffffffffffffff") == 0);
  assert(string_cast<unsigned long long>(u"0xfffffffffffffffff") == 0);
  assert(string_cast<unsigned long long>(L"0xfffffffffffffffff") == 0);

  return true;
}

#endif

}  // end namespace StringCast

using namespace StringCast;

#ifdef STRING_CAST_DISABLE_CXX17_DEPRACATINO_WARNINGS
#undef _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#endif

#pragma warning(pop)
#endif
