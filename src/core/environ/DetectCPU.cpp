//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// CPU idetification / features detection routine
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "cpu_types.h"
#include "DebugIntf.h"
#include "SysInitIntf.h"

#include "ThreadIntf.h"
#include "Exception.h"

/*
	Note: CPU clock measuring routine is in EmergencyExit.cpp, reusing
	hot-key watching thread.
*/

//---------------------------------------------------------------------------
extern "C"
{
	tjs_uint32 TVPCPUType = 0; // CPU type
    tjs_uint32 TVPCPUFeatures	=	0;
}

static bool TVPCPUChecked = false;
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPGetCPUTypeForOne
//---------------------------------------------------------------------------
static void TVPGetCPUTypeForOne()
{
	try
	{
        TVPCPUFeatures = 0;

		//TVPCheckCPU(); // in detect_cpu.nas
	}
	catch(.../*EXCEPTION_EXECUTE_HANDLER*/)
	{
		// exception had been ocured
		throw Exception("CPU check failure.");
	}

	// check OSFXSR
// 	if(TVPCPUFeatures & TVP_CPU_HAS_SSE)
// 	{
// 		__try
// 		{
// 			__emit__(0x0f, 0x57, 0xc0); // xorps xmm0, xmm0   (SSE)
// 		}
// 		__except(EXCEPTION_EXECUTE_HANDLER)
// 		{
// 			// exception had been ocured
// 			// execution of 'xorps' is failed (XMM registers not available)
// 			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE;
// 			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE2;
// 		}
// 	}

}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
static ttstr TVPDumpCPUFeatures(tjs_uint32 features)
{
	ttstr ret;
// #define TVP_DUMP_CPU(x, n) { ret += TJS_W("  ") TJS_W(n);  \
// 	if(features & x) ret += TJS_W(":yes"); else ret += TJS_W(":no"); }
// 
// 	TVP_DUMP_CPU(TVP_CPU_HAS_FPU, "FPU");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_MMX, "MMX");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_3DN, "3DN");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_SSE, "SSE");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_CMOV, "CMOVcc");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_E3DN, "E3DN");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_EMMX, "EMMX");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_SSE2, "SSE2");
// 	TVP_DUMP_CPU(TVP_CPU_HAS_TSC, "TSC");

	return ret;
}
//---------------------------------------------------------------------------
static ttstr TVPDumpCPUInfo(tjs_int cpu_num)
{
	// dump detected cpu type
	ttstr features(TJS_W("(info) CPU #") + ttstr(cpu_num) +
		TJS_W(" : "));

	features += TVPDumpCPUFeatures(TVPCPUFeatures);

	tjs_uint32 vendor = TVPCPUFeatures & TVP_CPU_VENDOR_MASK;

// #undef TVP_DUMP_CPU
// #define TVP_DUMP_CPU(x, n) { \
// 	if(vendor == x) features += TJS_W("  ") TJS_W(n); }
// 
// 	TVP_DUMP_CPU(TVP_CPU_IS_INTEL, "Intel");
// 	TVP_DUMP_CPU(TVP_CPU_IS_AMD, "AMD");
// 	TVP_DUMP_CPU(TVP_CPU_IS_IDT, "IDT");
// 	TVP_DUMP_CPU(TVP_CPU_IS_CYRIX, "Cyrix");
// 	TVP_DUMP_CPU(TVP_CPU_IS_NEXGEN, "NexGen");
// 	TVP_DUMP_CPU(TVP_CPU_IS_RISE, "Rise");
// 	TVP_DUMP_CPU(TVP_CPU_IS_UMC, "UMC");
// 	TVP_DUMP_CPU(TVP_CPU_IS_TRANSMETA, "Transmeta");
// 
// 	TVP_DUMP_CPU(TVP_CPU_IS_UNKNOWN, "Unknown");
// 
// #undef TVP_DUMP_CPU

//	features += TJS_W("(") + ttstr((const tjs_nchar *)TVPCPUVendor) + TJS_W(")");

// 	if(TVPCPUName[0]!=0)
// 		features += TJS_W(" [") + ttstr((const tjs_nchar *)TVPCPUName) + TJS_W("]");

// 	features += TJS_W("  CPUID(1)/EAX=") + TJSInt32ToHex(TVPCPUID1_EAX);
// 	features += TJS_W(" CPUID(1)/EBX=") + TJSInt32ToHex(TVPCPUID1_EBX);

	TVPAddImportantLog(features);

// 	if(((TVPCPUID1_EAX >> 8) & 0x0f) <= 4)
// 		throw Exception("CPU check failure: CPU family 4 or lesser is not supported\r\n"+
// 		features.AsAnsiString());

	return features;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPDetectCPU
//---------------------------------------------------------------------------
static void TVPDisableCPU(tjs_uint32 featurebit, const tjs_char *name)
{
#if 0
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
#endif
}

#if defined(WIN32) || defined(__ANDROID__)
#include <cpu-features.h>
#endif
//---------------------------------------------------------------------------
void TVPDetectCPU()
{
    if(TVPCPUChecked) return;
    TVPCPUChecked = true;

	//if(SDL_HasSSE2()) TVPCPUFeatures |= TVP_CPU_HAS_SSE2 | TVP_CPU_HAS_EMMX;
	//if(SDL_HasSSE())  TVPCPUFeatures |= TVP_CPU_HAS_SSE;
	//if(SDL_HasMMX())  TVPCPUFeatures |= TVP_CPU_HAS_MMX;
	//if(SDL_HasSSE2()) TVPCPUFeatures |= TVP_CPU_HAS_SSE2;
#if defined(__ANDROID__) || defined(WIN32)
    //if (android_getCpuFamily() == ANDROID_CPU_FAMILY_ARM) {
        TVPCPUFeatures |= TVP_CPU_FAMILY_ARM; // must be arm
#if defined(__arm64__) || defined(__aarch64__) || defined(__LP64__)
		TVPCPUFeatures |= TVP_CPU_HAS_NEON; // aka. asimd
#else
        if((android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) != 0) {
            TVPCPUFeatures |= TVP_CPU_HAS_NEON;
        }
#endif
    //}
#endif
#ifdef __APPLE__
    // must be iOS
    TVPCPUFeatures |= TVP_CPU_FAMILY_ARM | TVP_CPU_HAS_NEON;
#endif

    tjs_uint32 features = 0;
    features =  (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
    TVPCPUType &= ~ TVP_CPU_FEATURE_MASK;
    TVPCPUType |= features;

// 	// get process affinity mask
// 	DWORD pam = 1;
// 	HANDLE hp = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());
// 	if(hp)
// 	{
// 		DWORD sam = 1;
// 		GetProcessAffinityMask(hp, &pam, &sam);
// 		CloseHandle(hp);
// 	}
// 
// 	// for each CPU...
// 	ttstr cpuinfo;
// 	bool first = true;
// 	for(tjs_int cpu = 0; cpu < 32; cpu++)
// 	{
// 		if(pam & (1<<cpu))
// 		{
// 			tTVPCPUCheckThread * thread = new tTVPCPUCheckThread(1<<cpu);
// 			thread->WaitEnd();
// 			bool succeeded = thread->GetSucceeded();
// 			delete thread;
// 			if(!succeeded) throw Exception("CPU check failure");
// 			cpuinfo += TVPDumpCPUInfo(cpu) + TJS_W("\r\n");
// 
// 			// mask features
// 			if(first)
// 			{
// 				features =  (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
// 				TVPCPUType = TVPCPUFeatures;
// 				first = false;
// 			}
// 			else
// 			{
// 				features &= (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
// 			}
// 		}
// 	}

	// Disable or enable cpu features by option
// 	TVPDisableCPU(TVP_CPU_HAS_MMX,  TJS_W("-cpummx"));
// 	TVPDisableCPU(TVP_CPU_HAS_3DN,  TJS_W("-cpu3dn"));
// 	TVPDisableCPU(TVP_CPU_HAS_SSE,  TJS_W("-cpusse"));
// 	TVPDisableCPU(TVP_CPU_HAS_CMOV, TJS_W("-cpucmov"));
// 	TVPDisableCPU(TVP_CPU_HAS_E3DN, TJS_W("-cpue3dn"));
// 	TVPDisableCPU(TVP_CPU_HAS_EMMX, TJS_W("-cpuemmx"));
// 	TVPDisableCPU(TVP_CPU_HAS_SSE2, TJS_W("-cpusse2"));
 	TVPDisableCPU(TVP_CPU_HAS_NEON, TJS_W("-cpuneon"));

// 	if(TVPCPUType == 0)
// 		throw Exception("CPU check failure: Not supported CPU\r\n" +
// 		cpuinfo.AsAnsiString());
// 
// 	TVPAddImportantLog(TJS_W("(info) finally detected CPU features : ") +
//     	TVPDumpCPUFeatures(TVPCPUType));
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// jpeg and png loader support functions
//---------------------------------------------------------------------------
unsigned long MMXReady = 0;
extern "C"
{
	void CheckMMX(void)
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


