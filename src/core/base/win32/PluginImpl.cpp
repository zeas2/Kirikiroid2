//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Plugins" class implementation / Service for plug-ins
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include <functional>
#include "ScriptMgnIntf.h"
#include "PluginImpl.h"
#include "StorageImpl.h"
#include "GraphicsLoaderImpl.h"

#include "MsgImpl.h"
#include "SysInitIntf.h"

#include "tjsHashSearch.h"
#include "EventIntf.h"
#include "TransIntf.h"
#include "tjsArray.h"
#include "tjsDictionary.h"
#include "DebugIntf.h"
#include "FuncStubs.h"
#include "tjs.h"

#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
	#include "oldwaveunpacker.h"
#endif

#pragma pack(push, 8)
	///  tvpsnd.h needs packing size of 8
	//#include "tvpsnd.h"
#pragma pack(pop)

#ifdef TVP_SUPPORT_KPI
	#include "kmp_pi.h"
#endif

#include "FilePathUtil.h"
#include "Application.h"
#include "SysInitImpl.h"
#include <set>
#ifdef _MSC_VER
#define strcasecmp _stricmp
#endif

#if 0
//---------------------------------------------------------------------------
// export table
//---------------------------------------------------------------------------
static tTJSHashTable<ttstr, void *> TVPExportFuncs;
static bool TVPExportFuncsInit = false;
void TVPAddExportFunction(const char *name, void *ptr)
{
	TVPExportFuncs.Add(name, ptr);
}
void TVPAddExportFunction(const tjs_char *name, void *ptr)
{
	TVPExportFuncs.Add(name, ptr);
}
static void TVPInitExportFuncs()
{
	if(TVPExportFuncsInit) return;
	TVPExportFuncsInit = true;


	// Export functions
	TVPExportFunctions();
}
//---------------------------------------------------------------------------
struct tTVPFunctionExporter : iTVPFunctionExporter
{
	bool TJS_INTF_METHOD QueryFunctions(const tjs_char **name, void **function,
		tjs_uint count);
	bool TJS_INTF_METHOD QueryFunctionsByNarrowString(const char **name,
		void **function, tjs_uint count);
} static TVPFunctionExporter;
//---------------------------------------------------------------------------
bool TJS_INTF_METHOD tTVPFunctionExporter::QueryFunctions(const tjs_char **name, void **function,
		tjs_uint count)
{
	// retrieve function table by given name table.
	// return false if any function is missing.
	bool ret = true;
	ttstr tname;
	for(tjs_uint i = 0; i<count; i++)
	{
		tname = name[i];
		void ** ptr = TVPExportFuncs.Find(tname);
		if(ptr)
			function[i] = *ptr;
		else
			function[i] = NULL, ret= false;
	}
	return ret;
}
//---------------------------------------------------------------------------
bool TJS_INTF_METHOD tTVPFunctionExporter::QueryFunctionsByNarrowString(
	const char **name, void **function, tjs_uint count)
{
	// retrieve function table by given name table.
	// return false if any function is missing.
	bool ret = true;
	ttstr tname;
	for(tjs_uint i = 0; i<count; i++)
	{
		tname = name[i];
		void ** ptr = TVPExportFuncs.Find(tname);
		if(ptr)
			function[i] = *ptr;
		else
			function[i] = NULL, ret= false;
	}
	return ret;
}
//---------------------------------------------------------------------------
extern "C" iTVPFunctionExporter * __stdcall TVPGetFunctionExporter()
{
	// for external applications
	TVPInitExportFuncs();
    return &TVPFunctionExporter;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TVPThrowPluginUnboundFunctionError(const char *funcname)
{
	TVPThrowExceptionMessage(TVPPluginUnboundFunctionError, funcname);
}
//---------------------------------------------------------------------------
void TVPThrowPluginUnboundFunctionError(const tjs_char *funcname)
{
	TVPThrowExceptionMessage(TVPPluginUnboundFunctionError, funcname);
}
//---------------------------------------------------------------------------
#endif


#if 0
//---------------------------------------------------------------------------
// implementation of IStorageProvider
//---------------------------------------------------------------------------
class tTVPStorageProvider : public ITSSStorageProvider
{
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, void **ppvObjOut)
	{
		if(!ppvObjOut) return E_INVALIDARG;

		*ppvObjOut = NULL;
		if(!memcmp(&iid, &IID_IUnknown, 16))
			*ppvObjOut = (IUnknown*)this;
		else if(!memcmp(&iid, &IID_ITSSStorageProvider, 16))
			*ppvObjOut = (ITSSStorageProvider*)this;

		if(*ppvObjOut)
		{
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }
	ULONG STDMETHODCALLTYPE Release(void) { return 1; }

	HRESULT __stdcall GetStreamForRead(
		LPWSTR url,
		IUnknown * *stream);

	HRESULT __stdcall GetStreamForWrite(
		LPWSTR url,
		IUnknown * *stream) { return E_NOTIMPL; }

	HRESULT __stdcall GetStreamForUpdate(
		LPWSTR url,
		IUnknown * *stream) { return E_NOTIMPL; }
};
//---------------------------------------------------------------------------
HRESULT __stdcall tTVPStorageProvider::GetStreamForRead(
		LPWSTR url,
		IUnknown * *stream)
{
	tTJSBinaryStream *stream0;
	try
	{
		stream0 = TVPCreateStream(url);
	}
	catch(...)
	{
		return E_FAIL;
	}

	IUnknown *istream = (IUnknown*)(IStream*)new tTVPIStreamAdapter(stream0);
	*stream = istream;

	return S_OK;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Plug-ins management
//---------------------------------------------------------------------------
struct tTVPPlugin
{
	ttstr Name;
	HINSTANCE Instance;

	tTVPPluginHolder *Holder;

	bool IsSusiePicturePlugin; // Susie picture plugins are managed in GraphicsLoaderImpl.cpp
	bool IsSusieArchivePlugin; // Susie archive plugins are managed in SusieArchive.cpp

	ITSSModule *TSSModule;

#ifdef TVP_SUPPORT_KPI
	KMPMODULE *KMPModule;
#endif

	tTVPV2LinkProc V2Link;
	tTVPV2UnlinkProc V2Unlink;


	tTVPGetModuleInstanceProc GetModuleInstance;
	tTVPGetModuleThreadModelProc GetModuleThreadModel;
	tTVPShowConfigWindowProc ShowConfigWindow;
	tTVPCanUnloadNowProc CanUnloadNow;
#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
	tTVPCreateWaveUnpackerProc CreateWaveUnpacker;
#endif

#ifdef TVP_SUPPORT_KPI
	pfnGetKMPModule GetKMPModule;
#endif

	std::vector<ttstr> SupportedExts;

	tTVPPlugin(const ttstr & name, ITSSStorageProvider *storageprovider);
	~tTVPPlugin();

	bool Uninit();
};
//---------------------------------------------------------------------------
tTVPPlugin::tTVPPlugin(const ttstr & name, ITSSStorageProvider *storageprovider)
{
	Name = name;

	Instance = NULL;
	Holder = NULL;
	IsSusiePicturePlugin = false;
	IsSusieArchivePlugin = false;
	TSSModule = NULL;

#ifdef TVP_SUPPORT_KPI
	KMPModule = NULL;
#endif

	V2Link = NULL;
	V2Unlink = NULL;

	GetModuleInstance = NULL;
	GetModuleThreadModel = NULL;
	ShowConfigWindow = NULL;
	CanUnloadNow = NULL;

#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
	CreateWaveUnpacker = NULL;
#endif

#ifdef TVP_SUPPORT_KPI
	GetKMPModule = NULL;
#endif

	// load DLL
	Holder = new tTVPPluginHolder(name);
	Instance = LoadLibrary(Holder->GetLocalName().AsStdString().c_str());
	if(!Instance)
	{
		delete Holder;
		TVPThrowExceptionMessage(TVPCannotLoadPlugin, name);
	}

	try
	{
		// retrieve each functions
		V2Link = (tTVPV2LinkProc)
			GetProcAddress(Instance, "V2Link");
		V2Unlink = (tTVPV2UnlinkProc)
			GetProcAddress(Instance, "V2Unlink");

		GetModuleInstance = (tTVPGetModuleInstanceProc)
			GetProcAddress(Instance, "GetModuleInstance");
		GetModuleThreadModel = (tTVPGetModuleThreadModelProc)
			GetProcAddress(Instance, "GetModuleThreadModel");
		ShowConfigWindow = (tTVPShowConfigWindowProc)
			GetProcAddress(Instance, "ShowConfigWindow");
		CanUnloadNow = (tTVPCanUnloadNowProc)
			GetProcAddress(Instance, "CanUnloadNow");
#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
		CreateWaveUnpacker = (tTVPCreateWaveUnpackerProc)
			GetProcAddress(Instance, "CreateWaveUnpacker");
#endif

#ifdef TVP_SUPPORT_KPI
		GetKMPModule = (pfnGetKMPModule)
			GetProcAddress(Instance, SZ_KMP_GETMODULE);
#endif

		// link
		if(V2Link)
		{
			V2Link(TVPGetFunctionExporter());
		}

		// retrieve ModuleInstance
		// Susie Plug-in check
		if(GetProcAddress(Instance, "GetPicture"))
		{
			IsSusiePicturePlugin = true;
			TVPLoadPictureSPI(Instance);
			return;
		}
		if(GetProcAddress(Instance, "GetFile"))
		{
			IsSusieArchivePlugin = true;
			TVPLoadArchiveSPI(Instance);
			return;
		}

		if(GetModuleInstance)
		{
			HRESULT hr = GetModuleInstance(&TSSModule, storageprovider,
				 NULL, Application->GetHandle());
			if(FAILED(hr) || TSSModule == NULL)
				TVPThrowExceptionMessage(TVPCannotLoadPlugin, name);

			// get supported extensions
			unsigned long index = 0;
			while(true)
			{
				wchar_t mediashortname[33];
				wchar_t buf[256];
				HRESULT hr = TSSModule->GetSupportExts(index,
					mediashortname, buf, 255);
				if(hr == S_OK)
					SupportedExts.push_back(ttstr(buf).AsLowerCase());
				else
					break;
				index ++;
			}
		}


#ifdef TVP_SUPPORT_KPI
		// retrieve KbMediaPlayer Plug-in module instance
		if(GetKMPModule)
		{
			KMPModule = GetKMPModule();
			if(KMPModule->dwVersion != 100)
				TVPThrowExceptionMessage(TVPCannotLoadPlugin, name +
					TJS_W(" (invalid version)"));
			if(!KMPModule->dwReentrant)
				TVPThrowExceptionMessage(TVPCannotLoadPlugin, name +
					TJS_W(" (is not re-entrant)"));

			if(KMPModule->Init) KMPModule->Init();
		}
#endif
	}
	catch(...)
	{
		FreeLibrary(Instance);
		delete Holder;
		throw;
	}
}
//---------------------------------------------------------------------------
tTVPPlugin::~tTVPPlugin()
{
}
//---------------------------------------------------------------------------
bool tTVPPlugin::Uninit()
{
	tTJS *tjs = TVPGetScriptEngine();
	if(tjs) tjs->DoGarbageCollection(); // to release unused objects

	if(V2Unlink)
	{
 		if(FAILED(V2Unlink())) return false;
	}
#ifdef TVP_SUPPORT_KPI
	if(KMPModule) if(KMPModule->Deinit) KMPModule->Deinit();
#endif
	if(TSSModule) TSSModule->Release();
	if(IsSusiePicturePlugin) TVPUnloadPictureSPI(Instance);
	if(IsSusieArchivePlugin) TVPUnloadArchiveSPI(Instance);
	FreeLibrary(Instance);
	delete Holder;
	return true;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
bool TVPPluginUnloadedAtSystemExit = false;
typedef std::vector<tTVPPlugin*> tTVPPluginVectorType;
struct tTVPPluginVectorStruc
{
	tTVPPluginVectorType Vector;
	tTVPStorageProvider StorageProvider;
} static TVPPluginVector;
static void TVPDestroyPluginVector(void)
{
	// state all plugins are to be released
	TVPPluginUnloadedAtSystemExit = true;

	// delete all objects
	tTVPPluginVectorType::iterator i;
	while(TVPPluginVector.Vector.size())
	{
		i = TVPPluginVector.Vector.end() - 1;
		try
		{
			(*i)->Uninit();
			delete *i;
		}
		catch(...)
		{
		}
		TVPPluginVector.Vector.pop_back();
	}
}
tTVPAtExit TVPDestroyPluginVectorAtExit
	(TVP_ATEXIT_PRI_RELEASE, TVPDestroyPluginVector);
#endif
//---------------------------------------------------------------------------
bool TVPLoadInternalPlugin(const ttstr &_name);
extern std::set<ttstr> TVPRegisteredPlugins;
static bool TVPPluginLoading = false;
void TVPLoadPlugin(const ttstr & name)
{
	bool success = TVPLoadInternalPlugin(name);
    return; // seal all plugins
#if 0
	// load plugin
	if(TVPPluginLoading)
		TVPThrowExceptionMessage(TVPCannnotLinkPluginWhilePluginLinking);
			// linking plugin while other plugin is linking, is prohibited
			// by data security reason.

	// check whether the same plugin was already loaded
	tTVPPluginVectorType::iterator i;
	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->Name == name) return;
	}

	tTVPPlugin * p;

	try
	{
		TVPPluginLoading = true;
		p = new tTVPPlugin(name, &TVPPluginVector.StorageProvider);
		TVPPluginLoading = false;
	}
	catch(...)
	{
		TVPPluginLoading = false;
		throw;
	}

	TVPPluginVector.Vector.push_back(p);
#endif
}
//---------------------------------------------------------------------------
bool TVPUnloadPlugin(const ttstr & name)
{
	// unload plugin
#if 0
	tTVPPluginVectorType::iterator i;
	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->Name == name)
		{
			if(!(*i)->Uninit()) return false;
			delete *i;
			TVPPluginVector.Vector.erase(i);
			return true;
		}
	}
	TVPThrowExceptionMessage(TVPNotLoadedPlugin, name);
	return false;
