//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Graphics Loader ( loads graphic format from storage )
//---------------------------------------------------------------------------

#ifndef GraphicsLoaderImplH
#define GraphicsLoaderImplH

#include "GraphicsLoaderIntf.h"
#if 0
//---------------------------------------------------------------------------
// tTVPSusiePlugin
//---------------------------------------------------------------------------
class tTVPSusiePlugin
{
protected:
	int (PASCAL * GetPluginInfo)(int infono, LPSTR buf,int buflen);
	int (PASCAL * IsSupported)(LPSTR filename, DWORD dw);
	int (PASCAL * GetPicture)(LPSTR buf, long len, unsigned int flag,
			  HANDLE *pHBInfo, HANDLE *pHBm,
			  FARPROC lpPrgressCallback, long lData);
	int (PASCAL * GetArchiveInfo)(LPSTR buf,long len,
			unsigned int flag, HLOCAL *lphInf);
	int (PASCAL * GetFile)(LPSTR src,long len, LPSTR dest,unsigned int flag,
				FARPROC prgressCallback, long lData);

	HINSTANCE ModuleInstance;

	std::vector<ttstr> Extensions;

	tTVPSusiePlugin(HINSTANCE inst, const char *api);
	virtual ~tTVPSusiePlugin();

	static int PASCAL ProgressCallback(int nNum,int nDenom,long lData) { return 0; }

public:
	const std::vector<ttstr> & GetExtensions() const { return Extensions; }

	HINSTANCE GetModuleInstance() const { return ModuleInstance; }
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Susie Plug-in management functions
// ( support of SPI for archive files is in StorageImpl.cpp )
//---------------------------------------------------------------------------
extern void TVPLoadPictureSPI(HINSTANCE inst, tTVPBMPAlphaType alphatype = batMulAlpha);
extern void TVPUnloadPictureSPI(HINSTANCE inst);
//---------------------------------------------------------------------------
#endif
#endif
