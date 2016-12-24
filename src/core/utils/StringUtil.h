
#ifndef __STRING_UTILITY_H__
#define __STRING_UTILITY_H__

#include <string>
#include <iostream>
#include <algorithm>
#include <locale>

struct equal_char_ignorecase {
	inline bool operator()(char x, char y) const {
		std::locale loc;
		return std::tolower(x,loc) == std::tolower(y,loc);
	}
};

inline bool icomp( const std::string& x, const std::string& y ) {
	return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), equal_char_ignorecase());
}

struct equal_wchar_ignorecase {
	inline bool operator()(wchar_t x, wchar_t y) const {
		std::locale loc;
		return std::tolower(x,loc) == std::tolower(y,loc);
	}
};
inline bool icomp( const std::wstring& x, const std::wstring& y ) {
	return x.size() == y.size() && std::equal(x.begin(), x.end(), y.begin(), equal_wchar_ignorecase());
}

static int ttstr_find_first_not_of(const ttstr &str, const ttstr& r) {
	const tjs_char *const pend = str.c_str() + str.length();
	for (const tjs_char *p = str.c_str(); p < pend; ++p) {
		if (r.IndexOf(*p) != -1) {
			return p - str.c_str();
		}
	}
	return -1;
}

static int ttstr_find_last_not_of(const ttstr &str, const ttstr& r) {
	const tjs_char *pbegin = str.c_str();
	for (const tjs_char *p = str.c_str() + str.length();; --p) {
		if (r.IndexOf(*p) != -1)
			return (p - str.c_str());	// found a match
		else if (p == pbegin)
			break;	// at beginning, no more chance for match
	}
	return -1;
}

inline std::string Trim( const std::string& val ) {
	static const char* TRIM_STR=" \01\02\03\04\05\06\a\b\t\n\v\f\r\x0E\x0F\x7F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F";
	std::string::size_type pos = val.find_first_not_of( TRIM_STR );
	std::string::size_type lastpos = val.find_last_not_of( TRIM_STR );
	if( pos == lastpos ) {
		if( pos == std::string::npos ) {
			return val;
		} else {
			return val.substr(pos,1);
		}
	} else {
		std::string::size_type len = lastpos - pos + 1;
		return val.substr(pos,len);
	}
}
inline ttstr Trim(const ttstr& val) {
	static const tjs_char* TRIM_STR=TJS_W(" \01\02\03\04\05\06\a\b\t\n\v\f\r\x0E\x0F\x7F\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F");
	int pos = ttstr_find_first_not_of(val, TRIM_STR);
	int lastpos = ttstr_find_last_not_of(val, TRIM_STR);
	if( pos == lastpos ) {
		if( pos == std::wstring::npos ) {
			return val;
		} else {
			return val.SubString(pos,1);
		}
	} else {
		std::wstring::size_type len = lastpos - pos + 1;
		return val.SubString(pos, len);
	}
}

#endif // __STRING_UTILITY_H__
