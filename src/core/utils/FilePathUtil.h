
#ifndef __FILE_PATH_UTIL_H__
#define __FILE_PATH_UTIL_H__

#include <string>
#include <stdlib.h>

#if 0
#include <shlwapi.h>
#include <winnetwk.h>
#endif

static std::string IncludeTrailingBackslash(const std::string & path)
{
	int n = path.length();

	if ( n == 0 )
		return "/";
	switch (path.c_str()[n - 1])
	{
	case '\\':
	case '/':
		return path;
	default:
		return path + '/';
	}
}

static ttstr IncludeTrailingBackslash(const ttstr& path) {
	int n = path.length();
	if (n == 0) return TJS_W("/");
	switch (path.c_str()[n - 1])
	{
	case '\\':
	case '/':
		return path;
	default:
		return path + '/';
	}

#if 0
	if (path[path.length() - 1] != L'\\') {
		return std::wstring(path+L"\\");
	}
	return std::wstring(path);
#endif
}
inline ttstr ExcludeTrailingBackslash(const ttstr& path) {
	if( path[path.length()-1] == L'\\' ) {
		return ttstr(path, path.length() - 1);
	}
	return path;
}

extern std::string ExtractFileDir(const std::string & FileName);
inline ttstr ExtractFileDir(const ttstr& path) {
	return ExtractFileDir(path.AsNarrowStdString());
#if 0
	wchar_t drive[_MAX_DRIVE];
	wchar_t dir[_MAX_DIR];
#ifdef _WIN32
	_wsplitpath_s(path.c_str(), drive, _MAX_DRIVE, dir,_MAX_DIR, NULL, 0, NULL, 0 );
#else
	wsplitpath(path.c_str(), drive, dir, NULL, NULL );
#endif
	std::wstring dirstr = std::wstring( dir );
	if( dirstr[dirstr.length()-1] != L'\\' ) {
		return std::wstring( drive ) + dirstr;
	} else {
		return std::wstring( drive ) + dirstr.substr(0,dirstr.length()-1);
	}
#endif
}
#if 0
inline std::wstring ExtractFilePath(const std::wstring& path) {
	wchar_t drive[MAX_PATH];
	wchar_t dir[MAX_PATH];
#ifdef _WIN32
	_wsplitpath_s(path.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);
#else
	wsplitpath(path.c_str(), drive, dir, NULL, NULL );
#endif
	return std::wstring(drive) + std::wstring(dir);
}

#define DirectoryExists TVPCheckExistentLocalFolder
#define FileExists TVPCheckExistentLocalFile
inline std::wstring ChangeFileExt( const std::wstring& path, const std::wstring& ext ) {
	wchar_t drive[_MAX_DRIVE];
	wchar_t dir[_MAX_DIR];
	wchar_t fname[_MAX_FNAME];
#ifdef _WIN32
	_wsplitpath_s( path.c_str(), drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, NULL, 0 );
#else
	wsplitpath( path.c_str(), drive, dir, fname, NULL );
#endif
	return std::wstring( drive ) + std::wstring( dir ) + std::wstring( fname ) + ext;
}
inline std::wstring ExtractFileName( const std::wstring& path ) {
	wchar_t fname[_MAX_FNAME];
	wchar_t ext[_MAX_EXT];
#ifdef _WIN32
	_wsplitpath_s( path.c_str(), NULL, 0, NULL, 0, fname, _MAX_FNAME, ext, _MAX_EXT );
#else
	wsplitpath( path.c_str(), NULL, NULL, fname, ext );
#endif
	return std::wstring( fname ) + std::wstring( ext );
}
inline std::wstring ExpandUNCFileName( const std::wstring& path ) {
#ifdef _WIN32
	std::wstring result;
	DWORD InfoSize = 0;
	if( ERROR_MORE_DATA == WNetGetUniversalName( path.c_str(), UNIVERSAL_NAME_INFO_LEVEL, NULL, &InfoSize) ) {
		UNIVERSAL_NAME_INFO* pInfo = reinterpret_cast<UNIVERSAL_NAME_INFO*>( ::GlobalAlloc(GMEM_FIXED, InfoSize) );
		DWORD ret = ::WNetGetUniversalName( path.c_str(), UNIVERSAL_NAME_INFO_LEVEL, pInfo, &InfoSize);
		if( NO_ERROR == ret ) {
			result = std::wstring(pInfo->lpUniversalName);
		}
		::GlobalFree(pInfo);
	} else {
		wchar_t fullpath[_MAX_PATH];
		result = std::wstring( _wfullpath( fullpath, path.c_str(), _MAX_PATH ) );
	}
	return result;
#else
	#error Not Implemented yet.
#endif
}
#endif
#endif // __FILE_PATH_UTIL_H__