#endif
	return true;
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// plug-in autoload support
//---------------------------------------------------------------------------
struct tTVPFoundPlugin
{
	std::string Path;
	std::string Name;
	bool operator < (const tTVPFoundPlugin &rhs) const { return Name < rhs.Name; }
};
static tjs_int TVPAutoLoadPluginCount = 0;
static void TVPSearchPluginsAt(std::vector<tTVPFoundPlugin> &list, std::string folder)
{
	TVPListDir(folder, [&](const std::string &filename, int mask){
		if (mask & S_IFREG) {
			if (!strcasecmp(filename.c_str() + filename.length() - 4, ".tpm")) {
				tTVPFoundPlugin fp;
				fp.Path = folder;
				fp.Name = filename;
				list.emplace_back(fp);
			}
		}
	});
#if 0
	WIN32_FIND_DATA ffd;
	HANDLE handle = ::FindFirstFile((folder + L"*.tpm").c_str(), &ffd);
	if(handle != INVALID_HANDLE_VALUE)
	{
		BOOL cont;
		do
		{
			if(!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				tTVPFoundPlugin fp;
				fp.Path = folder;
				fp.Name = ffd.cFileName;
				list.push_back(fp);
			}
			cont = FindNextFile(handle, &ffd);
		} while(cont);
		FindClose(handle);
	}
#endif
}

