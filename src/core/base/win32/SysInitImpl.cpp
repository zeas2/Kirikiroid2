//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Initialization and Uninitialization
//---------------------------------------------------------------------------
#include "tjsCommHead.h"


#include "FilePathUtil.h"
// #include <delayimp.h>
// #include <mmsystem.h>
// #include <objbase.h>
// #include <commdlg.h>

#include "SysInitImpl.h"
#include "StorageIntf.h"
#include "StorageImpl.h"
#include "MsgIntf.h"
#include "GraphicsLoaderIntf.h"
#include "SystemControl.h"
#include "DebugIntf.h"
#include "tjsLex.h"
#include "LayerIntf.h"
#include "Random.h"
#include "DetectCPU.h"
#include "XP3Archive.h"
#include "ScriptMgnIntf.h"
#include "XP3Archive.h"
//#include "VersionFormUnit.h"
#include "EmergencyExit.h"

//#include "tvpgl_ia32_intf.h"

#include "BinaryStream.h"
#include "Application.h"
#include "Exception.h"
#include "ApplicationSpecialPath.h"
//#include "resource.h"
//#include "ConfigFormUnit.h"
#include "TickCount.h"
#ifdef IID
#undef IID
#endif
#define uint32_t unsigned int
#include <thread>
#undef uint32_t
#include "Platform.h"
#include "ConfigManager/IndividualConfigManager.h"

