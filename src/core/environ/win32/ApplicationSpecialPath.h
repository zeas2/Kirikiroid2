
#ifndef __APPLICATION_SPECIAL_PATH_H__
#define __APPLICATION_SPECIAL_PATH_H__

//#include <shlobj.h>
#include "FilePathUtil.h"
#include "StorageIntf.h"

class ApplicationSpecialPath {
public:
#if 0
	static std::wstring GetSpecialFolderPath(int csidl) {
		wchar_t path[MAX_PATH+1];
		if(!SHGetSpecialFolderPath(NULL, path, csidl, false))
			return std::wstring();
		return std::wstring(path);
	}
	static inline std::wstring GetPersonalPath() {
		std::wstring path = GetSpecialFolderPath(CSIDL_PERSONAL);
		if( path.empty() ) path = GetSpecialFolderPath(CSIDL_APPDATA);

		if(path != L"") {
			return path;
		}
		return L"";
	}
	static inline std::wstring GetAppDataPath() {
		std::wstring path = GetSpecialFolderPath(CSIDL_APPDATA);
		if(path != L"" ) {
			return path;
		}
		return L"";
	}
	static inline std::wstring GetSavedGamesPath() {
		std::wstring result;
		PWSTR ppszPath = NULL;
		HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &ppszPath);
		if( hr == S_OK ) {
			result = std::wstring(ppszPath);
			::CoTaskMemFree( ppszPath );
		}
		return result;
	}
#endif
	static inline ttstr ReplaceStringAll(ttstr src, const ttstr& target, const ttstr& dest) {
		src.Replace(target, dest);
		return src;
	}

#if 0
	static inline ttstr GetConfigFileName(const ttstr& exename) {
		return ChangeFileExt(exename.AsNarrowStdString(), ".cf");
	}
#endif
	static ttstr GetDataPathDirectory(ttstr datapath, const ttstr& exename) {
		ttstr nativeDataPath = TVPGetAppPath();
		TVPGetLocalName(nativeDataPath);
		nativeDataPath += "/savedata/";
		return nativeDataPath;
#if 0
		if(datapath == L"" ) datapath = std::wstring(L"$(exepath)\\savedata");

		std::wstring exepath = ExcludeTrailingBackslash(ExtractFileDir(exename));
		std::wstring personalpath = ExcludeTrailingBackslash(GetPersonalPath());
		std::wstring appdatapath = ExcludeTrailingBackslash(GetAppDataPath());
		std::wstring savedgamespath = ExcludeTrailingBackslash(GetSavedGamesPath());
		if(personalpath == L"") personalpath = exepath;
		if(appdatapath == L"") appdatapath = exepath;
		if(savedgamespath == L"") savedgamespath = exepath;

		OSVERSIONINFO osinfo;
		osinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
		::GetVersionEx(&osinfo);

		bool vista_or_later = osinfo.dwPlatformId == VER_PLATFORM_WIN32_NT && osinfo.dwMajorVersion >= 6;

		std::wstring vistapath = vista_or_later ? appdatapath : exepath;

		datapath = ReplaceStringAll(datapath, L"$(exepath)", exepath);
		datapath = ReplaceStringAll(datapath, L"$(personalpath)", personalpath);
		datapath = ReplaceStringAll(datapath, L"$(appdatapath)", appdatapath);
		datapath = ReplaceStringAll(datapath, L"$(vistapath)", vistapath);
		datapath = ReplaceStringAll(datapath, L"$(savedgamespath)", savedgamespath);
		return IncludeTrailingBackslash(ExpandUNCFileName(datapath));
#endif
	}
#if 0
	static ttstr GetUserConfigFileName(const ttstr& datapath, const ttstr& exename) {
		// exepath, personalpath, appdatapath
		return GetDataPathDirectory(datapath, exename) + ExtractFileName(ChangeFileExt(exename.AsNarrowStdString(), ".cfu"));
	}
#endif
};


#endif // __APPLICATION_SPECIAL_PATH_H__
