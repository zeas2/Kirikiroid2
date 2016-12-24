//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// CPU idetification / features detection routine
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <Windows.h>
#include "tvpgl_ia32_intf.h"
#include "DebugIntf.h"
#include "SysInitIntf.h"

#include "ThreadIntf.h"

#include "Exception.h"
#include <intrin.h>
#include "MsgIntf.h"

#pragma intrinsic(__rdtsc)

/*
	Note: CPU clock measuring routine is in EmergencyExit.cpp, reusing
	hot-key watching thread.
*/

//---------------------------------------------------------------------------
extern "C"
{
	tjs_uint32 TVPCPUType = 0; // CPU type
}

static bool TVPCPUChecked = false;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPGetCPUTypeForOne
//---------------------------------------------------------------------------
static void TVPGetCPUTypeForOne()
{
#if 1
	TVPCheckCPU(); // in detect_cpu.nas
#else
	__try {
		TVPCheckCPU(); // in detect_cpu.nas
	} __except(EXCEPTION_EXECUTE_HANDLER) {
		// exception had been ocured
		throw Exception( TVPCpuCheckFailure );
	}
#endif

	// check OSFXSR WinXP以降ならサポートしているので、もうこのチェックは無意味かな
#ifndef TJS_64BIT_OS
	if(TVPCPUFeatures & TVP_CPU_HAS_SSE)
	{
		__try {
			//__emit__(0x0f, 0x57, 0xc0); // xorps xmm0, xmm0   (SSE)
			__asm xorps xmm0, xmm0
		} __except(EXCEPTION_EXECUTE_HANDLER) {
			// exception had been ocured
			// execution of 'xorps' is failed (XMM registers not available)
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE;
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE2;
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE3;
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSSE3;
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE41;
			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE42;
		}
	}
#endif
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTVPCPUCheckThread
//---------------------------------------------------------------------------
class tTVPCPUCheckThread : public tTVPThread
{
	bool Succeeded;
public:
	tTVPCPUCheckThread(DWORD tam) : tTVPThread(true)
	{
		// set thread affinity mask
		Succeeded = true;

		SetThreadAffinityMask(GetHandle(), tam);

		Resume();
	}

	~tTVPCPUCheckThread()
	{
	}

	void WaitEnd()
	{
		Terminate();
		WaitFor();
	}

	bool GetSucceeded() const { return Succeeded; }

	void Execute(void);
} ;
//---------------------------------------------------------------------------
void tTVPCPUCheckThread::Execute(void)
{
	try
	{
		TVPGetCPUTypeForOne();
	}
	catch(...)
	{
		Succeeded = false;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
static ttstr TVPDumpCPUFeatures(tjs_uint32 features)
{
	ttstr ret;
#define TVP_DUMP_CPU(x, n) { ret += TJS_W("  ") TJS_W(n);  \
	if(features & x) ret += TJS_W(":yes"); else ret += TJS_W(":no"); }

	TVP_DUMP_CPU(TVP_CPU_HAS_FPU, "FPU");
	TVP_DUMP_CPU(TVP_CPU_HAS_MMX, "MMX");
	TVP_DUMP_CPU(TVP_CPU_HAS_3DN, "3DN");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE, "SSE");
	TVP_DUMP_CPU(TVP_CPU_HAS_CMOV, "CMOVcc");
	TVP_DUMP_CPU(TVP_CPU_HAS_E3DN, "E3DN");
	TVP_DUMP_CPU(TVP_CPU_HAS_EMMX, "EMMX");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE2, "SSE2");
	TVP_DUMP_CPU(TVP_CPU_HAS_TSC, "TSC");

	TVP_DUMP_CPU(TVP_CPU_HAS_TSCP, "TSCP");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE3, "SSE3");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSSE3, "SSSE3");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE41, "SSE41");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE42, "SSE42");
	TVP_DUMP_CPU(TVP_CPU_HAS_SSE4a, "SSE4A");
	TVP_DUMP_CPU(TVP_CPU_HAS_AVX, "AVX");
	TVP_DUMP_CPU(TVP_CPU_HAS_AVX2, "AVX2");
	TVP_DUMP_CPU(TVP_CPU_HAS_FMA3, "FMA3");
	TVP_DUMP_CPU(TVP_CPU_HAS_AES, "AES");
	TVP_DUMP_CPU(TVP_CPU_HAS_RDRAND, "RDRAND");
	TVP_DUMP_CPU(TVP_CPU_HAS_RDSEED, "RDSEED");

	return ret;
}
//---------------------------------------------------------------------------
static ttstr TVPDumpCPUInfo(tjs_int cpu_num)
{
	// dump detected cpu type
	ttstr features( TVPFormatMessage(TVPInfoCpuNumber,ttstr(cpu_num)) );

	features += TVPDumpCPUFeatures(TVPCPUFeatures);

	tjs_uint32 vendor = TVPCPUFeatures & TVP_CPU_VENDOR_MASK;

#undef TVP_DUMP_CPU
#define TVP_DUMP_CPU(x, n) { \
	if(vendor == x) features += TJS_W("  ") TJS_W(n); }

	TVP_DUMP_CPU(TVP_CPU_IS_INTEL, "Intel");
	TVP_DUMP_CPU(TVP_CPU_IS_AMD, "AMD");
	TVP_DUMP_CPU(TVP_CPU_IS_IDT, "IDT");
	TVP_DUMP_CPU(TVP_CPU_IS_CYRIX, "Cyrix");
	TVP_DUMP_CPU(TVP_CPU_IS_NEXGEN, "NexGen");
	TVP_DUMP_CPU(TVP_CPU_IS_RISE, "Rise");
	TVP_DUMP_CPU(TVP_CPU_IS_UMC, "UMC");
	TVP_DUMP_CPU(TVP_CPU_IS_TRANSMETA, "Transmeta");
	TVP_DUMP_CPU(TVP_CPU_IS_NSC, "NSC");
	TVP_DUMP_CPU(TVP_CPU_IS_COMPAQ, "Compaq");

	TVP_DUMP_CPU(TVP_CPU_IS_UNKNOWN, "Unknown");