//---------------------------------------------------------------------------
// global data
//---------------------------------------------------------------------------
ttstr TVPNativeProjectDir;
ttstr TVPNativeDataPath;
bool TVPProjectDirSelected = false;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// System security options
//---------------------------------------------------------------------------
// system security options are held inside the executable, where
// signature checker will refer. This enables the signature checker
// (or other security modules like XP3 encryption module) to check
// the changes which is not intended by the contents author.
const static char TVPSystemSecurityOptions[] =
"-- TVPSystemSecurityOptions disablemsgmap(0):forcedataxp3(0):acceptfilenameargument(0) --";
//---------------------------------------------------------------------------
int GetSystemSecurityOption(const char *name)
{
	size_t namelen = TJS_nstrlen(name);
	const char *p = TJS_nstrstr(TVPSystemSecurityOptions, name);
	if(!p) return 0;
	if(p[namelen] == '(' && p[namelen + 2] == ')')
		return p[namelen+1] - '0';
	return 0;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// delayed DLL load procedure hook
//---------------------------------------------------------------------------
// for supporting of "_inmm.dll" (C) irori
// http://www.geocities.co.jp/Playtown-Domino/8282/
//---------------------------------------------------------------------------
/*
note:
	_inmm.dll is a replacement of winmm.dll ( windows multimedia system dll ).
	_inmm.dll enables "MCI CD-DA supporting applications" to play musics using
	various way, including midi, mp3, wave or digital CD-DA, by applying a
	patch on those applications.

	TVP(kirikiri) system has a special structure of executable file --
	delayed loading of winmm.dll, in addition to compressed code/data area
	by the UPX executable packer.
	_inmm.dll's patcher can not recognize TVP's import area.

	So we must implement supporting of _inmm.dll alternatively.

	This function only works when -_inmm=yes or -inmm=yes option is specified at
	command line or embeded options area.
*/
//---------------------------------------------------------------------------
#if 0
static HMODULE _inmm = NULL;
static FARPROC WINAPI DllLoadHook(dliNotification dliNotify,  DelayLoadInfo * pdli)
{
	if(dliNotify == dliNotePreLoadLibrary)
	{
		if(!stricmp(pdli->szDll, "winmm.dll"))
		{
			HMODULE mod = LoadLibrary("_inmm.dll");
			if(mod) _inmm = mod;
			return (FARPROC)mod;
		}
	}
	else if(dliNotify == dliNotePreGetProcAddress)
	{
		if(_inmm == pdli->hmodCur)
		{
			char buf[256];
			buf[1] = 0;
			TJS_nstrcpy(buf, pdli->dlp.szProcName);
			buf[0] = '_';
			return (FARPROC)GetProcAddress(_inmm, buf);
		}
	}

	return 0;
}
//---------------------------------------------------------------------------
static void RegisterDllLoadHook(void)
{
	bool flag = false;
	tTJSVariant val;
	if( TVPGetCommandLine(TJS_W("-_inmm"), &val) ||
		TVPGetCommandLine(TJS_W("-inmm" ), &val) )
	{
		// _inmm support
		ttstr str(val);
		if(str == TJS_W("yes"))
			flag = true;
	}
	if(flag) __pfnDliNotifyHook = DllLoadHook;
}
//---------------------------------------------------------------------------
#endif




#ifdef TVP_REPORT_HW_EXCEPTION
//---------------------------------------------------------------------------
// Hardware Exception Report Related
//---------------------------------------------------------------------------
// TVP's Hardware Exception Report comes with hacking RTL source.
// insert following code into rtl/soruce/except/xx.cpp
/*
typedef void __cdecl (*__dee_hacked_getExceptionObjectHook_type)(int ErrorCode,
		EXCEPTION_RECORD *P, unsigned long osEsp, unsigned long osERR, PCONTEXT ctx);
static __dee_hacked_getExceptionObjectHook_type __dee_hacked_getExceptionObjectHook = NULL;

extern "C"
{
	__dee_hacked_getExceptionObjectHook_type
		__cdecl __dee_hacked_set_getExceptionObjectHook(
		__dee_hacked_getExceptionObjectHook_type handler)
	{
		__dee_hacked_getExceptionObjectHook_type oldhandler;
		oldhandler = __dee_hacked_getExceptionObjectHook;
		__dee_hacked_getExceptionObjectHook = handler;
		return oldhandler;
	}
}
*/
// and insert following code into getExceptionObject
/*
	if(__dee_hacked_getExceptionObjectHook)
		__dee_hacked_getExceptionObjectHook(ErrorCode, P, osEsp, osERR, ctx);
*/
//---------------------------------------------------------------------------
/*
typedef void __cdecl (*__dee_hacked_getExceptionObjectHook_type)(int ErrorCode,
		EXCEPTION_RECORD *P, unsigned long osEsp, unsigned long osERR, PCONTEXT ctx);
extern "C"
{
	extern __dee_hacked_getExceptionObjectHook_type
		__cdecl __dee_hacked_set_getExceptionObjectHook(
		__dee_hacked_getExceptionObjectHook_type handler);
}
*/

//---------------------------------------------------------------------------
// data
#define TVP_HWE_MAX_CODES_AT_EIP 96
#define TVP_HWE_MAX_STACK_AT_ESP 80
#define TVP_HWE_MAX_STACK_DATA_DUMP  16
#define TVP_HWE_MAX_CALL_TRACE 32
#define TVP_HWE_MAX_CALL_CODE_DUMP 26
static bool TVPHWExcRaised = false;
struct tTVPHWExceptionData
{
	tjs_int Code;
	tjs_uint8 *EIP;
	tjs_uint32 *ESP;
	ULONG_PTR AccessFlag; // for EAccessViolation (0=read, 1=write, 8=execute)
	void *AccessTarget; // for EAccessViolation
	CONTEXT Context; // OS exception context
	wchar_t Module[MAX_PATH]; // module name which caused the exception

	tjs_uint8 CodesAtEIP[TVP_HWE_MAX_CODES_AT_EIP];
	tjs_int CodesAtEIPLen;
	void * StackAtESP[TVP_HWE_MAX_STACK_AT_ESP];
	tjs_int StackAtESPLen;
	tjs_uint8 StackDumps[TVP_HWE_MAX_STACK_AT_ESP][TVP_HWE_MAX_STACK_DATA_DUMP];
	tjs_int StackDumpsLen[TVP_HWE_MAX_STACK_AT_ESP];

	void * CallTrace[TVP_HWE_MAX_CALL_TRACE];
	tjs_int CallTraceLen;
	tjs_uint8 CallTraceDumps[TVP_HWE_MAX_CALL_TRACE][TVP_HWE_MAX_CALL_CODE_DUMP];
	tjs_int CallTraceDumpsLen[TVP_HWE_MAX_CALL_TRACE];
};
static tTVPHWExceptionData TVPLastHWExceptionData;

HANDLE TVPHWExceptionLogHandle = NULL;
//---------------------------------------------------------------------------
static wchar_t TVPHWExceptionLogFilename[MAX_PATH];

static void TVPWriteHWELogFile()
{
	TVPEnsureDataPathDirectory();
	TJS_strcpy(TVPHWExceptionLogFilename, TVPNativeDataPath.c_str());
	TJS_strcat(TVPHWExceptionLogFilename, L"hwexcept.log");
	TVPHWExceptionLogHandle = CreateFile(TVPHWExceptionLogFilename, GENERIC_WRITE,
		FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

	if(TVPHWExceptionLogHandle == INVALID_HANDLE_VALUE) return;
	DWORD filesize;
	filesize = GetFileSize(TVPHWExceptionLogHandle, NULL);
	SetFilePointer(TVPHWExceptionLogHandle, filesize, NULL, FILE_BEGIN);

	// write header
	const wchar_t headercomment[] =
		L"THIS IS A HARDWARE EXCEPTION LOG FILE OF KIRIKIRI. "
		L"PLEASE SEND THIS FILE TO THE AUTHOR WITH *.console.log FILE. ";
	DWORD written = 0;
	for(int i = 0; i < 4; i++)
		WriteFile(TVPHWExceptionLogHandle, L"----", 4*sizeof(wchar_t), &written, NULL);
	WriteFile(TVPHWExceptionLogHandle, headercomment, sizeof(headercomment)-1,
		&written, NULL);
	for(int i = 0; i < 4; i++)
		WriteFile(TVPHWExceptionLogHandle, L"----", 4*sizeof(wchar_t), &written, NULL);
		

	// write version
	WriteFile(TVPHWExceptionLogHandle, &TVPVersionMajor,
		sizeof(TVPVersionMajor), &written, NULL);
	WriteFile(TVPHWExceptionLogHandle, &TVPVersionMinor,
		sizeof(TVPVersionMinor), &written, NULL);
	WriteFile(TVPHWExceptionLogHandle, &TVPVersionRelease,
		sizeof(TVPVersionRelease), &written, NULL);
	WriteFile(TVPHWExceptionLogHandle, &TVPVersionBuild,
		sizeof(TVPVersionBuild), &written, NULL);

	// write tTVPHWExceptionData
	WriteFile(TVPHWExceptionLogHandle, &TVPLastHWExceptionData,
		sizeof(TVPLastHWExceptionData), &written, NULL);


	// close the handle
	if(TVPHWExceptionLogHandle != INVALID_HANDLE_VALUE)
		CloseHandle(TVPHWExceptionLogHandle);

}
//---------------------------------------------------------------------------
//void __cdecl TVP__dee_hacked_getExceptionObjectHook(int ErrorCode,
//		EXCEPTION_RECORD *P, unsigned long osEsp, unsigned long osERR, PCONTEXT ctx)
#ifdef TJS_64BIT_OS
void TVPHandleSEHException( int ErrorCode, EXCEPTION_RECORD *P, unsigned long long osEsp, PCONTEXT ctx)
#else
void TVPHandleSEHException( int ErrorCode, EXCEPTION_RECORD *P, unsigned long osEsp, PCONTEXT ctx)
#endif
{
	// exception hook function
	int len;
	tTVPHWExceptionData * d = &TVPLastHWExceptionData;

	d->Code = ErrorCode;

	// get AccessFlag and AccessTarget
	if(d->Code == 11) // EAccessViolation
	{
		d->AccessFlag = P->ExceptionInformation[0];
		d->AccessTarget = (void*)P->ExceptionInformation[1];
	}

	// get OS context
	if(!IsBadReadPtr(ctx, sizeof(*ctx)))
	{
		memcpy(&(d->Context), ctx, sizeof(*ctx));
	}
	else
	{
		memset(&(d->Context), 0, sizeof(*ctx));
	}

	// get codes at eip
	d->EIP = (tjs_uint8*)P->ExceptionAddress;
	len = TVP_HWE_MAX_CODES_AT_EIP;

	while(len)
	{
		if(!IsBadReadPtr(d->EIP, len))
		{
			memcpy(d->CodesAtEIP, d->EIP, len);
			d->CodesAtEIPLen = len;
			break;
		}
		len--;
	}

	// get module name
	MEMORY_BASIC_INFORMATION mbi;
	VirtualQuery(d->EIP, &mbi, sizeof(mbi));
	if(mbi.State == MEM_COMMIT)
	{
		if(!GetModuleFileName((HMODULE)mbi.AllocationBase, d->Module,
			MAX_PATH))
		{
			d->Module[0] = 0;
		}
	}
	else
	{
		d->Module[0] = 0;
	}


	// get stack at esp
	d->ESP = (tjs_uint32*)osEsp;
	len = TVP_HWE_MAX_STACK_AT_ESP;

	while(len)
	{
		if(!IsBadReadPtr(d->ESP, len * sizeof(tjs_uint32)))
		{
			memcpy(d->StackAtESP, d->ESP, len * sizeof(tjs_uint32));
			d->StackAtESPLen = len;
			break;
		}
		len--;
	}

	// get data pointed by each stack data
	for(tjs_int i = 0; i<d->StackAtESPLen; i++)
	{
		void * base = d->StackAtESP[i];
		len = TVP_HWE_MAX_STACK_DATA_DUMP;
		while(len)
		{
			if(!IsBadReadPtr(base, len))
			{
				memcpy(d->StackDumps[i], base, len);
				d->StackDumpsLen[i] = len;
				break;
			}
			len--;
		}
	}

	// get call trace at esp
	d->CallTraceLen = 0;
	tjs_int p = 0;
	while(d->CallTraceLen < TVP_HWE_MAX_CALL_TRACE)
	{
		if(IsBadReadPtr(d->ESP + p, sizeof(tjs_uint32)))
			break;

		if(!IsBadReadPtr((void*)d->ESP[p], 4))
		{
			VirtualQuery((void*)d->ESP[p], &mbi, sizeof(mbi));
			if(mbi.State == MEM_COMMIT)
			{
				wchar_t module[MAX_PATH];
				if(::GetModuleFileName((HMODULE)mbi.AllocationBase, module, MAX_PATH))
				{
					tjs_uint8 buf[16];
					if((DWORD)d->ESP[p] >= 16 &&
						!IsBadReadPtr((void*)((DWORD)d->ESP[p] - 16), 16))
					{
						memcpy(buf, (void*)((DWORD)d->ESP[p] - 16), 16);
						bool flag = false;
						if(buf[11] == 0xe8) flag = true;
						if(!flag)
						{
							for(tjs_int i = 0; i<15; i++)
							{
								if(buf[i] == 0xff && (buf[i+1] & 0x38) == 0x10)
								{
									flag = true;
									break;
								}
							}
						}
						if(flag)
						{
							// this seems to be a call code
							d->CallTrace[d->CallTraceLen] = (void *) d->ESP[p];
							d->CallTraceLen ++;

						}
					}
				}
			}
		}

		p ++;
	}

	// get data pointed by each call trace data
	for(tjs_int i = 0; i<d->CallTraceLen; i++)
	{
		void * base = d->CallTrace[i];
		len = TVP_HWE_MAX_STACK_DATA_DUMP;
		while(len)
		{
			if(!IsBadReadPtr(base, len))
			{
				memcpy(d->CallTraceDumps[i], base, len);
				d->CallTraceDumpsLen[i] = len;
				break;
			}
			len--;
		}
	}

	TVPHWExcRaised = true;

	TVPWriteHWELogFile();
}
//---------------------------------------------------------------------------
static void TVPDumpCPUFlags(ttstr &line, DWORD flags, DWORD bit, tjs_char *name)
{
	line += name;
	if(flags & bit)
		line += TJS_W("+ ");
	else
		line += TJS_W("- ");
}
//---------------------------------------------------------------------------
void TVPDumpOSContext(const CONTEXT &ctx)
{
	// dump OS context block
	static const int BUF_SIZE = 256;
	tjs_char buf[BUF_SIZE];

	// mask FP exception
	TJSSetFPUE();

	// - context flags
	ttstr line;
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Context Flags : 0x%08X [ "), ctx.ContextFlags);
	line += buf;
	if(ctx.ContextFlags & CONTEXT_DEBUG_REGISTERS)
		line += TJS_W("CONTEXT_DEBUG_REGISTERS ");
	if(ctx.ContextFlags & CONTEXT_FLOATING_POINT)
		line += TJS_W("CONTEXT_FLOATING_POINT ");
	if(ctx.ContextFlags & CONTEXT_SEGMENTS)
		line += TJS_W("CONTEXT_SEGMENTS ");
	if(ctx.ContextFlags & CONTEXT_INTEGER)
		line += TJS_W("CONTEXT_INTEGER ");
	if(ctx.ContextFlags & CONTEXT_CONTROL)
		line += TJS_W("CONTEXT_CONTROL ");
#ifndef TJS_64BIT_OS
	if(ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS)
		line += TJS_W("CONTEXT_EXTENDED_REGISTERS ");
#endif
	line += TJS_W("]");

	TVPAddLog(line);


	// - debug registers
#ifndef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE,
		TJS_W("Debug Registers   : ")
		TJS_W("0:0x%08X  ")
		TJS_W("1:0x%08X  ")
		TJS_W("2:0x%08X  ")
		TJS_W("3:0x%08X  ")
		TJS_W("6:0x%08X  ")
		TJS_W("7:0x%08X  "),
			ctx.Dr0, ctx.Dr1, ctx.Dr2, ctx.Dr3, ctx.Dr6, ctx.Dr7);
#else
	TJS_snprintf(buf, BUF_SIZE,
		TJS_W("Debug Registers   : ")
		TJS_W("0:0x%016lx  ")
		TJS_W("1:0x%016lx  ")
		TJS_W("2:0x%016lx  ")
		TJS_W("3:0x%016lx  ")
		TJS_W("6:0x%016lx  ")
		TJS_W("7:0x%016lx  "),
			ctx.Dr0, ctx.Dr1, ctx.Dr2, ctx.Dr3, ctx.Dr6, ctx.Dr7);
#endif
	TVPAddLog(buf);


	// - Segment registers
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Segment Registers : GS:0x%04X  FS:0x%04X  ES:0x%04X  DS:0x%04X  CS:0x%04X  SS:0x%04X"),
		ctx.SegGs, ctx.SegFs, ctx.SegEs, ctx.SegDs, ctx.SegCs, ctx.SegSs);
	TVPAddLog(buf);

	// - Generic Integer Registers
#ifdef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Integer Registers : RAX:0x%016lx  RBX:0x%016lx  RCX:0x%016lx  RDX:0x%016lx"),
		ctx.Rax, ctx.Rbx, ctx.Rcx, ctx.Rdx);
	TVPAddLog(buf);
	
	TJS_snprintf(buf, BUF_SIZE, TJS_W("R8 :0x%016lx  R9 :0x%016lx  R10:0x%016lx  R11:0x%016lx"), ctx.R8, ctx.R9, ctx.R10, ctx.R11);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("R12:0x%016lx  R13:0x%016lx  R14:0x%016lx  R15:0x%016lx"), ctx.R12, ctx.R13, ctx.R14, ctx.R15);
	TVPAddLog(buf);
