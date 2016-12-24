#pragma once
#ifdef WIN32
#include <string>
// posix io api
extern "C" {
	extern int __cdecl close(int _FileHandle);
	extern int __cdecl read(int _FileHandle, void * _DstBuf, unsigned int _MaxCharCount);
	extern __int64 __cdecl lseek64(int _FileHandle, __int64 _Offset, int _Origin);
	extern int __cdecl write(int _Filehandle, const void * _Buf, unsigned int _MaxCharCount);
	extern int __cdecl open_win32(const char * _Filename, int _OpenFlag, int access = 0);
#define open(...) open_win32(__VA_ARGS__)
	extern void* valloc(int n);
	extern void vfree(void *p);
	extern void logStack(std::string &out);
	struct _stat_win32 : public stat {};
	int _stat_win32(const char * _Filename, struct stat * _Stat);
#define stat _stat_win32
	FILE * fopen(const char * _Filename, const char * _Mode);
}
#endif