#undef TVP_DUMP_CPU

	features += TJS_W("(") + ttstr((const tjs_nchar *)TVPCPUVendor) + TJS_W(")");

	if(TVPCPUName[0]!=0)
		features += TJS_W(" [") + ttstr((const tjs_nchar *)TVPCPUName) + TJS_W("]");

	features += TJS_W("  CPUID(1)/EAX=") + TJSInt32ToHex(TVPCPUID1_EAX);
	features += TJS_W(" CPUID(1)/EBX=") + TJSInt32ToHex(TVPCPUID1_EBX);

	TVPAddImportantLog(features);

	if(((TVPCPUID1_EAX >> 8) & 0x0f) <= 4)
		throw Exception( TVPFormatMessage( TVPCpuCheckFailureCpuFamilyOrLesserIsNotSupported, features).c_str() );

	return features;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPDetectCPU
//---------------------------------------------------------------------------
static void TVPDisableCPU(tjs_uint32 featurebit, const tjs_char *name)
{
	tTJSVariant val;
	ttstr str;
	if(TVPGetCommandLine(name, &val))
	{
		str = val;
		if(str == TJS_W("no"))
			TVPCPUType &=~ featurebit;
		else if(str == TJS_W("force"))
			TVPCPUType |= featurebit;
	}
}
//---------------------------------------------------------------------------
void TVPDetectCPU()
{
	if(TVPCPUChecked) return;
	TVPCPUChecked = true;

#ifdef TJS_64BIT_OS

	// get process affinity mask
	ULONGLONG pam = 1;
	HANDLE hp = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ::GetCurrentProcessId());
	if(hp)
	{
		ULONGLONG sam = 1;
		::GetProcessAffinityMask(hp, (PDWORD_PTR)&pam, (PDWORD_PTR)&sam);
		::CloseHandle(hp);
	}

	// for each CPU...
	ttstr cpuinfo;
	bool first = true;
	tjs_uint32 features = 0;
	for(tjs_int cpu = 0; cpu < 64; cpu++)
	{
		if(pam & (1ULL<<cpu))
		{
			tTVPCPUCheckThread * thread = new tTVPCPUCheckThread(1<<cpu);
			thread->WaitEnd();
			bool succeeded = thread->GetSucceeded();
			delete thread;
			if(!succeeded) throw Exception(L"CPU check failure");
			cpuinfo += TVPDumpCPUInfo(cpu) + TJS_W("\r\n");

			// mask features
			if(first)
			{
				features =  (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
				TVPCPUType = TVPCPUFeatures;
				first = false;
			}
			else
			{
				features &= (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
			}
		}
	}
#else
	// get process affinity mask
	DWORD pam = 1;
	HANDLE hp = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ::GetCurrentProcessId());
	if(hp)
	{
		DWORD sam = 1;
		::GetProcessAffinityMask(hp, &pam, &sam);
		::CloseHandle(hp);
	}

	// for each CPU...
	ttstr cpuinfo;
	bool first = true;
	tjs_uint32 features = 0;
	for(tjs_int cpu = 0; cpu < 32; cpu++)
	{
		if(pam & (1<<cpu))
		{
			tTVPCPUCheckThread * thread = new tTVPCPUCheckThread(1<<cpu);
			thread->WaitEnd();
			bool succeeded = thread->GetSucceeded();
			delete thread;
			if(!succeeded) throw Exception(L"CPU check failure");
			cpuinfo += TVPDumpCPUInfo(cpu) + TJS_W("\r\n");

			// mask features
			if(first)
			{
				features =  (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
				TVPCPUType = TVPCPUFeatures;
				first = false;
			}
			else
			{
				features &= (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
			}
		}
	}
#endif
	TVPCPUType &= ~ TVP_CPU_FEATURE_MASK;
	TVPCPUType |= features;

	// Disable or enable cpu features by option
	TVPDisableCPU(TVP_CPU_HAS_MMX,  TJS_W("-cpummx"));
	TVPDisableCPU(TVP_CPU_HAS_3DN,  TJS_W("-cpu3dn"));
	TVPDisableCPU(TVP_CPU_HAS_SSE,  TJS_W("-cpusse"));
	TVPDisableCPU(TVP_CPU_HAS_CMOV, TJS_W("-cpucmov"));
	TVPDisableCPU(TVP_CPU_HAS_E3DN, TJS_W("-cpue3dn"));
	TVPDisableCPU(TVP_CPU_HAS_EMMX, TJS_W("-cpuemmx"));
	TVPDisableCPU(TVP_CPU_HAS_SSE2, TJS_W("-cpusse2"));

	TVPDisableCPU(TVP_CPU_HAS_SSE3, TJS_W("-cpusse3"));
	TVPDisableCPU(TVP_CPU_HAS_SSSE3, TJS_W("-cpussse3"));
	TVPDisableCPU(TVP_CPU_HAS_SSE41, TJS_W("-cpusse41"));
	TVPDisableCPU(TVP_CPU_HAS_SSE42, TJS_W("-cpusse42"));
	TVPDisableCPU(TVP_CPU_HAS_SSE4a, TJS_W("-cpusse4a"));
	TVPDisableCPU(TVP_CPU_HAS_AVX, TJS_W("-cpuavx"));
	TVPDisableCPU(TVP_CPU_HAS_AVX2, TJS_W("-cpuavx2"));
	TVPDisableCPU(TVP_CPU_HAS_FMA3, TJS_W("-cpufma3"));
	TVPDisableCPU(TVP_CPU_HAS_AES, TJS_W("-cpuaes"));

	if(TVPCPUType == 0)
		throw Exception( TVPFormatMessage(TVPCpuCheckFailureNotSupprtedCpu, cpuinfo).c_str() );

	TVPAddImportantLog( TVPFormatMessage(TVPInfoFinallyDetectedCpuFeatures,TVPDumpCPUFeatures(TVPCPUType)) );
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// jpeg and png loader support functions
//---------------------------------------------------------------------------
unsigned long MMXReady = 0;
extern "C"
{
	void __fastcall CheckMMX(void)
	{
		TVPDetectCPU();
		MMXReady = TVPCPUType & TVP_CPU_HAS_MMX;
	}
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPGetCPUType
//---------------------------------------------------------------------------
tjs_uint32 TVPGetCPUType()
{
	TVPDetectCPU();
	return TVPCPUType;
}
//---------------------------------------------------------------------------

