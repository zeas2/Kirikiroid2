//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "System" class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

// #include <shellapi.h>
// #include <shlobj.h>

#include "GraphicsLoaderImpl.h"

#include "SystemImpl.h"
#include "SystemIntf.h"
#include "SysInitIntf.h"
#include "StorageIntf.h"
#include "StorageImpl.h"
#include "TickCount.h"
#include "ComplexRect.h"
#include "WindowImpl.h"
#include "SystemControl.h"
#include "DInputMgn.h"

#include "Application.h"
#include "TVPScreen.h"
//#include "CompatibleNativeFuncs.h"
#include "DebugIntf.h"
//#include "VersionFormUnit.h"
#include "vkdefine.h"
#include "ScriptMgnIntf.h"
#include "tjsArray.h"
#include "Platform.h"

//---------------------------------------------------------------------------
static ttstr TVPAppTitle;
static bool TVPAppTitleInit = false;
//---------------------------------------------------------------------------



#if 0
//---------------------------------------------------------------------------
// TVPShowSimpleMessageBox
//---------------------------------------------------------------------------
static void TVPShowSimpleMessageBox(const ttstr & text, const ttstr & caption)
{
	HWND hWnd = TVPGetModalWindowOwnerHandle();
	if( hWnd == INVALID_HANDLE_VALUE ) {
		hWnd = NULL;
	}
	::MessageBox( hWnd, text.AsStdString().c_str(), caption.AsStdString().c_str(), MB_OK|MB_ICONINFORMATION );
}
//---------------------------------------------------------------------------
#endif

bool TVPGetKeyMouseAsyncState(tjs_uint keycode, bool getcurrent);
//---------------------------------------------------------------------------
// TVPGetAsyncKeyState
//---------------------------------------------------------------------------
bool TVPGetAsyncKeyState(tjs_uint keycode, bool getcurrent)
{
	// get keyboard state asynchronously.
	// return current key state if getcurrent is true.
	// otherwise, return whether the key is pushed during previous call of
	// TVPGetAsyncKeyState at the same keycode.

	if(keycode >= VK_PAD_FIRST  && keycode <= VK_PAD_LAST)
	{
		// JoyPad related keys are treated in DInputMgn.cpp
		return TVPGetJoyPadAsyncState(keycode, getcurrent);
	}

	return TVPGetKeyMouseAsyncState(keycode, getcurrent);
#if 0
	if(keycode == VK_LBUTTON || keycode == VK_RBUTTON)
	{
		// check whether the mouse button is swapped
		if(GetSystemMetrics(SM_SWAPBUTTON))
		{
			// mouse button had been swapped; swap key code
			if(keycode == VK_LBUTTON)
				keycode = VK_RBUTTON;
			else
				keycode = VK_LBUTTON;
		}
	}

	return 0!=( GetAsyncKeyState(keycode) & ( getcurrent?0x8000:0x0001) );
#endif
}
//---------------------------------------------------------------------------






#if 0
//---------------------------------------------------------------------------
// TVPGetPlatformName
//---------------------------------------------------------------------------
ttstr TVPGetPlatformName()
{
	SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo( &sysInfo );
	switch( sysInfo.wProcessorArchitecture )
	{
		case PROCESSOR_ARCHITECTURE_AMD64:
		case PROCESSOR_ARCHITECTURE_IA64:
			return ttstr(TJS_W("Win64"));

		case PROCESSOR_ARCHITECTURE_INTEL:
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
		default:
			return ttstr(TJS_W("Win32"));
	}
}
//---------------------------------------------------------------------------