void TVPLoadInternalPlugins();
void TVPLoadPluigins(void)
{
	TVPLoadInternalPlugins();
	// This function searches plugins which have an extension of ".tpm"
	// in the default path: 
	//    1. a folder which holds kirikiri executable
	//    2. "plugin" folder of it
	// Plugin load order is to be decided using its name;
	// aaa.tpm is to be loaded before aab.tpm (sorted by ASCII order)

	// search plugins from path: (exepath), (exepath)\system, (exepath)\plugin
	std::vector<tTVPFoundPlugin> list;

	std::string exepath = ExtractFileDir(TVPNativeProjectDir.AsNarrowStdString());

	TVPSearchPluginsAt(list, exepath);
	TVPSearchPluginsAt(list, exepath + "/system");
	TVPSearchPluginsAt(list, exepath + "/plugin");

	// sort by filename
	std::sort(list.begin(), list.end());

	// load each plugin
	TVPAutoLoadPluginCount = (tjs_int)list.size();
	for(std::vector<tTVPFoundPlugin>::iterator i = list.begin();
		i != list.end();
		i++)
	{
		TVPAddImportantLog(ttstr(TJS_W("(info) Loading ")) + ttstr(i->Name.c_str()));
		TVPLoadPlugin((i->Path + "/" + i->Name).c_str());
	}
}
//---------------------------------------------------------------------------
tjs_int TVPGetAutoLoadPluginCount() { return TVPAutoLoadPluginCount; }
//---------------------------------------------------------------------------