#else
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Integer Registers : EAX:0x%08X  EBX:0x%08X  ECX:0x%08X  EDX:0x%08X"),
		ctx.Eax, ctx.Ebx, ctx.Ecx, ctx.Edx);
	TVPAddLog(buf);
#endif

	// - Index Registers
#ifdef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Index Registers   : RSI:0x%016lx  RDI:0x%016lx"),
		ctx.Rsi, ctx.Rdi);
#else
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Index Registers   : ESI:0x%08X  EDI:0x%08X"),
		ctx.Esi, ctx.Edi);
#endif
	TVPAddLog(buf);

	// - Pointer Registers
#ifdef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Pointer Registers : RBP:0x%016lx  RSP:0x%016lx  RIP:0x%016lx"),
		ctx.Rbp, ctx.Rsp, ctx.Rip);
#else
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Pointer Registers : EBP:0x%08X  ESP:0x%08X  EIP:0x%08X"),
		ctx.Ebp, ctx.Esp, ctx.Eip);
#endif
	TVPAddLog(buf);

	// - Flag Register
	TJS_snprintf(buf, BUF_SIZE, TJS_W("Flag Register     : 0x%08X [ "),
		ctx.EFlags);
	line = buf;
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 0), TJS_W("CF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 2), TJS_W("PF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 4), TJS_W("AF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 6), TJS_W("ZF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 7), TJS_W("SF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 8), TJS_W("TF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<< 9), TJS_W("IF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<10), TJS_W("DF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<11), TJS_W("OF"));
	TJS_snprintf(buf, BUF_SIZE, TJS_W("IO%d "), (ctx.EFlags >> 12) & 0x03);
	line += buf;
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<14), TJS_W("NF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<16), TJS_W("RF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<17), TJS_W("VM"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<18), TJS_W("AC"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<19), TJS_W("VF"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<20), TJS_W("VP"));
	TVPDumpCPUFlags(line, ctx.EFlags, (1<<21), TJS_W("ID"));
	line += TJS_W("]");
	TVPAddLog(line);

	// - FP registers

	// -- control words
#ifdef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Control Word : 0x%08X   FP Status Word : 0x%08X   FP Tag Word : 0x%08X"),
		ctx.FltSave.ControlWord, ctx.FltSave.StatusWord, ctx.FltSave.TagWord);
	TVPAddLog(buf);

	// -- offsets/selectors
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Error Offset : 0x%08X   FP Error Selector : 0x%08X"),
		ctx.FltSave.ErrorOffset, ctx.FltSave.ErrorSelector);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Data Offset  : 0x%08X   FP Data Selector  : 0x%08X"),
		ctx.FltSave.DataOffset, ctx.FltSave.DataSelector);

	// -- registers
	long double *ptr = (long double *)&(ctx.FltSave.FloatRegisters[0]);
	for(tjs_int i = 0; i < 8; i++)
	{
		TJS_snprintf(buf, BUF_SIZE, TJS_W("FP ST(%d) : %28.20Lg 0x%04X%016I64X"), i,
			ptr[i], (unsigned int)*(tjs_uint16*)(((tjs_uint8*)(ptr + i)) + 8),
			*(tjs_uint64*)(ptr + i));
		TVPAddLog(buf);
	}

	// -- Cr0NpxState
	TJS_snprintf(buf, BUF_SIZE,TJS_W("FP MX CSR   : 0x%08X"), ctx.FltSave.MxCsr);	//
	TVPAddLog(buf);
#else
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Control Word : 0x%08X   FP Status Word : 0x%08X   FP Tag Word : 0x%08X"),
		ctx.FloatSave.ControlWord, ctx.FloatSave.StatusWord, ctx.FloatSave.TagWord);
	TVPAddLog(buf);

	// -- offsets/selectors
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Error Offset : 0x%08X   FP Error Selector : 0x%08X"),
		ctx.FloatSave.ErrorOffset, ctx.FloatSave.ErrorSelector);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("FP Data Offset  : 0x%08X   FP Data Selector  : 0x%08X"),
		ctx.FloatSave.DataOffset, ctx.FloatSave.DataSelector);

	// -- registers
	long double *ptr = (long double *)&(ctx.FloatSave.RegisterArea[0]);
	for(tjs_int i = 0; i < 8; i++)
	{
		TJS_snprintf(buf, BUF_SIZE, TJS_W("FP ST(%d) : %28.20Lg 0x%04X%016I64X"), i,
			ptr[i], (unsigned int)*(tjs_uint16*)(((tjs_uint8*)(ptr + i)) + 8),
			*(tjs_uint64*)(ptr + i));
		TVPAddLog(buf);
	}

	// -- Cr0NpxState
	TJS_snprintf(buf, BUF_SIZE,TJS_W("FP CR0 NPX State  : 0x%08X"), ctx.FloatSave.Cr0NpxState);
	TVPAddLog(buf);
#endif

	// -- SSE/SSE2 registers
#ifdef TJS_64BIT_OS
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  0 : 0x%016lx 0x%016lx"),ctx.Xmm0.High, ctx.Xmm0.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  1 : 0x%016lx 0x%016lx"),ctx.Xmm1.High, ctx.Xmm1.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  2 : 0x%016lx 0x%016lx"),ctx.Xmm2.High, ctx.Xmm2.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  3 : 0x%016lx 0x%016lx"),ctx.Xmm3.High, ctx.Xmm3.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  4 : 0x%016lx 0x%016lx"),ctx.Xmm4.High, ctx.Xmm4.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  5 : 0x%016lx 0x%016lx"),ctx.Xmm5.High, ctx.Xmm5.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  6 : 0x%016lx 0x%016lx"),ctx.Xmm6.High, ctx.Xmm6.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  7 : 0x%016lx 0x%016lx"),ctx.Xmm7.High, ctx.Xmm7.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  8 : 0x%016lx 0x%016lx"),ctx.Xmm8.High, ctx.Xmm8.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM  9 : 0x%016lx 0x%016lx"),ctx.Xmm9.High, ctx.Xmm9.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 10 : 0x%016lx 0x%016lx"),ctx.Xmm10.High, ctx.Xmm10.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 11 : 0x%016lx 0x%016lx"),ctx.Xmm11.High, ctx.Xmm11.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 12 : 0x%016lx 0x%016lx"),ctx.Xmm12.High, ctx.Xmm12.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 13 : 0x%016lx 0x%016lx"),ctx.Xmm13.High, ctx.Xmm13.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 14 : 0x%016lx 0x%016lx"),ctx.Xmm14.High, ctx.Xmm14.Low);
	TVPAddLog(buf);
	TJS_snprintf(buf, BUF_SIZE, TJS_W("XMM 15 : 0x%016lx 0x%016lx"),ctx.Xmm15.High, ctx.Xmm15.Low);
	TVPAddLog(buf);

	TJS_snprintf(buf,BUF_SIZE,  TJS_W("MXCSR : 0x%08x"), ctx.MxCsr );
	TVPAddLog(buf);
