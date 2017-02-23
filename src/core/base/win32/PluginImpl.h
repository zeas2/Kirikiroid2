//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Plugins" class implementation / Service for plug-ins
//---------------------------------------------------------------------------
#ifndef PluginImplH
#define PluginImplH
//---------------------------------------------------------------------------
#include <memory.h>

#include "PluginIntf.h"
#include "win32type.h"
#ifdef TVP_SUPPORT_KPI
	#include "kmp_pi.h"
#endif
#if 0

//---------------------------------------------------------------------------
/*[*/
//---------------------------------------------------------------------------
// iTVPFunctionExporter, exporting main module's functions
//---------------------------------------------------------------------------
struct iTVPFunctionExporter
{
	virtual bool TJS_INTF_METHOD QueryFunctions(const tjs_char **name, void **function,
		tjs_uint count) = 0;
	virtual bool TJS_INTF_METHOD QueryFunctionsByNarrowString(const char **name,
		void **function, tjs_uint count) = 0;
};
//---------------------------------------------------------------------------


/*]*/
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
struct ITSSModule;
struct IWaveUnpacker;
struct ITSSStorageProvider;
extern "C"
{
	iTVPFunctionExporter * __stdcall TVPGetFunctionExporter();

	// V2 plug-in
	typedef HRESULT (_stdcall * tTVPV2LinkProc)(iTVPFunctionExporter *);
	typedef HRESULT (_stdcall * tTVPV2UnlinkProc)();

	// TSS
	typedef HRESULT (_stdcall * tTVPGetModuleInstanceProc)(ITSSModule **out,
		ITSSStorageProvider *provider, IStream * config, HWND mainwin);
	typedef ULONG (_stdcall * tTVPGetModuleThreadModelProc)(void);
	typedef HRESULT (_stdcall * tTVPShowConfigWindowProc)(HWND parentwin,
		IStream * storage );
	typedef ULONG (_stdcall * tTVPCanUnloadNowProc)(void);

#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
	// WaveUnpacker
	typedef HRESULT (_stdcall * tTVPCreateWaveUnpackerProc)(IStream *storage,long size,
		char *name,IWaveUnpacker **out); // old WaveUnpacker stuff
#endif
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
struct ITSSWaveDecoder;
extern ITSSWaveDecoder * TVPSearchAvailTSSWaveDecoder(const ttstr & storage, const ttstr & extension);
#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
class IWaveUnpacker;
extern IWaveUnpacker * TVPSearchAvailWaveUnpacker(const ttstr & storage, IStream **stream);
#endif
#ifdef TVP_SUPPORT_KPI
extern void * TVPSearchAvailKMPWaveDecoder(const ttstr & storage, KMPMODULE ** module,
	SOUNDINFO * info);
#endif
extern void TVPAddExportFunction(const tjs_char *name, void *ptr);
extern void TVPAddExportFunction(const char *name, void *ptr);
TJS_EXP_FUNC_DEF(void, TVPThrowPluginUnboundFunctionError, (const char *funcname));
TJS_EXP_FUNC_DEF(void, TVPThrowPluginUnboundFunctionError, (const tjs_char *funcname));
#endif
inline TJS_EXP_FUNC_DEF(void *, TVP_malloc, (size_t size)) { return malloc(size); }
// inline TJS_EXP_FUNC_DEF(void *, TVP_realloc, (void *pp, size_t size)) { return realloc(pp, size); }
inline TJS_EXP_FUNC_DEF(void, TVP_free, (void *pp)) { return free(pp); }
TJS_EXP_FUNC_DEF(tjs_int, TVPGetAutoLoadPluginCount, ());
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(int, ZLIB_uncompress, (unsigned char *dest, unsigned long *destlen, const unsigned char *source, unsigned long sourcelen));
TJS_EXP_FUNC_DEF(int, ZLIB_compress, (unsigned char *dest, unsigned long *destlen, const unsigned char *source, unsigned long sourcelen));
TJS_EXP_FUNC_DEF(int, ZLIB_compress2, (unsigned char *dest, unsigned long *destlen, const unsigned char *source, unsigned long sourcelen, int level));

/*[*/

//---------------------------------------------------------------------------
// this stub includes exported function from Independent implementation of
// MD5 (RFC 1321) by Aladdin Enterprises.
//---------------------------------------------------------------------------
// TVP_md5_init, TVP_md5_append, TVP_md5_finish are exported
typedef struct TVP_md5_state_s { tjs_uint8 buffer[4*2+8+4*4+8+64]; } TVP_md5_state_t; // md5_state_t 
//---------------------------------------------------------------------------


/*]*/

TJS_EXP_FUNC_DEF(void, TVP_md5_init, (TVP_md5_state_t *pms));
TJS_EXP_FUNC_DEF(void, TVP_md5_append, (TVP_md5_state_t *pms, const tjs_uint8 *data, int nbytes));
TJS_EXP_FUNC_DEF(void, TVP_md5_finish, (TVP_md5_state_t *pms, tjs_uint8 *digest));

//TJS_EXP_FUNC_DEF(HWND, TVPGetApplicationWindowHandle, ());
TJS_EXP_FUNC_DEF(void, TVPProcessApplicationMessages, ());
TJS_EXP_FUNC_DEF(void, TVPHandleApplicationMessage, ());

TJS_EXP_FUNC_DEF(bool, TVPRegisterGlobalObject, (const tjs_char *name, iTJSDispatch2 * dsp));
TJS_EXP_FUNC_DEF(bool, TVPRemoveGlobalObject, (const tjs_char *name));

/*[*/
//---------------------------------------------------------------------------
// data types for TVPDoTryBlock
//---------------------------------------------------------------------------
	// TVPDoTryBlock executes specified 'tryblock' in try block.
	// If any exception occured,
	// 'catchblock' is to be executed. 'data' is applicatoin defined data
	// block passed to 'tryblock' and 'catchblock' and 'finallyblock'.
	// if the 'catchblock' returns true, the exception is to be rethrown.
	// if false then the exception is to be vanished.
	// 'finallyblock' can be null, is to be executed whatever the exception
	// is generated or not.

struct tTVPExceptionDesc
{
	ttstr type; // the exception type, currently 'eTJS' or 'unknown'
	ttstr message; // the exception message (if exists. otherwise empty).
};

typedef void (TJS_USERENTRY *tTVPTryBlockFunction)(void * data);
typedef bool (TJS_USERENTRY *tTVPCatchBlockFunction)(void * data, const tTVPExceptionDesc & desc);
typedef void (TJS_USERENTRY *tTVPFinallyBlockFunction)(void *data);
//---------------------------------------------------------------------------




/*]*/

TJS_EXP_FUNC_DEF(void, TVPDoTryBlock, (tTVPTryBlockFunction tryblock, tTVPCatchBlockFunction catchblock, tTVPFinallyBlockFunction finallyblock, void *data));


//TJS_EXP_FUNC_DEF(bool, TVPGetFileVersionOf, (const wchar_t* module_filename, tjs_int &major, tjs_int &minor, tjs_int &release, tjs_int &build));


//---------------------------------------------------------------------------
extern bool TVPPluginUnloadedAtSystemExit;
extern void TVPLoadPluigins(void);
//---------------------------------------------------------------------------


#endif
