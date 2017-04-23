#include "Platform.h"
#include "cocos2d/MainScene.h"
//#undef WIN32
#include <windows.h>
#include <mmsystem.h>
#include <Psapi.h>
#include <codecvt>
#include "StorageImpl.h"
#include "SysInitIntf.h"
#include "platform/CCFileUtils.h"
#include <sys/stat.h>
#include "Application.h"
#include "EventIntf.h"
#include "cocos/base/CCDirector.h"
#include <shellapi.h>
#include "XP3ArchiveRepack.h"
#include "RenderManager.h"
#include <sys/utime.h>

#pragma comment(lib,"psapi.lib")

tjs_int TVPGetSystemFreeMemory()
{
	MEMORYSTATUS info;
	GlobalMemoryStatus(&info);
	return info.dwAvailPhys / (1024 * 1024);
}

tjs_int TVPGetSelfUsedMemory()
{
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return info.WorkingSetSize / (1024 * 1024);
}

void TVPGetMemoryInfo(TVPMemoryInfo &m)
{
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);

    m.MemTotal = status.dwTotalPhys / 1024;
    m.MemFree = status.dwAvailPhys / 1024;
    m.SwapTotal = status.dwTotalPageFile / 1024;
    m.SwapFree = status.dwAvailPageFile / 1024;
    m.VirtualTotal = status.dwTotalVirtual / 1024;
    m.VirtualUsed = (status.dwTotalVirtual - status.dwAvailVirtual) / 1024;
}

// int gettimeofday(struct timeval * val, struct timezone *)
// {
// 	if (val)
// 	{
// 		LARGE_INTEGER liTime, liFreq;
// 		QueryPerformanceFrequency(&liFreq);
// 		QueryPerformanceCounter(&liTime);
// 		val->tv_sec = (long)(liTime.QuadPart / liFreq.QuadPart);
// 		val->tv_usec = (long)(liTime.QuadPart * 1000000.0 / liFreq.QuadPart - val->tv_sec * 1000000.0);
// 	}
// 	return 0;
// }

void *dlopen(const char *filename, int flag) {
	return (void*)LoadLibraryA(filename);
}

void *dlsym(void* handle, const char *funcname) {
	return (void *)GetProcAddress((HMODULE)handle, funcname);
}

extern "C" int usleep(unsigned long us) {
	Sleep(us / 1000);
	return 0;
}

//extern "C" __declspec(dllimport) int __cdecl __wgetmainargs(int * _Argc, wchar_t *** _Argv, wchar_t *** _Env, int _DoWildCard, void * _StartInfo);
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
std::string TVPGetDefaultFileDir() {
	wchar_t buf[MAX_PATH];
	_wgetcwd(buf, sizeof(buf) / sizeof(buf[0]));
	wchar_t *p = buf;
	while (*p) {
		if (*p == '\\') *p = '/';
		++p;
	}
	return converter.to_bytes(buf);
}

int TVPCheckArchive(const ttstr &localname);
void TVPCheckAndSendDumps(const std::string &dumpdir, const std::string &packageName, const std::string &versionStr);
bool TVPCheckStartupArg() {
	int argc;
	wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
//	__wgetmainargs(&argc, &argv, &env, 0, &info);
	TVPCheckAndSendDumps(TVPGetDefaultFileDir() + "/dumps", "win32-test", "test");
	if (argc > 1) {
		if (TVPCheckExistentLocalFile(argv[1])) {
			if (TVPCheckArchive(argv[1]) == 1) {
				TVPMainScene::GetInstance()->startupFrom(converter.to_bytes(argv[1]));
				return true;
			}
			return false;
		}
		bool bootable = false;
		TVPListDir(converter.to_bytes(argv[1]), [&](const std::string &_name, int mask) {
			if (mask & (S_IFREG)) {
				std::string name(_name);
				std::transform(name.begin(), name.end(), name.begin(), [](int c)->int {
					if (c <= 'Z' && c >= 'A')
						return c - ('A' - 'a');
					return c;
				});
				if (name == "startup.tjs") {
					bootable = true;
				}
			}
		});
		for (int i = 2; i < argc; ++i) {
			std::wstring str = argv[i];
			size_t pos = str.find(L'=');
			if (pos == str.npos) {
				TVPSetCommandLine(argv[i], "yes");
			} else {
				ttstr val = str.c_str() + pos + 1;
				TVPSetCommandLine(str.substr(0, pos).c_str(), val);
			}
		}
		if (bootable) {
			TVPMainScene::GetInstance()->startupFrom(converter.to_bytes(argv[1]));
			return true;
		}
	}
	return false;
}

int TVPShowSimpleMessageBox(const ttstr & text, const ttstr & caption, const std::vector<ttstr> &vecButtons)
{
	// there has no implement under android
	switch (vecButtons.size()) {
	case 1:
		MessageBoxW(0, text.c_str(), caption.c_str(), /*MB_OK*/0);
		return 0;
		break;
	case 2:
		switch (MessageBoxW(0, text.c_str(), caption.c_str(), /*MB_YESNO*/4)) {
		case 6:
			return 0;
		default:
			return 1;
		}
		break;
	}
	return -1;
}

extern "C" int TVPShowSimpleMessageBox(const char *pszText, const char *pszTitle, unsigned int nButton, const char **btnText) {
	std::vector<ttstr> vecButtons;
	for (unsigned int i = 0; i < nButton; ++i) {
		vecButtons.emplace_back(btnText[i]);
	}
	return TVPShowSimpleMessageBox(pszText, pszTitle, vecButtons);
}

