#pragma once
#include "tjsCommHead.h"
#include <string>
#include <vector>

struct TVPMemoryInfo
{ // all in kB
    unsigned long MemTotal;
    unsigned long MemFree;
    unsigned long SwapTotal;
    unsigned long SwapFree;
    unsigned long VirtualTotal;
    unsigned long VirtualUsed;
};

void TVPGetMemoryInfo(TVPMemoryInfo &m);
tjs_int TVPGetSystemFreeMemory(); // in MB
tjs_int TVPGetSelfUsedMemory(); // in MB

extern "C" int TVPShowSimpleMessageBox(const char * text, const char * caption, unsigned int nButton, const char **btnText); // C-style

int TVPShowSimpleMessageBox(const ttstr & text, const ttstr & caption, const std::vector<ttstr> &vecButtons);
int TVPShowSimpleMessageBox(const ttstr & text, const ttstr & caption);
int TVPShowSimpleMessageBoxYesNo(const ttstr & text, const ttstr & caption);

int TVPShowSimpleInputBox(ttstr &text, const ttstr &caption, const ttstr &prompt, const std::vector<ttstr> &vecButtons);

std::vector<std::string> TVPGetDriverPath();
std::vector<std::string> TVPGetAppStoragePath();
bool TVPCheckStartupPath(const std::string &path);
std::string TVPGetPackageVersionString();
void TVPExitApplication(int code);
void TVPCheckMemory();
const std::string &TVPGetInternalPreferencePath();
bool TVPDeleteFile(const std::string &filename);
bool TVPRenameFile(const std::string &from, const std::string &to);
bool TVPCopyFile(const std::string &from, const std::string &to);

void TVPShowIME(int x, int y, int w, int h);
void TVPHideIME();

void TVPRelinquishCPU();
void TVPPrintLog(const char *str);

void TVPFetchSDCardPermission(); // for android only

struct tTVP_stat {
	uint16_t st_mode;
	uint64_t st_size;
	uint64_t st_atime;
	uint64_t st_mtime;
	uint64_t st_ctime;
};

bool TVP_stat(const tjs_char *name, tTVP_stat &s);
bool TVP_stat(const char *name, tTVP_stat &s);
void TVP_utime(const char *name, time_t modtime);

void TVPSendToOtherApp(const std::string &filename);