#if 0
//---------------------------------------------------------------------------
// interface to Wave decode plugins
//---------------------------------------------------------------------------
ITSSWaveDecoder * TVPSearchAvailTSSWaveDecoder(const ttstr & storage, const ttstr & extension)
{
	tTVPPluginVectorType::iterator i;
	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->TSSModule)
		{
			// check whether the plugin supports extension
			bool supported = false;
			std::vector<ttstr>::iterator ei;
			for(ei = (*i)->SupportedExts.begin(); ei != (*i)->SupportedExts.end(); ei++)
			{
				if(ei->GetLen() == 0) { supported = true; break; }
				if(extension == *ei) { supported = true; break; }
			}

			if(!supported) continue;

			// retrieve instance from (*i)->TSSModule
			IUnknown *intf = NULL;
			HRESULT hr = (*i)->TSSModule->GetMediaInstance(
				(wchar_t*)storage.c_str(), &intf);
			if(SUCCEEDED(hr))
			{
				try
				{
					// check  whether the instance has IID_ITSSWaveDecoder
					// interface.
					ITSSWaveDecoder * decoder;
					if(SUCCEEDED(intf->QueryInterface(IID_ITSSWaveDecoder,
						(void**) &decoder)))
					{
						intf->Release();
						return decoder; // OK
					}
				}
				catch(...)
				{
					intf->Release();
					throw;
				}
				intf->Release();
			}

		}
	}
	return NULL; // not found
}
//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
#ifdef TVP_SUPPORT_OLD_WAVEUNPACKER
IWaveUnpacker * TVPSearchAvailWaveUnpacker(const ttstr & storage, IStream **stream)
{
	tTVPPluginVectorType::iterator i;
	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->CreateWaveUnpacker) break;
	}
	if(i == TVPPluginVector.Vector.end()) return NULL; // KPI not found

	// retrieve IStream interface
	AnsiString ansiname = storage.AsAnsiString();

	tTJSBinaryStream *stream0 = NULL;
	long size;
	try
	{
		stream0 = TVPCreateStream(storage);
		size = (long)stream0->GetSize();
	}
	catch(...)
	{
		if(stream0) delete stream0;
		return NULL;
	}

	IStream *istream = new tTVPIStreamAdapter(stream0);

	try
	{

		for(i = TVPPluginVector.Vector.begin();
			i != TVPPluginVector.Vector.end(); i++)
		{
			if((*i)->CreateWaveUnpacker)
			{
				// call CreateWaveUnpacker to retrieve decoder instance
				IWaveUnpacker *out;
				HRESULT hr = (*i)->CreateWaveUnpacker(istream, size,
					ansiname.c_str(), &out);
				if(SUCCEEDED(hr))
				{
					*stream = istream;
					return out;
				}
			}
		}
	}
	catch(...)
	{
		istream->Release();
		return NULL;
	}
	istream->Release();
	return NULL; // not found
}
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifdef TVP_SUPPORT_KPI
void * TVPSearchAvailKMPWaveDecoder(const ttstr & storage, KMPMODULE ** module,
	SOUNDINFO * info)
{
	tTVPPluginVectorType::iterator i;
	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->KMPModule) break;
	}
	if(i == TVPPluginVector.Vector.end()) return NULL; // KPI not found

	AnsiString localname;

	if(TJS_strchr(storage.c_str(), TVPArchiveDelimiter)) return NULL;
		// in-archive storage is not supported

	try
	{
		ttstr ln(TVPSearchPlacedPath(storage));
		TVPGetLocalName(ln);
		localname  = ln.AsAnsiString();
	}
	catch(...)
	{
		return NULL;
	}

	AnsiString ext = TVPExtractStorageExt(storage).AsAnsiString();

	for(i = TVPPluginVector.Vector.begin();
		i != TVPPluginVector.Vector.end(); i++)
	{
		if((*i)->KMPModule)
		{
			// search over available extensions
			const char **module_ext = (*i)->KMPModule->ppszSupportExts;
			while(*module_ext)
			{
				if(!strcmpi(ext.c_str(), *module_ext)) break;
				module_ext ++;
			}
			if(!*module_ext) continue; // not found in this plug-in

			*module = (*i)->KMPModule;
			HKMP hkmp = (*i)->KMPModule->Open(localname.c_str(), info);
			if(hkmp)
				(*i)->KMPModule->SetPosition(hkmp, 0);
					// rewind; some plug-ins crash when the initial rewind is
					// not processed...
			return hkmp;
		}
	}
	return NULL; // not found
}
#endif
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// some service functions for plugin
//---------------------------------------------------------------------------
#include <zlib.h>
int ZLIB_uncompress(unsigned char *dest, unsigned long *destlen,
	const unsigned char *source, unsigned long sourcelen)
{
	return uncompress(dest, destlen, source, sourcelen);
}
//---------------------------------------------------------------------------
int ZLIB_compress(unsigned char *dest, unsigned long *destlen,
	const unsigned char *source, unsigned long sourcelen)
{
	return compress(dest, destlen, source, sourcelen);
}
//---------------------------------------------------------------------------
int ZLIB_compress2(unsigned char *dest, unsigned long *destlen,
	const unsigned char *source, unsigned long sourcelen, int level)
{
	return compress2(dest, destlen, source, sourcelen, level);
}
//---------------------------------------------------------------------------
#include "md5.h"
static char TVP_assert_md5_state_t_size[
	 (sizeof(TVP_md5_state_t) >= sizeof(md5_state_t))];
	// if this errors, sizeof(TVP_md5_state_t) is not equal to sizeof(md5_state_t).
	// sizeof(TVP_md5_state_t) must be equal to sizeof(md5_state_t).