std::vector<std::string> TVPGetDriverPath() {
	std::vector<std::string> ret;
	char drv[4] = { 'C', ':', '/', 0 };
	for (char c = 'C'; c <= 'Z'; ++c) {
		drv[0] = c;
		switch (GetDriveTypeA(drv)) {
		case DRIVE_REMOVABLE:
		case DRIVE_FIXED:
		case DRIVE_REMOTE:
			ret.emplace_back(drv);
			break;
		}
	}
	return ret;
}

std::vector<std::string> TVPGetAppStoragePath() {
	std::vector<std::string> ret;
	ret.emplace_back(TVPGetDefaultFileDir());
	return ret;
}

bool TVPCheckStartupPath(const std::string &path) { return true; }

std::string TVPGetPackageVersionString() {
	return "win32";
}

void TVPControlAdDialog(int adType, int arg1, int arg2) {}
void TVPForceSwapBuffer() {}


//---------------------------------------------------------------------------
// TVPCreateFolders
//---------------------------------------------------------------------------
static bool _TVPCreateFolders(const ttstr &folder)
{
	// create directories along with "folder"
	if (folder.IsEmpty()) return true;

	if (TVPCheckExistentLocalFolder(folder))
		return true; // already created

	const tjs_char *p = folder.c_str();
	tjs_int i = folder.GetLen() - 1;

	if (p[i] == TJS_W(':')) return true;

	while (i >= 0 && (p[i] == TJS_W('/') || p[i] == TJS_W('\\'))) i--;

	if (i >= 0 && p[i] == TJS_W(':')) return true;

	for (; i >= 0; i--)
	{
		if (p[i] == TJS_W(':') || p[i] == TJS_W('/') ||
			p[i] == TJS_W('\\'))
			break;
	}

	ttstr parent(p, i + 1);
	if (!TVPCreateFolders(parent)) return false;

	return !_wmkdir(folder.c_str());

}
//---------------------------------------------------------------------------

bool TVPCreateFolders(const ttstr &folder)
{
	if (folder.IsEmpty()) return true;

	const tjs_char *p = folder.c_str();
	tjs_int i = folder.GetLen() - 1;

	if (p[i] == TJS_W(':')) return true;

	if (p[i] == TJS_W('/') || p[i] == TJS_W('\\')) i--;

	return _TVPCreateFolders(ttstr(p, i + 1));
}

bool TVPWriteDataToFile(const ttstr &filepath, const void *data, unsigned int len) {
	FILE* handle = _wfopen(filepath.c_str(), L"wb");
	if (handle) {
		bool ret = fwrite(data, 1, len, handle) == len;
		fclose(handle);
		return ret;
	}
	return false;
}

std::string TVPGetCurrentLanguage() {
	LANGID lid = GetUserDefaultUILanguage();
	const LCID locale_id = MAKELCID(lid, SORT_DEFAULT);
	char code[10] = { 0 };
	char country[10] = { 0 };
	GetLocaleInfoA(locale_id, LOCALE_SISO639LANGNAME, code, sizeof(code));
	GetLocaleInfoA(locale_id, LOCALE_SISO3166CTRYNAME, country, sizeof(country));
	std::string ret = code;
	if (country[0]) {
		for (int i = 0; i < sizeof(country) && country[i]; ++i) {
			char c = country[i];
			if (c <= 'Z' && c >= 'A') {
				country[i] += 'a' - 'A';
			}
		}
		ret += "_";
		ret += country;
	}
	return ret;
}

extern tTJS *TVPScriptEngine;
void TVPReleaseFontLibrary();
void TVPExitApplication(int code) {
	// clear some static data for memory leak detect
	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
	if (!TVPIsSoftwareRenderManager())
		iTVPTexture2D::RecycleProcess();

// 	if (TVPScriptEngine) TVPScriptEngine->Cleanup();
// 	TVPReleaseFontLibrary();
// 	delete ::Application;
// 	TVPMainScene::GetInstance()->removeFromParent();
	exit(code);
}

// const std::string &TVPGetInternalPreferencePath() {
// 	static std::string ret(cocos2d::FileUtils::getInstance()->getWritablePath());
// 	return ret;
// }

bool TVPDeleteFile(const std::string &filename)
{
	return _wunlink(ttstr(filename).c_str()) == 0;
}

bool TVPRenameFile(const std::string &from, const std::string &to)
{
#ifdef WIN32
	tjs_int ret = _wrename(ttstr(from).c_str(), ttstr(to).c_str());
#else
	tjs_int ret = rename(from.c_str(), to.c_str());
#endif
	return !ret;
}

void TVPProcessInputEvents() {}
void TVPShowIME(int x, int y, int w, int h) {}
void TVPHideIME() {}

void TVPRelinquishCPU(){ Sleep(0); }

tjs_uint32 TVPGetRoughTickCount32()
{
	return timeGetTime();
}

void TVPPrintLog(const char *str) {
	printf("%s", str);
}

bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
	struct _stat64 t;
	bool ret = !_wstat64(name, &t);
	s.st_mode = t.st_mode;
	s.st_size = t.st_size;
	s.st_atime = t.st_atime;
	s.st_mtime = t.st_mtime;
	s.st_ctime = t.st_ctime;
	return ret;
}

bool TVP_stat(const char *name, tTVP_stat &s) {
	ttstr filename(name);
	return TVP_stat(filename.c_str(), s);
}

void TVP_utime(const char *name, time_t modtime) {
	_utimbuf utb;
	utb.modtime = modtime;
	utb.actime = modtime;
	ttstr filename(name);
	_wutime(filename.c_str(), &utb);
}