#else
	if(ctx.ContextFlags & CONTEXT_EXTENDED_REGISTERS)
	{
		// ExtendedRegisters is a area which meets fxsave and fxrstor instruction?
		#pragma pack(push,1)
		union xmm_t
		{
			struct
			{
				float sA;
				float sB;
				float sC;
				float sD;
			};
			struct
			{
				double dA;
				double dB;
			};
			struct
			{
				tjs_uint64 i64A;
				tjs_uint64 i64B;
			};
		};
		#pragma pack(pop)
		for(tjs_int i = 0; i < 8; i++)
		{
			xmm_t * xmm = (xmm_t *)(ctx.ExtendedRegisters + i * 16+ 0xa0);
			TJS_snprintf(buf, BUF_SIZE,
				TJS_W("XMM %d : [ %15.8g %15.8g %15.8g %15.8g ] [ %24.16lg %24.16lg ] [ 0x%016I64X-0x%016I64X ]"),
				i,
				xmm->sD, xmm->sC, xmm->sB, xmm->sA,
				xmm->dB, xmm->dA,
				xmm->i64B, xmm->i64A);
			TVPAddLog(buf);
		}
		TJS_snprintf(buf,BUF_SIZE,  TJS_W("MXCSR : 0x%08X"),
			*(DWORD*)(ctx.ExtendedRegisters + 0x18));
		TVPAddLog(buf);
	}
#endif
}
//---------------------------------------------------------------------------
void TVPDumpHWException()
{
	// dump latest hardware exception if it exists

	if(!TVPHWExcRaised) return;
	TVPHWExcRaised = false;

	TVPOnError();
	
	static const int BUF_SIZE = 256;
	tjs_char buf[BUF_SIZE];
	tTVPHWExceptionData * d = &TVPLastHWExceptionData;

	TVPAddLog(ttstr(TVPHardwareExceptionRaised));

	ttstr line;

	line = TJS_W("Exception : ");

	tjs_char *p = NULL;
	switch(d->Code)
	{
	case  3:	p = TJS_W("Divide By Zero"); break;
	case  4:	p = TJS_W("Range Error"); break;
	case  5:	p = TJS_W("Integer Overflow"); break;
	case  6:	p = TJS_W("Invalid Operation"); break;
	case  7:	p = TJS_W("Zero Divide"); break;
	case  8:	p = TJS_W("Overflow"); break;
	case  9:	p = TJS_W("Underflow"); break;
	case 10:	p = TJS_W("Invalid Cast"); break;
	case 11:	p = TJS_W("Access Violation"); break;
	case 12:	p = TJS_W("Privilege Violation"); break;
	case 13:	p = TJS_W("Control C"); break;
	case 14:	p = TJS_W("Stack Overflow"); break;
	}

	if(p) line += p;

	if(d->Code == 11)
	{
		// EAccessViolation
		const tjs_char *mode = TJS_W("unknown");
		if(d->AccessFlag == 0)
			mode = TJS_W("read");
		else if(d->AccessFlag == 1)
			mode = TJS_W("write");
		else if(d->AccessFlag == 8)
			mode = TJS_W("execute");
		TJS_snprintf(buf, BUF_SIZE, TJS_W("(%ls access to 0x%p)"), mode, d->AccessTarget);
		line += buf;
	}

	TJS_snprintf(buf, BUF_SIZE, TJS_W("  at  EIP = 0x%p   ESP = 0x%p"), d->EIP, d->ESP);
	line += buf;
	if(d->Module[0])
	{
		line += TJS_W("   in ") + ttstr(d->Module);
	}

	TVPAddLog(line);

	// dump OS context
	TVPDumpOSContext(d->Context);

	// dump codes at EIP
	line = TJS_W("Codes at EIP : ");
	for(tjs_int i = 0; i<d->CodesAtEIPLen; i++)
	{
		TJS_snprintf(buf, BUF_SIZE, TJS_W("0x%02X "), d->CodesAtEIP[i]);
		line += buf;
	}
	TVPAddLog(line);

	TVPAddLog(TJS_W("Stack data and data pointed by each stack data :"));

	// dump stack and data
	for(tjs_int s = 0; s<d->StackAtESPLen; s++)
	{
		TJS_snprintf(buf, BUF_SIZE, TJS_W("0x%p (ESP+%3d) : 0x%p : "),
			(DWORD)d->ESP + s*sizeof(tjs_uint32),
			s*sizeof(tjs_uint32), d->StackAtESP[s]);
		line = buf;

		for(tjs_int i = 0; i<d->StackDumpsLen[s]; i++)
		{
			TJS_snprintf(buf, BUF_SIZE, TJS_W("0x%02X "), d->StackDumps[s][i]);
			line += buf;
		}
		TVPAddLog(line);
	}

	// dump call trace
	TVPAddLog(TJS_W("Call Trace :"));
	for(tjs_int s = 0; s<d->CallTraceLen; s++)
	{
		TJS_snprintf(buf, BUF_SIZE, TJS_W("0x%p : "),
			d->CallTrace[s]);
		line = buf;

		for(tjs_int i = 0; i<d->CallTraceDumpsLen[s]; i++)
		{
			TJS_snprintf(buf, BUF_SIZE, TJS_W("0x%02X "), d->CallTraceDumps[s][i]);
			line += buf;
		}
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery((void*)d->CallTrace[s], &mbi, sizeof(mbi));
		if(mbi.State == MEM_COMMIT)
		{
			wchar_t module[MAX_PATH];
			if(::GetModuleFileName((HMODULE)mbi.AllocationBase, module, MAX_PATH))
			{
				line += ttstr(ExtractFileName(module).c_str());
				TJS_snprintf(buf, BUF_SIZE, TJS_W(" base 0x%p"), mbi.AllocationBase);
				line += buf;
			}
		}
		TVPAddLog(line);
	}
}
//---------------------------------------------------------------------------
#else
void TVPDumpHWException(void)
{
	// dummy
}
#endif
//---------------------------------------------------------------------------