//---------------------------------------------------------------------------
void TVP_md5_init(TVP_md5_state_t *pms)
{
	md5_init((md5_state_t*)pms);
}
//---------------------------------------------------------------------------
void TVP_md5_append(TVP_md5_state_t *pms, const tjs_uint8 *data, int nbytes)
{
	md5_append((md5_state_t*)pms, (const md5_byte_t*)data, nbytes);
}
//---------------------------------------------------------------------------
void TVP_md5_finish(TVP_md5_state_t *pms, tjs_uint8 *digest)
{
	md5_finish((md5_state_t*)pms, digest);
}
#if 0
//---------------------------------------------------------------------------
HWND TVPGetApplicationWindowHandle()
{
	return Application->GetHandle();
}
//---------------------------------------------------------------------------
void TVPProcessApplicationMessages()
{
	Application->ProcessMessages();
}
//---------------------------------------------------------------------------
void TVPHandleApplicationMessage()
{
	Application->HandleMessage();
}
#endif
//---------------------------------------------------------------------------
bool TVPRegisterGlobalObject(const tjs_char *name, iTJSDispatch2 * dsp)
{
	// register given object to global object
	tTJSVariant val(dsp);
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	tjs_error er;
	try
	{
		er = global->PropSet(TJS_MEMBERENSURE, name, NULL, &val, global);
	}
	catch(...)
	{
		global->Release();
		return false;
	}
	global->Release();
	return TJS_SUCCEEDED(er);
}
//---------------------------------------------------------------------------
bool TVPRemoveGlobalObject(const tjs_char *name)
{
	// remove registration of global object
	iTJSDispatch2 *global = TVPGetScriptDispatch();
	if(!global) return false;
	tjs_error er;
	try
	{
		er = global->DeleteMember(0, name, NULL, global);
	}
	catch(...)
	{
		global->Release();
		return false;
	}
	global->Release();
	return TJS_SUCCEEDED(er);
}
//---------------------------------------------------------------------------
void TVPDoTryBlock(
	tTVPTryBlockFunction tryblock,
	tTVPCatchBlockFunction catchblock,
	tTVPFinallyBlockFunction finallyblock,
	void *data)
{
	try
	{
		tryblock(data);
	}
	catch(const eTJS & e)
	{
		if(finallyblock) finallyblock(data);
		tTVPExceptionDesc desc;
		desc.type = TJS_W("eTJS");
		desc.message = e.GetMessage();
		if(catchblock(data, desc)) throw;
		return;
	}
	catch(...)
	{
		if(finallyblock) finallyblock(data);
		tTVPExceptionDesc desc;
		desc.type = TJS_W("unknown");
		if(catchblock(data, desc)) throw;
		return;
	}
	if(finallyblock) finallyblock(data);
}
//---------------------------------------------------------------------------


