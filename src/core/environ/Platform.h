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
bool TVPDeleteFile(const ttstr &filename);
bool TVPRenameFile(const ttstr &from, const ttstr &to);

void TVPShowIME(int x, int y, int w, int h);
void TVPHideIME();

void TVPRelinquishCPU();
void TVPPrintLog(const char *str);