#if 0
//---------------------------------------------------------------------------
// random generator initializer
//---------------------------------------------------------------------------
static BOOL CALLBACK TVPInitRandomEnumWinProc(HWND hwnd, LPARAM lparam)
{
	RECT r;
	GetWindowRect(hwnd, &r);
	TVPPushEnvironNoise(&hwnd, sizeof(hwnd));
	TVPPushEnvironNoise(&r, sizeof(r));
	DWORD procid, threadid;
	threadid = GetWindowThreadProcessId(hwnd, &procid);
	TVPPushEnvironNoise(&procid, sizeof(procid));
	TVPPushEnvironNoise(&threadid, sizeof(threadid));
	return TRUE;
}
#endif
//---------------------------------------------------------------------------
static void TVPInitRandomGenerator()
{
	// initialize random generator
#if 0
	DWORD tick = GetTickCount();
	TVPPushEnvironNoise(&tick, sizeof(DWORD));
	GUID guid;
	CoCreateGuid(&guid);
	TVPPushEnvironNoise(&guid, sizeof(guid));
	DWORD id = GetCurrentProcessId();
	TVPPushEnvironNoise(&id, sizeof(id));
	id = GetCurrentThreadId();
	TVPPushEnvironNoise(&id, sizeof(id));
	SYSTEMTIME systime;
	GetSystemTime(&systime);
	TVPPushEnvironNoise(&systime, sizeof(systime));
	POINT pt;
	GetCursorPos(&pt);
	TVPPushEnvironNoise(&pt, sizeof(pt));

	EnumWindows((WNDENUMPROC)TVPInitRandomEnumWinProc, 0);
#endif
	tjs_uint32 tick = TVPGetRoughTickCount32();
	TVPPushEnvironNoise(&tick, sizeof(tick));
	std::thread::id tid = std::this_thread::get_id();
	TVPPushEnvironNoise(&tid, sizeof(tid));
	time_t curtime = time(NULL);
	TVPPushEnvironNoise(&curtime, sizeof(curtime));
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPInitializeBaseSystems
//---------------------------------------------------------------------------
void TVPInitializeBaseSystems()
{
	// set system archive delimiter
	tTJSVariant v;
	if(TVPGetCommandLine(TJS_W("-arcdelim"), &v))
		TVPArchiveDelimiter = ttstr(v)[0];

	// set default current directory
	{
		TVPSetCurrentDirectory( IncludeTrailingBackslash(ExtractFileDir(ExePath())) );
	}

	// load message map file
	bool load_msgmap = GetSystemSecurityOption("disablemsgmap") == 0;

	if(load_msgmap)
	{
		const tjs_char name_msgmap [] = TJS_W("msgmap.tjs");
		if(TVPIsExistentStorage(name_msgmap))
			TVPExecuteStorage(name_msgmap, NULL, false, TJS_W(""));
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// system initializer / uninitializer
//---------------------------------------------------------------------------
#if 0
// フォルダ選択ダイアログのコールバック関数
static int CALLBACK TVPBrowseCallbackProc(HWND hwnd,UINT uMsg,LPARAM lParam,LPARAM lpData)
{
    if(uMsg==BFFM_INITIALIZED){
		wchar_t exeDir[MAX_PATH];
		TJS_strcpy(exeDir, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
        ::SendMessage(hwnd,BFFM_SETSELECTION,(WPARAM)TRUE,(LPARAM)exeDir);
    }
    return 0;
}
#endif
static tjs_uint64 TVPTotalPhysMemory = 0;
static void TVPInitProgramArgumentsAndDataPath(bool stop_after_datapath_got);
void TVPBeforeSystemInit()
{
	//RegisterDllLoadHook();
		// register DLL delayed import hook to support _inmm.dll

	TVPInitProgramArgumentsAndDataPath(false); // ensure command line

	// set system archive delimiter after patch.tjs specified
	tTJSVariant v;
	if (TVPGetCommandLine(TJS_W("-arcdelim"), &v))
		TVPArchiveDelimiter = ttstr(v)[0];

	if (TVPIsExistentStorageNoSearchNoNormalize(TVPProjectDir)) {
		TVPProjectDir += TVPArchiveDelimiter;
	} else {
		TVPProjectDir += TJS_W("/");
	}
	TVPSetCurrentDirectory(TVPProjectDir);

#ifdef TVP_REPORT_HW_EXCEPTION
	// __dee_hacked_set_getExceptionObjectHook(TVP__dee_hacked_getExceptionObjectHook);
		// register hook function for hardware exceptions
#endif
#if 0
	Application->SetHintHidePause( 24*60*60*1000 );
		// not to hide tool tip hint immediately
	Application->SetShowHint( false );
	Application->SetShowHint( true );
		// to ensure assigning new HintWindow Class defined in HintWindow.cpp 
#endif

	// randomize
	TVPInitRandomGenerator();

	// memory usage
	{
#if 0
		MEMORYSTATUSEX status = { sizeof(MEMORYSTATUSEX) };
		::GlobalMemoryStatusEx(&status);

		TVPPushEnvironNoise(&status, sizeof(status));

		TVPTotalPhysMemory = status.ullTotalPhys;

		ttstr memstr( std::to_wstring(TVPTotalPhysMemory).c_str() );
		TVPAddImportantLog( TVPFormatMessage(TVPInfoTotalPhysicalMemory, memstr) );

		tTJSVariant opt;
		if(TVPGetCommandLine(TJS_W("-memusage"), &opt))
		{
			ttstr str(opt);
			if(str == TJS_W("low"))
				TVPTotalPhysMemory = 0; // assumes zero
		}

		if(TVPTotalPhysMemory <= 36*1024*1024)
		{
			// very very low memory, forcing to assume zero memory
			TVPTotalPhysMemory = 0;
		}

		if(TVPTotalPhysMemory < 48*1024*1024ULL)
		{
			// extra low memory
			if(TJSObjectHashBitsLimit > 0)
				TJSObjectHashBitsLimit = 0;
			TVPSegmentCacheLimit = 0;
			TVPFreeUnusedLayerCache = true; // in LayerIntf.cpp
		}
		else if(TVPTotalPhysMemory < 64*1024*1024)
		{
			// low memory
			if(TJSObjectHashBitsLimit > 4)
				TJSObjectHashBitsLimit = 4;
		}
#endif
		TVPMemoryInfo meminf;
		TVPGetMemoryInfo(meminf);
		TVPPushEnvironNoise(&meminf, sizeof(meminf));
		
		TVPTotalPhysMemory = meminf.MemTotal * 1024;
		if (TVPTotalPhysMemory > 768 * 1024 * 1024) {
			TVPTotalPhysMemory -= 512 * 1024 * 1024; // assume that system reserved 512M memory
		} else {
			TVPTotalPhysMemory /= 2; // use half memory in small memory devices
		}

		TVPAddImportantLog(TVPFormatMessage(TVPInfoTotalPhysicalMemory, tjs_int64(TVPTotalPhysMemory)));
		if (TVPTotalPhysMemory > 256 * 1024 * 1024) {
			std::string str = IndividualConfigManager::GetInstance()->GetValue<std::string>("memusage", "unlimited");
			if (str == ("low"))
				TVPTotalPhysMemory = 0; // assumes zero
			else if (str == ("medium"))
				TVPTotalPhysMemory = 128 * 1024 * 1024;
			else if (str == ("high"))
				TVPTotalPhysMemory = 256 * 1024 * 1024;
		} else { // use minimum memory usage if less than 256M(512M physics)
			TVPTotalPhysMemory = 0;
		}

		if (TVPTotalPhysMemory < 128 * 1024 * 1024)
		{
			// very very low memory, forcing to assume zero memory
			TVPTotalPhysMemory = 0;
		}

		if (TVPTotalPhysMemory < 128 * 1024 * 1024)
		{
			// extra low memory
			if (TJSObjectHashBitsLimit > 0)
				TJSObjectHashBitsLimit = 0;
			TVPSegmentCacheLimit = 0;
			TVPFreeUnusedLayerCache = true; // in LayerIntf.cpp
		} else if (TVPTotalPhysMemory < 256 * 1024 * 1024)
		{
			// low memory
			if (TJSObjectHashBitsLimit > 4)
				TJSObjectHashBitsLimit = 4;
		}
	}
#if 0

	wchar_t buf[MAX_PATH];
	bool bufset = false;
	bool nosel = false;
	bool forcesel = false;

	bool forcedataxp3 = GetSystemSecurityOption("forcedataxp3") != 0;
	bool acceptfilenameargument = GetSystemSecurityOption("acceptfilenameargument") != 0;

	if(!forcedataxp3 && !acceptfilenameargument)
	{
		if(TVPGetCommandLine(TJS_W("-nosel")) || TVPGetCommandLine(TJS_W("-about")))
		{
			nosel = true;
		}
		else
		{
			wchar_t exeDir[MAX_PATH];
			TJS_strcpy(exeDir, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
			for(tjs_int i = 1; i<_argc; i++)
			{
				if(_argv[i][0] == '-' && _argv[i][1] == '-' && _argv[i][2] == 0)
					break;

				if(_argv[i][0] != '-')
				{
					// TODO: set the current directory
					::SetCurrentDirectory( exeDir );
					TJS_strncpy(buf, ttstr(_argv[i]).c_str(), MAX_PATH-1);
					buf[MAX_PATH-1] = TJS_W('\0');
					if(DirectoryExists(buf)) // is directory?
						TJS_strcat(buf, TJS_W("\\"));

					TVPProjectDirSelected = true;
					bufset = true;
					nosel = true;
				}
			}
		}
	}

	// check "-sel" option, to force show folder selection window
	if(!forcedataxp3 && TVPGetCommandLine(TJS_W("-sel")))
	{
		// sel option was set
		if(bufset)
		{
			wchar_t path[MAX_PATH];
			wchar_t *dum = 0;
			GetFullPathName(buf, MAX_PATH-1, path, &dum);
			TJS_strcpy(buf, path);
			TVPProjectDirSelected = false;
			bufset = true;
		}
		nosel = true;
		forcesel = true;
	}

	// check "content-data" directory
	if(!forcedataxp3 && !nosel)
	{
		wchar_t tmp[MAX_PATH];
		TJS_strcpy(tmp, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
		TJS_strcat(tmp, TJS_W("content-data"));
		if(DirectoryExists(tmp))
		{
			TJS_strcat(tmp, TJS_W("\\"));
			TJS_strcpy(buf, tmp);
			TVPProjectDirSelected = true;
			bufset = true;
			nosel = true;
		}
	}

	// check "data.xp3" archive
 	if(!nosel)
	{
		wchar_t tmp[MAX_PATH];
		TJS_strcpy(tmp, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
		TJS_strcat(tmp, TJS_W("data.xp3"));
		if(FileExists(tmp))
		{
			TJS_strcpy(buf, tmp);
			TVPProjectDirSelected = true;
			bufset = true;
			nosel = true;
		}
	}

	// check "data.exe" archive
 	if(!nosel)
	{
		wchar_t tmp[MAX_PATH];
		TJS_strcpy(tmp, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
		TJS_strcat(tmp, TJS_W("data.exe"));
		if(FileExists(tmp))
		{
			TJS_strcpy(buf, tmp);
			TVPProjectDirSelected = true;
			bufset = true;
			nosel = true;
		}
	}

	// check self combined xpk archive
	if(!nosel)
	{
		if(TVPIsXP3Archive(TVPNormalizeStorageName(ExePath())))
		{
			TJS_strcpy(buf, ExePath().c_str());
			TVPProjectDirSelected = true;
			bufset = true;
			nosel = true;
		}
	}


	// check "data" directory
	if(!forcedataxp3 && !nosel)
	{
		wchar_t tmp[MAX_PATH];
		TJS_strcpy(tmp, IncludeTrailingBackslash(ExtractFileDir(ExePath())).c_str());
		TJS_strcat(tmp, TJS_W("data"));
		if(DirectoryExists(tmp))
		{
			TJS_strcat(tmp, TJS_W("\\"));
			TJS_strcpy(buf, tmp);
			TVPProjectDirSelected = true;
			bufset = true;
			nosel = true;
		}
	}

	// decide a directory to execute or to show folder selection
	if(!bufset)
	{
		if(forcedataxp3) throw EAbort(TJS_W("Aborted"));
		TJS_strcpy(buf, ExtractFileDir(ExePath()).c_str());
		int curdirlen = (int)TJS_strlen(buf);
		if(buf[curdirlen-1] != TJS_W('\\')) buf[curdirlen] = TJS_W('\\'), buf[curdirlen+1] = 0;
	}

#ifndef TVP_DISABLE_SELECT_XP3_OR_FOLDER
	if(!forcedataxp3 && (!nosel || forcesel))
	{
		BOOL			bRes;
		wchar_t			chPutFolder[MAX_PATH];
		LPITEMIDLIST	pidlRetFolder;
		BROWSEINFO		stBInfo;
		::ZeroMemory( &stBInfo, sizeof(stBInfo) );

		stBInfo.pidlRoot = NULL;
		stBInfo.hwndOwner = NULL;
		stBInfo.pszDisplayName = chPutFolder;
		stBInfo.lpszTitle = TVPSelectXP3FileOrFolder;
		stBInfo.ulFlags = BIF_BROWSEINCLUDEFILES|BIF_RETURNFSANCESTORS|BIF_DONTGOBELOWDOMAIN|BIF_RETURNONLYFSDIRS;
		stBInfo.lpfn = TVPBrowseCallbackProc;
		stBInfo.lParam = NULL;

		pidlRetFolder = ::SHBrowseForFolder( &stBInfo );
		if( pidlRetFolder != NULL ) {
			bRes = ::SHGetPathFromIDList( pidlRetFolder, chPutFolder );
			if( bRes != FALSE ) {
				wcsncpy( buf, chPutFolder, MAX_PATH );
				tjs_int buflen = (tjs_int)TJS_strlen(buf);
				if( buflen >= 1 ) {
					if( buf[buflen-1] != TJS_W('\\') && buflen < (MAX_PATH-2) ) {
						buf[buflen] = TJS_W('\\');
						buf[buflen+1] = TJS_W('\0');
					}
				}
				TVPProjectDirSelected = true;
			}
			::CoTaskMemFree( pidlRetFolder );
		}
	}
#endif

	// check project dir and store some environmental variables
	if(TVPProjectDirSelected)
	{
		Application->SetShowMainForm( false );
	}

	tjs_int buflen = (tjs_int)TJS_strlen(buf);
	if(buflen >= 1)
	{
		if(buf[buflen-1] != TJS_W('\\')) buf[buflen] = TVPArchiveDelimiter, buf[buflen+1] = 0;
	}

	TVPProjectDir = TVPNormalizeStorageName(buf);
	TVPSetCurrentDirectory(TVPProjectDir);
	TVPNativeProjectDir = buf;

	if(TVPProjectDirSelected)
	{
		TVPAddImportantLog( TVPFormatMessage(TVPInfoSelectedProjectDirectory, TVPProjectDir) );
	}
#endif
}
//---------------------------------------------------------------------------
static void TVPDumpOptions();
//---------------------------------------------------------------------------
extern bool TVPEnableGlobalHeapCompaction;
extern void TVPGL_SSE2_Init();
extern "C" void TVPGL_ASM_Init();
extern bool TVPAutoSaveBookMark;
static bool TVPHighTimerPeriod = false;
static uint32_t TVPTimeBeginPeriodRes = 0;
//---------------------------------------------------------------------------
void TVPAfterSystemInit()
{
	// check CPU type
	TVPDetectCPU();

	TVPAllocGraphicCacheOnHeap = false; // always false since beta 20

	// determine maximum graphic cache limit
	tTJSVariant opt;
	tjs_int64 limitmb = -1;
	if(TVPGetCommandLine(TJS_W("-gclim"), &opt))
	{
		ttstr str(opt);
		if(str == TJS_W("auto"))
			limitmb = -1;
		else
			limitmb = opt.AsInteger();
	}


	if(limitmb == -1)
	{
		if(TVPTotalPhysMemory <= 32*1024*1024)
			TVPGraphicCacheSystemLimit = 0;
		else if(TVPTotalPhysMemory <= 48*1024*1024)
			TVPGraphicCacheSystemLimit = 0;
		else if(TVPTotalPhysMemory <= 64*1024*1024)
			TVPGraphicCacheSystemLimit = 0;
		else if(TVPTotalPhysMemory <= 96*1024*1024)
			TVPGraphicCacheSystemLimit = 4;
		else if(TVPTotalPhysMemory <= 128*1024*1024)
			TVPGraphicCacheSystemLimit = 8;
		else if(TVPTotalPhysMemory <= 192*1024*1024)
			TVPGraphicCacheSystemLimit = 12;
		else if(TVPTotalPhysMemory <= 256*1024*1024)
			TVPGraphicCacheSystemLimit = 20;
		else if(TVPTotalPhysMemory <= 512*1024*1024)
			TVPGraphicCacheSystemLimit = 40;
		else
			TVPGraphicCacheSystemLimit = tjs_uint64(TVPTotalPhysMemory / (1024*1024*10));	// cachemem = physmem / 10
		TVPGraphicCacheSystemLimit *= 1024*1024;
	}
	else
	{
		TVPGraphicCacheSystemLimit = limitmb * 1024*1024;
	}
	// 32bit なので 512MB までに制限
	if( TVPGraphicCacheSystemLimit >= 512*1024*1024 )
		TVPGraphicCacheSystemLimit = 512*1024*1024;


	if(TVPTotalPhysMemory <= 64*1024*1024)
		TVPSetFontCacheForLowMem();

//	TVPGraphicCacheSystemLimit = 1*1024*1024; // DEBUG 

	if(TVPGetCommandLine(TJS_W("-autosave"), &opt))
	{
		ttstr str(opt);
		if(str == TJS_W("yes")) {
			TVPAutoSaveBookMark = true;
		}
	}
	// check TVPGraphicSplitOperation option
	std::string _val = IndividualConfigManager::GetInstance()->GetValue<std::string>("renderer", "software");
	if (_val != "software") {
		TVPGraphicSplitOperationType = gsotNone;
	} else {
		TVPDrawThreadNum = IndividualConfigManager::GetInstance()->GetValue<int>("software_draw_thread", 0);
		if (TVPGetCommandLine(TJS_W("-gsplit"), &opt))
		{
			ttstr str(opt);
			if (str == TJS_W("no"))
				TVPGraphicSplitOperationType = gsotNone;
			else if (str == TJS_W("int"))
				TVPGraphicSplitOperationType = gsotInterlace;
			else if (str == TJS_W("yes") || str == TJS_W("simple"))
				TVPGraphicSplitOperationType = gsotSimple;
			else if (str == TJS_W("bidi"))
				TVPGraphicSplitOperationType = gsotBiDirection;

		}
	}

	// check TVPDefaultHoldAlpha option
	if(TVPGetCommandLine(TJS_W("-holdalpha"), &opt))
	{
		ttstr str(opt);
		if(str == TJS_W("yes") || str == TJS_W("true"))
			TVPDefaultHoldAlpha = true;
		else
			TVPDefaultHoldAlpha = false;
	}

	// check TVPJPEGFastLoad option
	if(TVPGetCommandLine(TJS_W("-jpegdec"), &opt)) // this specifies precision for JPEG decoding
	{
		ttstr str(opt);
		if(str == TJS_W("normal"))
			TVPJPEGLoadPrecision = jlpMedium;
		else if(str == TJS_W("low"))
			TVPJPEGLoadPrecision = jlpLow;
		else if(str == TJS_W("high"))
			TVPJPEGLoadPrecision = jlpHigh;

	}

	// dump option
	TVPDumpOptions();

	// initilaize x86 graphic routines
#if 0
#ifndef TJS_64BIT_OS
	TVPGL_IA32_Init();
#endif
	TVPGL_SSE2_Init();
#endif
	TVPGL_ASM_Init();

	// timer precision
	uint32_t prectick = 1;
	if(TVPGetCommandLine(TJS_W("-timerprec"), &opt))
	{
		ttstr str(opt);
		if(str == TJS_W("high")) prectick = 1;
		if(str == TJS_W("higher")) prectick = 5;
		if(str == TJS_W("normal")) prectick = 10;
	}

        // draw thread num
        tjs_int drawThreadNum = 0;
        if (TVPGetCommandLine(TJS_W("-drawthread"), &opt)) {
          ttstr str(opt);
          if (str == TJS_W("auto"))
            drawThreadNum = 0;
          else
            drawThreadNum = (tjs_int)opt;
        }
        TVPDrawThreadNum = drawThreadNum;
#if 0
	if(prectick)
	{
		// retrieve minimum timer resolution
		TIMECAPS tc;
		timeGetDevCaps(&tc, sizeof(tc));
		if(prectick < tc.wPeriodMin)
			TVPTimeBeginPeriodRes = tc.wPeriodMin;
		else
			TVPTimeBeginPeriodRes = prectick;
		if(TVPTimeBeginPeriodRes > tc.wPeriodMax)
			TVPTimeBeginPeriodRes = tc.wPeriodMax;
		// set timer resolution
		timeBeginPeriod(TVPTimeBeginPeriodRes);
		TVPHighTimerPeriod = true;
	}

	TVPPushEnvironNoise(&TVPCPUType, sizeof(TVPCPUType));

	// set LFH
	if(TVPGetCommandLine(TJS_W("-uselfh"), &opt)) {
		ttstr str(opt);
		if(str == TJS_W("yes") || str == TJS_W("true")) {
			ULONG HeapInformation = 2;
			BOOL lfhenable = ::HeapSetInformation( GetProcessHeap(), HeapCompatibilityInformation, &HeapInformation, sizeof(HeapInformation) );
			if( lfhenable ) {
				TVPAddLog( TJS_W("(info) Enable LFH") );
			} else {
				TVPAddLog( TJS_W("(info) Cannot Enable LFH") );
			}
		}
	}
	// Global Heap Compact
	if(TVPGetCommandLine(TJS_W("-ghcompact"), &opt)) {
		ttstr str(opt);
		if(str == TJS_W("yes") || str == TJS_W("true")) {
			TVPEnableGlobalHeapCompaction = true;
		}
	}
#endif
}
//---------------------------------------------------------------------------
void TVPBeforeSystemUninit()
{
	// TVPDumpHWException(); // dump cached hw exceptoin
}
//---------------------------------------------------------------------------
void TVPAfterSystemUninit()
{
#if 0
	// restore timer precision
	if(TVPHighTimerPeriod)
	{
		timeEndPeriod(TVPTimeBeginPeriodRes);
	}
#endif
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
bool TVPTerminated = false;
bool TVPTerminateOnWindowClose = true;
bool TVPTerminateOnNoWindowStartup = true;
int TVPTerminateCode = 0;
//---------------------------------------------------------------------------
void TVPTerminateAsync(int code)
{
	// do "A"synchronous temination of application
	TVPTerminated = true;
	TVPTerminateCode = code;

	// posting dummy message will prevent "missing WM_QUIT bug" in Direct3D framework.
	if(TVPSystemControl) TVPSystemControl->CallDeliverAllEventsOnIdle();

	Application->Terminate();

	if(TVPSystemControl) TVPSystemControl->CallDeliverAllEventsOnIdle();
}
//---------------------------------------------------------------------------
void TVPTerminateSync(int code)
{
	// do synchronous temination of application (never return)
	TVPSystemUninit();
	TVPExitApplication(code);
}
//---------------------------------------------------------------------------
void TVPMainWindowClosed()
{
	// called from WindowIntf.cpp, caused by closing all window.
	if( TVPTerminateOnWindowClose) TVPTerminateAsync();
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// GetCommandLine
//---------------------------------------------------------------------------
static std::vector<std::string> * TVPGetEmbeddedOptions()
{
#if 0
	HMODULE hModule = ::GetModuleHandle(NULL);
	const char *buf = NULL;
	unsigned int size = 0;
	HRSRC hRsrc = ::FindResource(NULL, MAKEINTRESOURCE(IDR_OPTION), TEXT("TEXT"));
	if( hRsrc != NULL ) {
		size = ::SizeofResource( hModule, hRsrc );
		HGLOBAL hGlobal = ::LoadResource( hModule, hRsrc );
		if( hGlobal != NULL ) {
			buf = reinterpret_cast<const char*>(::LockResource(hGlobal));
		}
	}
	if( buf == NULL ) return NULL;
#endif
	std::vector<std::string> *ret = NULL;
#if 0
	try {
		ret = new std::vector<std::string>();
		const char *tail = buf + size;
		const char *start = buf;
		while( buf < tail ) {
			if( buf[0] == 0x0D && buf[1] == 0x0A ) {	// CR LF
				ret->push_back( std::string(start,buf) );
				start = buf + 2;
				buf++;
			} else if( buf[0] == 0x0D || buf[0] == 0x0A ) {	// CR or LF
				ret->push_back( std::string(start,buf) );
				start = buf + 1;
			} else if( buf[0] == '\0' ) {
				ret->push_back( std::string(start,buf) );
				start = buf + 1;
				break;
			}
			buf++;
		}
		if( start < buf ) {
			ret->push_back( std::string(start,buf) );
		}
	} catch(...) {
		if(ret) delete ret;
		throw;
	}
	TVPAddImportantLog( (const tjs_char*)TVPInfoLoadingExecutableEmbeddedOptionsSucceeded );
#endif
	return ret;
}
//---------------------------------------------------------------------------
static std::vector<std::string> * TVPGetConfigFileOptions(const ttstr& filename)
{
#if 0
	// load .cf file
	std::wstring errmsg;
	if(!FileExists(filename))
		errmsg = (const tjs_char*)TVPFileNotFound;
#endif
	std::vector<std::string> * ret = NULL; // new std::vector<std::string>();
#if 0
	if (errmsg == TJS_W(""))
	{
		try
		{
			ret = LoadLinesFromFile(filename);
		}
		catch(Exception & e)
		{
			errmsg = e.what();
		}
		catch(...)
		{
			delete ret;
			throw;
		}
	}

	if(errmsg != TJS_W(""))
		TVPAddImportantLog( TVPFormatMessage(TVPInfoLoadingConfigurationFileFailed, filename.c_str(), errmsg.c_str()) );
	else
		TVPAddImportantLog( TVPFormatMessage(TVPInfoLoadingConfigurationFileSucceeded, filename.c_str()) );
#endif
	return ret;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------

static ttstr TVPParseCommandLineOne(const ttstr &i)
{
	// value is specified
	const tjs_char *p, *o;
	p = o = i.c_str();
	p = TJS_strchr(p, '=');

	if(p == NULL) { return i + TJS_W("=yes"); }

	p++;

	ttstr optname(o, (int)(p - o));

	if(*p == TJS_W('\'') || *p == TJS_W('\"'))
	{
		// as an escaped string
		tTJSVariant v;
		TJSParseString(v, &p);

		return optname + ttstr(v);
	}
	else
	{
		// as a string
		return optname + p;
	}
}
//---------------------------------------------------------------------------
std::vector <ttstr> TVPProgramArguments;
static bool TVPProgramArgumentsInit = false;
static tjs_int TVPCommandLineArgumentGeneration = 0;
static bool TVPDataPathDirectoryEnsured = false;
//---------------------------------------------------------------------------
tjs_int TVPGetCommandLineArgumentGeneration() { return TVPCommandLineArgumentGeneration; }
//---------------------------------------------------------------------------
void TVPEnsureDataPathDirectory()
{
	if(!TVPDataPathDirectoryEnsured)
	{
		TVPDataPathDirectoryEnsured = true;
		// ensure data path existence
		if(!TVPCheckExistentLocalFolder(TVPNativeDataPath.c_str()))
		{
			if(TVPCreateFolders(TVPNativeDataPath.c_str()))
				TVPAddImportantLog( TVPFormatMessage( TVPInfoDataPathDoesNotExistTryingToMakeIt, (const tjs_char*)TVPOk ) );
			else
				TVPAddImportantLog( TVPFormatMessage( TVPInfoDataPathDoesNotExistTryingToMakeIt, (const tjs_char*)TVPFaild ) );
		}
	}
}
//---------------------------------------------------------------------------
static void PushAllCommandlineArguments()
{
#if 0
	// store arguments given by commandline to "TVPProgramArguments"
	bool acceptfilenameargument = GetSystemSecurityOption("acceptfilenameargument") != 0;

	bool argument_stopped = false;
	if(acceptfilenameargument) argument_stopped = true;
	int file_argument_count = 0;
	for(tjs_int i = 1; i<_argc; i++)
	{
		if(argument_stopped)
		{
			ttstr arg_name_and_value = TJS_W("-arg") + ttstr(file_argument_count) + TJS_W("=")
				+ ttstr(_argv[i]);
			file_argument_count++;
			TVPProgramArguments.push_back(arg_name_and_value);
		}
		else
		{
			if(_argv[i][0] == '-')
			{
				if(_argv[i][1] == '-' && _argv[i][2] == 0)
				{
					// argument stopper
					argument_stopped = true;
				}
				else
				{
					ttstr value(_argv[i]);
					if(!TJS_strchr(value.c_str(), TJS_W('=')))
						value += TJS_W("=yes");
					TVPProgramArguments.push_back(TVPParseCommandLineOne(value));
				}
			}
		}
	}
#endif
}
//---------------------------------------------------------------------------
static void PushConfigFileOptions(const std::vector<std::string> * options)
{
	if(!options) return;
	for(unsigned int j = 0; j < options->size(); j++)
	{
		if( (*options)[j].c_str()[0] != ';') // unless comment
			TVPProgramArguments.push_back(
			TVPParseCommandLineOne(TJS_W("-") + ttstr((*options)[j].c_str())));
	}
}
//---------------------------------------------------------------------------
static void TVPInitProgramArgumentsAndDataPath(bool stop_after_datapath_got)
{
	if(!TVPProgramArgumentsInit)
	{
		TVPProgramArgumentsInit = true;

		// find options from self executable image
		const int num_option_layers = 3;
		std::vector<std::string> * options[num_option_layers];
		for(int i = 0; i < num_option_layers; i++) options[i] = NULL;
		try
		{
			// read embedded options and default configuration file
			options[0] = TVPGetEmbeddedOptions();
//			options[1] = TVPGetConfigFileOptions(ApplicationSpecialPath::GetConfigFileName(ExePath()));

			// at this point, we need to push all exsting known options
			// to be able to see datapath
			PushAllCommandlineArguments();
			PushConfigFileOptions(options[1]); // has more priority
			PushConfigFileOptions(options[0]); // has lesser priority

			// read datapath
			tTJSVariant val;
			ttstr config_datapath;
// 			if(TVPGetCommandLine(TJS_W("-datapath"), &val))
// 				config_datapath = ((ttstr)val).AsStdString();
			TVPNativeDataPath = ApplicationSpecialPath::GetDataPathDirectory(config_datapath, ExePath());

			if(stop_after_datapath_got) return;

			// read per-user configuration file
//			options[2] = TVPGetConfigFileOptions(ApplicationSpecialPath::GetUserConfigFileName(config_datapath, ExePath()));

			// push each options into option stock
			// we need to clear TVPProgramArguments first because of the
			// option priority order.
			TVPProgramArguments.clear();
			PushAllCommandlineArguments();
			PushConfigFileOptions(options[2]); // has more priority
			PushConfigFileOptions(options[1]); // has more priority
			PushConfigFileOptions(options[0]); // has lesser priority
		} catch(...) {
			for(int i = 0; i < num_option_layers; i++)
				if(options[i]) delete options[i];
			throw;
		}
		for(int i = 0; i < num_option_layers; i++)
			if(options[i]) delete options[i];


		// set data path
		TVPDataPath = TVPNormalizeStorageName(TVPNativeDataPath);
		TVPAddImportantLog( TVPFormatMessage( TVPInfoDataPath, TVPDataPath) );

		// set log output directory
		TVPSetLogLocation(TVPNativeDataPath);

		// increment TVPCommandLineArgumentGeneration
		TVPCommandLineArgumentGeneration++;
	}
}
//---------------------------------------------------------------------------
static void TVPDumpOptions()
{
	std::vector<ttstr>::const_iterator i;
 	ttstr options( TVPInfoSpecifiedOptionEarlierItemHasMorePriority );
	if(TVPProgramArguments.size())
	{
		for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end(); i++)
		{
			options += TJS_W(" ");
			options += *i;
		}
	}
	else
	{
		options += (const tjs_char*)TVPNone;
	}
	TVPAddImportantLog(options);
}
//---------------------------------------------------------------------------
bool TVPGetCommandLine(const tjs_char * name, tTJSVariant *value)
{
	TVPInitProgramArgumentsAndDataPath(false);

	tjs_int namelen = (tjs_int)TJS_strlen(name);
	std::vector<ttstr>::const_iterator i;
	for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end(); i++)
	{
		if(!TJS_strncmp(i->c_str(), name, namelen))
		{
			if(i->c_str()[namelen] == TJS_W('='))
			{
				// value is specified
				const tjs_char *p = i->c_str() + namelen + 1;
				if(value) *value = p;
				return true;
			}
			else if(i->c_str()[namelen] == 0)
			{
				// value is not specified
				if(value) *value = TJS_W("yes");
				return true;
			}
		}
	}
	return false;
}
//---------------------------------------------------------------------------
void TVPSetCommandLine(const tjs_char * name, const ttstr & value)
{
//	TVPInitProgramArgumentsAndDataPath(false);

	tjs_int namelen = (tjs_int)TJS_strlen(name);
	std::vector<ttstr>::iterator i;
	for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end(); i++)
	{
		if(!TJS_strncmp(i->c_str(), name, namelen))
		{
			if(i->c_str()[namelen] == TJS_W('=') || i->c_str()[namelen] == 0)
			{
				// value found
				*i = ttstr(i->c_str(), namelen) + TJS_W("=") + value;
				TVPCommandLineArgumentGeneration ++;
				if(TVPCommandLineArgumentGeneration == 0) TVPCommandLineArgumentGeneration = 1;
				return;
			}
		}
	}

	// value not found; insert argument into front
	TVPProgramArguments.insert(TVPProgramArguments.begin(), ttstr(name) + TJS_W("=") + value);
	TVPCommandLineArgumentGeneration ++;
	if(TVPCommandLineArgumentGeneration == 0) TVPCommandLineArgumentGeneration = 1;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPCheckPrintDataPath
//---------------------------------------------------------------------------
bool TVPCheckPrintDataPath()
{
#if 0
	// print current datapath to stdout, then exit
	for(int i=1; i<_argc; i++)
	{
		if(!strcmp(_argv[i], "-printdatapath")) // this does not refer TVPGetCommandLine
		{
			TVPInitProgramArgumentsAndDataPath(true);
			wprintf(L"%s\n", TVPNativeDataPath.c_str());

			return true; // processed
		}
	}
#endif
	return false;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPCheckAbout
//---------------------------------------------------------------------------
bool TVPCheckAbout(void)
{
#if 0
	if(TVPGetCommandLine(TJS_W("-about")))
	{
		Sleep(600);
		tjs_char msg[80];
		TJS_snprintf(msg, sizeof(msg)/sizeof(tjs_char), TVPInfoCpuClockRoughly, (int)TVPCPUClock);
		TVPAddImportantLog(msg);

		TVPShowVersionForm();
		return true;
	}
#endif
	return false;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPExecuteAsync
//---------------------------------------------------------------------------
static void TVPExecuteAsync( const std::wstring& progname)
{
#if 0
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWNORMAL;

	BOOL ret =
		CreateProcess(
			NULL,
			const_cast<LPTSTR>(progname.c_str()),
			NULL,
			NULL,
			FALSE,
			0,
			NULL,
			NULL,
			&si,
			&pi);

	if(ret)
	{
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
		return;
	}

	throw Exception(ttstr(TVPExecutionFail).AsStdString());
#endif
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPWaitWritePermit
//---------------------------------------------------------------------------
static bool TVPWaitWritePermit(const std::wstring& fn)
{
	return false;
#if 0
	tjs_int timeout = 10; // 10/1 = 5 seconds
	while(true)
	{
		HANDLE h = CreateFile(fn.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if(h != INVALID_HANDLE_VALUE)
		{
			CloseHandle(h);
			return true;
		}

		Sleep(500);
		timeout--;
		if(timeout == 0) return false;
	}
#endif
}
//---------------------------------------------------------------------------


#if 0
//---------------------------------------------------------------------------
// TVPShowUserConfig
//---------------------------------------------------------------------------
static void TVPShowUserConfig(std::string orgexe)
{
	TVPEnsureDataPathDirectory();

	Application->SetTitle( ChangeFileExt(ExtractFileName(orgexe), "") );
	TConfSettingsForm *form = new TConfSettingsForm(Application, true);
	form->InitializeConfig(orgexe);
	form->ShowModal();
	delete form;
}
//---------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------
// TVPExecuteUserConfig
//---------------------------------------------------------------------------
bool TVPExecuteUserConfig()
{
	return false;
	// check command line argument
#if 0
	tjs_int i;
	bool process = false;
	for(i=1; i<_argc; i++)
	{
		if(!strcmp(_argv[i], "-userconf")) // this does not refer TVPGetCommandLine
			process = true;
	}

	if(!process) return false;

	// execute user config mode
	//TVPShowUserConfig(ExePath());
	TVPShowUserConfig();

	// exit
	return true;
#endif
}
//---------------------------------------------------------------------------