#if 0
//---------------------------------------------------------------------------
// TVPGetFileVersionOf
//---------------------------------------------------------------------------
bool TVPGetFileVersionOf(const wchar_t* module_filename, tjs_int &major, tjs_int &minor, tjs_int &release, tjs_int &build)
{
	// retrieve file version
	major = minor = release = build = 0;

	VS_FIXEDFILEINFO *FixedFileInfo;
	BYTE *VersionInfo;
	bool got = false;

	UINT dum;
	DWORD dum2;

	wchar_t* filename = new wchar_t[TJS_strlen(module_filename) + 1];
	try
	{
		TJS_strcpy(filename, module_filename);

		DWORD size = ::GetFileVersionInfoSize (filename, &dum2);
		if(size)
		{
			VersionInfo = new BYTE[size + 2];
			try
			{
				if(::GetFileVersionInfo(filename, 0, size, (void*)VersionInfo))
				{
					if(::VerQueryValue((void*)VersionInfo, L"\\", (void**)(&FixedFileInfo),
						&dum))
					{
						major   = FixedFileInfo->dwFileVersionMS >> 16;
						minor   = FixedFileInfo->dwFileVersionMS & 0xffff;
						release = FixedFileInfo->dwFileVersionLS >> 16;
						build   = FixedFileInfo->dwFileVersionLS & 0xffff;
						got = true;
					}
				}
			}
			catch(...)
			{
				delete [] VersionInfo;
				throw;
			}
			delete [] VersionInfo;
		}
	}
	catch(...)
	{
		delete [] filename;
		throw;
	}

	delete [] filename;

	return got;
}
//---------------------------------------------------------------------------
#endif