typedef void (WINAPI *RtlGetVersionFunc)(OSVERSIONINFOEX* );
//---------------------------------------------------------------------------
// TVPGetOSName
//---------------------------------------------------------------------------
ttstr TVPGetOSName()
{
	OSVERSIONINFOEX ovi;
	ovi.dwOSVersionInfoSize = sizeof(ovi);

	bool isGetVersion = false;
	HMODULE hModule = ::LoadLibrary( L"ntdll.dll" );
	if( hModule ) {
		RtlGetVersionFunc func;
		func = (RtlGetVersionFunc)::GetProcAddress( hModule, "RtlGetVersion" );
		if( func ) {
			func( &ovi );
			isGetVersion = true;
		}
		::FreeLibrary( hModule );
		hModule = NULL;
	}
	if( isGetVersion == false ) {
		GetVersionEx((OSVERSIONINFO*)&ovi);
	}
	tjs_char buf[256];
	const tjs_char *osname = NULL;

	switch(ovi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32s:
		osname = TJS_W("Win32s"); break;
	case VER_PLATFORM_WIN32_WINDOWS:
		switch((ovi.dwBuildNumber&0xffff ))
		{
		case 1998:
			osname = TJS_W("Windows 98"); break;
		case 95:
			osname = TJS_W("Windows 95"); break;
		default:
			osname = TJS_W("Win9x"); break;
		}
		break;
	case VER_PLATFORM_WIN32_NT:
		if( ovi.dwMajorVersion == 5 ) {
			switch(ovi.dwMinorVersion) {
			case 0:
				osname = TJS_W("Windows 2000");
				break;
			case 1:
				osname = TJS_W("Windows XP");
				break;
			case 2:
				osname = TJS_W("Windows Server 2003");
				break;
			}
		} else if( ovi.dwMajorVersion == 6 ) {
			switch(ovi.dwMinorVersion) {
			case 0:
				if( ovi.wProductType == VER_NT_WORKSTATION )
					osname = TJS_W("Windows Vista");
				else
					osname = TJS_W("Windows Server 2008");
				break;
			case 1:
				if( ovi.wProductType == VER_NT_WORKSTATION )
					osname = TJS_W("Windows 7");
				else
					osname = TJS_W("Windows Server 2008 R2");
				break;
			case 2:
				if( ovi.wProductType == VER_NT_WORKSTATION )
					osname = TJS_W("Windows 8");
				else
					osname = TJS_W("Windows Server 2012");
				break;
			case 3:
				if( ovi.wProductType == VER_NT_WORKSTATION )
					osname = TJS_W("Windows 8.1");
				else
					osname = TJS_W("Windows Server 2012 R2");
				break;
			case 4:
				if( ovi.wProductType == VER_NT_WORKSTATION )
					osname = TJS_W("Windows 10");
				break;
			}
		}
		if( osname == NULL ) osname = TJS_W("Windows NT");
		break;
	default:
		osname = TJS_W("Unknown"); break;
	}

	TJS_snprintf(buf, sizeof(buf)/sizeof(tjs_char), TJS_W("%ls %d.%d.%d "), osname, ovi.dwMajorVersion,
		ovi.dwMinorVersion, ovi.dwBuildNumber&0xfff);

	ttstr str(buf);
	str += ttstr(ovi.szCSDVersion);

	return str;
}
//---------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------
// TVPGetOSBits
//---------------------------------------------------------------------------
tjs_int TVPGetOSBits()
{
#if 0
	SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo( &sysInfo );
	switch( sysInfo.wProcessorArchitecture )
	{
		case PROCESSOR_ARCHITECTURE_AMD64:
		case PROCESSOR_ARCHITECTURE_IA64:
			return 64;
		case PROCESSOR_ARCHITECTURE_INTEL:
		case PROCESSOR_ARCHITECTURE_UNKNOWN:
		default:
			return 32;
	}
#endif
	return 32;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPShellExecute
//---------------------------------------------------------------------------
bool TVPShellExecute(const ttstr &target, const ttstr &param)
{
	// open or execute target file
//	ttstr file = TVPGetNativeName(TVPNormalizeStorageName(target));
#if 0
	return TVPIsExistentStorageNoSearchNoNormalize(target);
	if (::ShellExecute(NULL, NULL,
		target.c_str(),
		param.IsEmpty() ? NULL : param.c_str(),
		L"",
		SW_SHOWNORMAL)
		<=(void *)32)
	{
		return false;
	}
	else
#endif
	{
		return true;
	}
}
//---------------------------------------------------------------------------

static tTJSVariant RegisterData;
ttstr TVPGetAppDataPath();
void TVPExecuteStorage(const ttstr &name, tTJSVariant *result, bool isexpression,
	const tjs_char * modestr);
static void InitRegisterData()
{
	static bool dataInited = false;
	if (!dataInited) {
		ttstr regfile = TVPGetAppDataPath() + TJS_W("RegisterData.tjs");
		if (TVPIsExistentStorageNoSearch(regfile)) {
			TVPExecuteStorage(regfile, &RegisterData, true, TJS_W(""));
		}
	}
}

//---------------------------------------------------------------------------
// TVPReadRegValue
//---------------------------------------------------------------------------
static void TVPReadRegValue(tTJSVariant &result, const ttstr & key)
{
	// open specified registry key
	if(key.IsEmpty()) { result.Clear(); return; }

	// check whether the key contains root key name
	//HKEY root = HKEY_CURRENT_USER;
	const tjs_char *key_p = key.c_str();

	InitRegisterData();
	// search value name
	tTJSVariant CurrentNode = RegisterData;
	const tjs_char *start = key_p;
	while (*start && CurrentNode.Type() != tvtObject) {
		iTJSDispatch2 *pObj;

		switch (*key_p) {
		case '\\':
		case '/':
			++key_p;
		case '\0':
			start = key_p;
			if (CurrentNode.Type() != tvtObject) {
				CurrentNode.Clear();
				break;
			}
			pObj = CurrentNode.AsObject();
			if (!pObj) {
				CurrentNode.Clear();
				break;
			}
			if (!TJS_SUCCEEDED(pObj->PropGet(TJS_MEMBERMUSTEXIST, ttstr(start, key_p - start - 1).c_str(), 0, &CurrentNode, pObj))) {
				CurrentNode.Clear();
				break;
			}
			start = key_p;
			continue;
		default:
			++key_p;
			continue;
		}
	}
	if (*start) {
		CurrentNode.Clear();
		return;
	}
	result = CurrentNode;
#if 0
	if(key.StartsWith(TJS_W("HKEY_CLASSES_ROOT")))
	{
		key_p += 17;
		root = HKEY_CLASSES_ROOT;
	}
	else if(key.StartsWith(TJS_W("HKEY_CURRENT_CONFIG")))
	{
		key_p += 19;
		root = HKEY_CURRENT_CONFIG;
	}
	else if(key.StartsWith(TJS_W("HKEY_CURRENT_USER")))
	{
		key_p += 17;
		root = HKEY_CURRENT_USER;
	}
	else if(key.StartsWith(TJS_W("HKEY_LOCAL_MACHINE")))
	{
		key_p += 18;
		root = HKEY_LOCAL_MACHINE;
	}
	else if(key.StartsWith(TJS_W("HKEY_USERS")))
	{
		key_p += 10;
		root = HKEY_USERS;
	}
	else if(key.StartsWith(TJS_W("HKEY_PERFORMANCE_DATA")))
	{
		key_p += 21;
		root = HKEY_PERFORMANCE_DATA;
	}
	else if(key.StartsWith(TJS_W("HKEY_DYN_DATA")))
	{
		key_p += 13;
		root = HKEY_DYN_DATA;
	}

	if(*key_p == TJS_W('\\')) key_p ++;

	// search value name
	const tjs_char *start = key_p;
	key_p += TJS_strlen(key_p);
	key_p--;
	while(start <= key_p && *key_p != TJS_W('\\')) key_p--;
	ttstr valuename(key_p+1);
	if(key_p < start || *key_p != TJS_W('\\')) key_p++;

	ttstr keyname(start, (int)(key_p - start));

	// open key
	HKEY handle;
	LONG res = RegOpenKeyEx(root, keyname.AsStdString().c_str(), 0, KEY_READ, &handle);
	if(res != ERROR_SUCCESS) { result.Clear(); return; }

	// try query value size and read key
	DWORD size;
	DWORD type;

	// query size
	res = RegQueryValueEx(handle, valuename.c_str(), 0, &type, NULL, &size);

	if(res != ERROR_SUCCESS)
	{
		RegCloseKey(handle);
		result.Clear();
		return;
	}


	switch(type)
	{
	case REG_DWORD:
//	case REG_DWORD_LITTLE_ENDIAN: // is actually the same as REG_DWORD
	case REG_DWORD_BIG_ENDIAN:
	case REG_EXPAND_SZ:
	case REG_SZ:
		break; // these should be OK

	case REG_MULTI_SZ: // sorry not yet implemented
	case REG_BINARY:
	case REG_LINK:
	case REG_NONE:
	case REG_RESOURCE_LIST:
	default:
		// not capable types
		RegCloseKey(handle);
		result.Clear();
		return;
	}

	while(true)
	{
		tjs_uint8 * data = new tjs_uint8[size];

		try
		{
			DWORD size2 = size;
			res = RegQueryValueEx(handle, valuename.c_str(), 0, NULL, data, &size2);

			if(res == ERROR_MORE_DATA)
			{
				// more data required
				delete [] data;
				size += 1024;
				continue;
			}
			else if(res != ERROR_SUCCESS)
			{
				RegCloseKey(handle);
				result.Clear();
				return;
			}

			// query succeeded


			// store data into result
			switch(type)
			{
			case REG_DWORD:
//			case REG_DWORD_LITTLE_ENDIAN:
				result = (tTVInteger)*(DWORD*)data;
				break;

			case REG_DWORD_BIG_ENDIAN:
				{
					DWORD val = *(DWORD*)data;
					val = (val >> 24) + ((val >> 8) & 0x0000ff00) +
						((val << 8) & 0x00ff0000) + (val << 24);
					result = (tTVInteger)val;
			  	}
				break;

			case REG_EXPAND_SZ:
			case REG_SZ:
				// data is stored in unicode
				result = ttstr((const tjs_char*)data);
				break;
			}
		}
		catch(...)
		{
			RegCloseKey(handle);
			delete [] data;
			throw;
		}
		RegCloseKey(handle);
		delete [] data;

		break;
	}
#endif
}
//---------------------------------------------------------------------------



#if 0
//---------------------------------------------------------------------------
// Static function for retrieving special folder path
//---------------------------------------------------------------------------
static ttstr TVPGetSpecialFolderPath(int csidl)
{
	WCHAR path[MAX_PATH+1];
	if(!SHGetSpecialFolderPath(NULL, path, csidl, false))
		return ttstr();
	return ttstr(path);
}
//---------------------------------------------------------------------------
#endif



//---------------------------------------------------------------------------
// TVPGetPersonalPath
//---------------------------------------------------------------------------
ttstr TVPGetPersonalPath()
{
#if 0
	// Retrieve personal directory;
	// This usually refers "My Documents".
	// If this is not exist, returns application data path, then exe path.
	// for windows vista, this refers application data path.
	ttstr path;
	path = TVPGetSpecialFolderPath(CSIDL_PERSONAL);
	if(path.IsEmpty())
		path = TVPGetSpecialFolderPath(CSIDL_APPDATA);
	
	if(!path.IsEmpty())
	{
		path = TVPNormalizeStorageName(path);
		if(path.GetLastChar() != TJS_W('/')) path += TJS_W('/');
		return path;
	}
#endif
	return TVPGetAppPath();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPGetAppDataPath
//---------------------------------------------------------------------------
ttstr TVPGetAppDataPath()
{
#if 0
	// Retrieve application data directory;
	// If this is not exist, returns application exe path.

	ttstr path = TVPGetSpecialFolderPath(CSIDL_APPDATA);
	
	if(!path.IsEmpty())
	{
		path = TVPNormalizeStorageName(path);
		if(path.GetLastChar() != TJS_W('/')) path += TJS_W('/');
		return path;
	}
#endif
	return TVPGetAppPath();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// TVPGetSavedGamesPath
//---------------------------------------------------------------------------
ttstr TVPGetSavedGamesPath()
{
#if 0
	ttstr path;
	PWSTR ppszPath = NULL;
	HRESULT hr = ::SHGetKnownFolderPath(FOLDERID_SavedGames, 0, NULL, &ppszPath);
	if( hr == S_OK ) {
		path = ppszPath;
		::CoTaskMemFree( ppszPath );
	}
	return path;
#endif
	return TVPGetAppPath();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPCreateAppLock
//---------------------------------------------------------------------------
bool TVPCreateAppLock(const ttstr &lockname)
{
#if 0
	// lock application using mutex
	CreateMutex(NULL, TRUE, lockname.c_str());

	if(GetLastError())
	{
		return false; // already running
	}
#endif

	// No need to release the mutex object because the mutex is automatically
	// released when the calling thread exits.

	return true;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
enum tTVPTouchDevice {
	tdNone				= 0,
	tdIntegratedTouch	= 0x00000001,
	tdExternalTouch		= 0x00000002,
	tdIntegratedPen		= 0x00000004,
	tdExternalPen		= 0x00000008,
	tdMultiInput		= 0x00000040,
	tdDigitizerReady	= 0x00000080,
	tdMouse				= 0x00000100,
	tdMouseWheel		= 0x00000200
};
/**
 * タッチデバイス(とマウス)の接続状態を取得する
 **/
static int TVPGetSupportTouchDevice()
{
	int result = 0;
#if 0
	if( procRegisterTouchWindow ) {
		int value = ::GetSystemMetrics( SM_DIGITIZER );

		if( value & NID_INTEGRATED_TOUCH ) result |= tdIntegratedTouch;
		if( value & NID_EXTERNAL_TOUCH ) result |= tdExternalTouch;
		if( value & NID_INTEGRATED_PEN ) result |= tdIntegratedPen;
		if( value & NID_EXTERNAL_PEN ) result |= tdExternalPen;
		if( value & NID_MULTI_INPUT ) result |= tdMultiInput;
		if( value & NID_READY ) result |= tdDigitizerReady;
	}
	int value = ::GetSystemMetrics( SM_MOUSEPRESENT );
	if( value ) {
		result |= tdMouse;
		value = ::GetSystemMetrics( SM_MOUSEWHEELPRESENT );
		if( value ) result |= tdMouseWheel;
	}
#endif
	result |= tdMouse;
	result |= tdMouseWheel;
	return result;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// System.onActivate and System.onDeactivate related
//---------------------------------------------------------------------------
static void TVPOnApplicationActivate(bool activate_or_deactivate);
//---------------------------------------------------------------------------
class tTVPOnApplicationActivateEvent : public tTVPBaseInputEvent
{
	static tTVPUniqueTagForInputEvent Tag;
	bool ActivateOrDeactivate; // true for activate; otherwise deactivate
public:
	tTVPOnApplicationActivateEvent(bool activate_or_deactivate) :
		tTVPBaseInputEvent(Application, Tag),
		ActivateOrDeactivate(activate_or_deactivate) {};
	void Deliver() const
	{ TVPOnApplicationActivate(ActivateOrDeactivate); }
};
tTVPUniqueTagForInputEvent tTVPOnApplicationActivateEvent              ::Tag;
//---------------------------------------------------------------------------
void TVPPostApplicationActivateEvent()
{
	TVPPostInputEvent(new tTVPOnApplicationActivateEvent(true), TVP_EPT_REMOVE_POST);
}
//---------------------------------------------------------------------------
void TVPPostApplicationDeactivateEvent()
{
	TVPPostInputEvent(new tTVPOnApplicationActivateEvent(false), TVP_EPT_REMOVE_POST);
}
//---------------------------------------------------------------------------
static void TVPOnApplicationActivate(bool activate_or_deactivate)
{
	// called by event system, to fire System.onActivate or
	// System.onDeactivate event
	if(!TVPSystemControlAlive) return;

	// check the state again (because the state may change during the event delivering).
	// but note that this implementation might fire activate events even in the application
	// is already activated (the same as deactivation).
	if(activate_or_deactivate != Application->GetActivating()) return;

	// fire the event
	TVPFireOnApplicationActivateEvent(activate_or_deactivate);
}
//---------------------------------------------------------------------------

#if 0
//---------------------------------------------------------------------------
static void TVPHeapDump()
{
	tjs_char buff[128];
	HANDLE heaps[100];
	DWORD c = ::GetProcessHeaps (100, heaps);
	TJS_sprintf( buff, 128, TJS_W("The process has %d heaps."), c );
	TVPAddLog( buff );

	const HANDLE default_heap = ::GetProcessHeap();
	const HANDLE crt_heap = (HANDLE)_get_heap_handle();
	for( unsigned int i = 0; i < c; i++ ) {
		ULONG heap_info = 0;
		SIZE_T ret_size = 0;
		bool isdefault = false;
		bool isCRT = false;
		if( ::HeapQueryInformation( heaps[i], HeapCompatibilityInformation, &heap_info, sizeof(heap_info), &ret_size) ) {
			tjs_char* type = NULL;
			switch( heap_info ) {
			case 0:
				type = TJS_W("standard");
				break;
			case 1:
				type = TJS_W("LAL");
				break;
			case 2:
				type = TJS_W("LFH");
				break;
			default:
				type = TJS_W("unknown");
				break;
			}
			if( heaps[i] == default_heap ) {
				isdefault = true;
			}
			if( heaps [i] == crt_heap ) {
				isCRT = true;
			}

			PROCESS_HEAP_ENTRY entry;
			memset( &entry, 0, sizeof (entry) );
			struct Info {
				int count;
				tjs_int64 total;
				tjs_int64 overhead;
				Info() : count(0), total(0), overhead(0) {}
			} use, uncommit, unused;
			while( ::HeapWalk( heaps[i], &entry) ) {
				if( entry.wFlags & PROCESS_HEAP_ENTRY_BUSY ) {
					use.count++;
					use.total += entry.cbData;
					use.overhead += entry.cbOverhead;
				} else if( entry.wFlags & PROCESS_HEAP_UNCOMMITTED_RANGE ) {
					uncommit.count++;
					uncommit.total += entry.cbData;
					uncommit.overhead += entry.cbOverhead;
				} else {
					unused.count++;
					unused.total += entry.cbData;
					unused.overhead += entry.cbOverhead;
				}
			}
			ttstr mes( TJS_W("#") );
			mes += ttstr((tjs_int)(i+1)) + TJS_W(" type: ") + type;
			if( isdefault ) mes += TJS_W(" [default]");
			if( isCRT ) mes += TJS_W(" [CRT]");
			TVPAddLog( mes );
			TJS_sprintf( buff, 128, L"  Allocated: %d, size: %lld, overhead: %lld", use.count, use.total, use.overhead );
			TVPAddLog( buff );
			TJS_sprintf( buff, 128, L"  Uncommitted: %d, size: %lld, overhead: %lld", uncommit.count, uncommit.total, uncommit.overhead );
			TVPAddLog( buff );
			TJS_sprintf( buff, 128, L"  Unused: %d, size: %lld, overhead: %lld", unused.count, unused.total, unused.overhead );
			TVPAddLog( buff );
		}
	}
}
//---------------------------------------------------------------------------
#endif
bool TVPAutoSaveBookMark = false;
extern void TVPDoSaveSystemVariables()
{
	try {
		// hack for save system variable
		iTJSDispatch2* global = TVPGetScriptDispatch();
		if (!global) return;
		tTJSVariant var;
		if (global->PropGet(0, TJS_W("kag"), nullptr, &var, global) == TJS_S_OK && var.Type() == tvtObject) {
			iTJSDispatch2* kag = var.AsObjectNoAddRef();
			if (kag->PropGet(0, TJS_W("saveSystemVariables"), nullptr, &var, kag) == TJS_S_OK) {
				iTJSDispatch2* fn = var.AsObjectNoAddRef();
				if (fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
					tTJSVariant *args = nullptr;
					fn->FuncCall(0, nullptr, nullptr, nullptr, 0, &args, kag);
				}
			}
			if(TVPAutoSaveBookMark
				&& kag->PropGet(0, TJS_W("saveBookMark"), nullptr, &var, kag) == TJS_S_OK
				&& var.Type() == tvtObject)
			{
				iTJSDispatch2* fn = var.AsObjectNoAddRef();
				if (fn->IsInstanceOf(0, 0, 0, TJS_W("Function"), fn)) {
					tTJSVariant num((tjs_int32)0);
					tTJSVariant *args = &num;
					fn->FuncCall(0, nullptr, nullptr, nullptr, 1, &args, kag);
				}
			}
		}
	} catch (...) {
		;
	}
}

//---------------------------------------------------------------------------
// TVPCreateNativeClass_System
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_System()
{
	tTJSNC_System *cls = new tTJSNC_System();


	// setup some platform-specific members
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/inform)
{
	// show simple message box
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr text = *param[0];

	ttstr caption;
	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		caption = *param[1];
	else
		caption = TJS_W("Information");

	if (numparams >= 3 && param[2]->Type() != tvtVoid) {
		if (param[2]->Type() == tvtObject) { // vector of button
			tTJSArrayNI* ni;
			param[2]->AsObjectNoAddRef()->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJSGetArrayClassID(), (iTJSNativeInstance**)&ni);
			std::vector<ttstr> vecButtons;
			vecButtons.reserve(ni->Items.size());
			for (const ttstr &label : ni->Items) {
				vecButtons.emplace_back(label);
			}
			int ret = TVPShowSimpleMessageBox(text, caption, vecButtons);
			if (result) result->operator =(ret);
		} else {
			int nButtons = param[2]->AsInteger();
			std::vector<ttstr> vecButtons;
			if (nButtons >= 1)
				vecButtons.emplace_back(TJS_W("OK"));
			if (nButtons >= 2)
				vecButtons.emplace_back(TJS_W("Cancel"));
			int ret = TVPShowSimpleMessageBox(text, caption, vecButtons);
			if (result) result->operator =(ret);
		}
		return TJS_S_OK;
	}

	TVPShowSimpleMessageBox(text, caption);

	if(result) result->Clear();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/inform)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTickCount)
{
	if(result)
	{
		TVPStartTickCount();

		*result = (tjs_int64) TVPGetTickCount();
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/getTickCount)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getKeyState)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_uint code = (tjs_int)*param[0];

	bool getcurrent = true;
	if(numparams >= 2) getcurrent = 0!=(tjs_int)*param[1];

	bool res = TVPGetAsyncKeyState(code, getcurrent);

	if(result) *result = (tjs_int)res;
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/getKeyState)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/shellExecute)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr target = *param[0];
	ttstr execparam;

	if(numparams >= 2) execparam = *param[1];

	bool res = TVPShellExecute(target, execparam);

	if(result) *result = (tjs_int)res;
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/shellExecute)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/system)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr target = *param[0];

	int ret = 0;// _wsystem(target.c_str());

	TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX); // this should clear all caches

	if(result) *result = (tjs_int)ret;
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/system)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/readRegValue)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	if(!result) return TJS_S_OK;

	ttstr key = *param[0];


	TVPReadRegValue(*result, key);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/readRegValue)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getArgument)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	if(!result) return TJS_S_OK;

	ttstr name = *param[0];

	bool res = TVPGetCommandLine(name.c_str(), result);

	if(!res) result->Clear();

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/getArgument)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setArgument)
{
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];
	ttstr value = *param[1];

	TVPSetCommandLine(name.c_str(), value);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/setArgument)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/createAppLock)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	if(!result) return TJS_S_OK;

	ttstr lockname = *param[0];

	bool res = TVPCreateAppLock(lockname);

	if(result) *result = (tjs_int)res;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/createAppLock)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/dumpHeap)
{
//	TVPHeapDump();
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/dumpHeap)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/nullpo)
{
	// force make a null-po
#ifdef _MSC_VER
	*(int *)0  = 0;
#else
	__builtin_trap();
#endif

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/nullpo)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/showVersion)
{
//	TVPShowVersionForm();
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/showVersion)
//---------------------------------------------------------------------------

//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(exePath)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetAppPath();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, exePath)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(personalPath)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetPersonalPath();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, personalPath)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(appDataPath)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetAppDataPath();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, appDataPath)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(dataPath)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPDataPath;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, dataPath)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(exeName)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		static ttstr exename(TVPNormalizeStorageName(ExePath()));
		*result = exename;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, exeName)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(savedGamesPath)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetSavedGamesPath();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, savedGamesPath)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(title)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		if(!TVPAppTitleInit)
		{
			TVPAppTitleInit = true;
			TVPAppTitle = Application->GetTitle();
		}
		*result = TVPAppTitle;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPAppTitle = *param;
		Application->SetTitle( TVPAppTitle.AsStdString() );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, title)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(screenWidth)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, screenWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(screenHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, screenHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(desktopLeft)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetDesktopLeft();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, desktopLeft)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(desktopTop)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetDesktopTop();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, desktopTop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(desktopWidth)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetDesktopWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, desktopWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(desktopHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = tTVPScreen::GetDesktopHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, desktopHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(touchDevice)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetSupportTouchDevice();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, touchDevice)
//----------------------------------------------------------------------


	return cls;

}
//---------------------------------------------------------------------------