//---------------------------------------------------------------------------
// TVPCreateNativeClass_Plugins
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Plugins()
{
	tTJSNC_Plugins *cls = new tTJSNC_Plugins();


	// setup some platform-specific members
//---------------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/link)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];

	TVPLoadPlugin(name);

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/link)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/unlink)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	ttstr name = *param[0];

	bool res = TVPUnloadPlugin(name);

	if(result) *result = (tjs_int)res;

	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(/*object to register*/cls,
	/*func. name*/unlink)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(getList)
{
	iTJSDispatch2 * array = TJSCreateArrayObject();
	try
	{
		tjs_int idx = 0;
		for (ttstr name : TVPRegisteredPlugins) {
			tTJSVariant val(name);
			array->PropSetByNum(TJS_MEMBERENSURE, idx++, &val, array);
		}
#if 0
		tTVPPluginVectorType::iterator i;
		tjs_int idx = 0;
		for(i = TVPPluginVector.Vector.begin(); i != TVPPluginVector.Vector.end(); i++)
		{
			tTJSVariant val = (*i)->Name.c_str();
			array->PropSetByNum(TJS_MEMBERENSURE, idx++, &val, array);
		}
#endif
		if (result) *result = tTJSVariant(array, array);
	}
	catch(...)
	{
		array->Release();
		throw;
	}
	array->Release();
	return TJS_S_OK;
}
TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(cls, getList)
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
	return cls;
}
//---------------------------------------------------------------------------




