//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// File Selector dialog box
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

// #include <cderr.h>
// #include <commdlg.h>

#include "MsgIntf.h"
#include "StorageImpl.h"
#include "WindowImpl.h"
#include "SysInitIntf.h"
#include "DebugIntf.h"

#include "TVPScreen.h"

std::string TVPShowFileSelector(
    const std::string &title,
    const std::string &filename,
    std::string initdir,
    bool issave
    );

//---------------------------------------------------------------------------
// TVPSelectFile related
//---------------------------------------------------------------------------
#define TVP_OLD_OFN_STRUCT_SIZE 76
//---------------------------------------------------------------------------
#if 0
static tjs_int TVPLastScreenWidth = 0;
static tjs_int TVPLastScreenHeight = 0;
static tjs_int TVPLastOFNLeft = -30000;
static tjs_int TVPLastOFNTop = -30000;
static UINT_PTR APIENTRY TVPOFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam,
	LPARAM lParam)
{
	if(uiMsg == WM_INITDIALOG)
	{
		int left, top;
		HWND parent = GetParent(hdlg);
		if((TVPLastOFNLeft == -30000 && TVPLastOFNTop == -30000) ||
			TVPLastScreenWidth != tTVPScreen::GetWidth() || TVPLastScreenHeight != tTVPScreen::GetHeight() )
		{
			// center the window
			RECT rect;
			GetWindowRect(parent, &rect);
			left = ((tTVPScreen::GetWidth() - rect.right + rect.left) / 2);
			top = ((tTVPScreen::GetHeight() - rect.bottom + rect.top) / 3);
		}
		else
		{
			// set last position
			left = TVPLastOFNLeft;
			top = TVPLastOFNTop;
		}

		TVPLastScreenWidth = tTVPScreen::GetWidth();
		TVPLastScreenHeight = tTVPScreen::GetHeight();

		SetWindowPos(parent, 0,
			left,
			top,
			0, 0, SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
	}
	else if(uiMsg == WM_DESTROY ||
		(uiMsg == WM_NOTIFY && ((OFNOTIFY*)lParam)->hdr.code == CDN_FILEOK))
	{
		HWND parent = GetParent(hdlg);
		RECT rect;
		GetWindowRect(parent, &rect);
		TVPLastOFNLeft = rect.left;
		TVPLastOFNTop = rect.top;
	}
	return 0;
}
//---------------------------------------------------------------------------
static void TVPPushFilterPair(std::vector<std::wstring> &filters, std::wstring filter)
{
	std::wstring::size_type vpos = filter.find_first_of(L"|");
	if( vpos != std::wstring::npos )
	{
		std::wstring name = filter.substr(0, vpos);
		std::wstring wild = filter.c_str() + vpos+1;
		filters.push_back(name);
		filters.push_back(wild);
	}
	else
	{
		filters.push_back(filter);
		filters.push_back(filter);
	}
}
#endif
//---------------------------------------------------------------------------
bool TVPSelectFile(iTJSDispatch2 *params)
{
	// show open dialog box
	// NOTE: currently this only shows ANSI version of file open dialog.
	tTJSVariant val;
	std::string initialdir;
	std::string title;
	std::string defaultext;
#if 0
	wchar_t* filter = NULL;
	wchar_t* filename = NULL;

	BOOL result;

	try
	{
		// prepare OPENFILENAME structure

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = TVPGetModalWindowOwnerHandle();
		if( ofn.hwndOwner == INVALID_HANDLE_VALUE ) {
			ofn.hwndOwner = NULL;
		}
		ofn.hInstance = NULL;

		// set application window position to current window position
		

		// get filter
		ofn.lpstrFilter = NULL;

		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("filter"), 0,
			&val, params)))
		{
			std::vector<std::wstring> filterlist;
			if(val.Type() != tvtObject)
			{
				TVPPushFilterPair(filterlist, ttstr(val).AsStdString());
			}
			else
			{
				iTJSDispatch2 * array = val.AsObjectNoAddRef();
				tjs_int count;
				tTJSVariant tmp;
				if(TJS_SUCCEEDED(array->PropGet(TJS_MEMBERMUSTEXIST,
					TJS_W("count"), 0, &tmp, array)))
					count = tmp;
				else
					count = 0;

				for(tjs_int i = 0; i < count; i++)
				{
					if(TJS_SUCCEEDED(array->PropGetByNum(TJS_MEMBERMUSTEXIST,
						i, &tmp, array)))
					{
						TVPPushFilterPair(filterlist, ttstr(tmp).AsStdString());
					}
				}
			}

			// create filter buffer
			tjs_int bufsize = 2;
			for(std::vector<std::wstring>::iterator i = filterlist.begin(); i != filterlist.end(); i++)
			{
				bufsize += (tjs_int)(i->length() + 1);
			}

			filter = new wchar_t[bufsize];

			wchar_t* p = filter;
			for(std::vector<std::wstring>::iterator i = filterlist.begin(); i != filterlist.end(); i++)
			{
				TJS_strcpy(p, i->c_str());
				p += i->length() + 1;
			}
			*(p++) = 0;
			*(p++) = 0;

			ofn.lpstrFilter = filter;
		}

		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter = 0;

		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("filterIndex"), 0, &val, params)))
			ofn.nFilterIndex = (tjs_int)val;
		else
			ofn.nFilterIndex = 0;

		// filenames
		filename = new wchar_t[MAX_PATH + 1];
 		filename[0] = 0;

		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("name"), 0, &val, params)))
		{
			ttstr lname(val);
			if(!lname.IsEmpty())
			{
				lname = TVPNormalizeStorageName(lname);
				TVPGetLocalName(lname);
				std::wstring name = lname.AsStdString();
				TJS_strncpy(filename, name.c_str(), MAX_PATH);
				filename[MAX_PATH] = 0;
			}
		}

		ofn.lpstrFile = filename;
		ofn.nMaxFile = MAX_PATH + 1;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;

		// initial dir
		ofn.lpstrInitialDir = NULL;
		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("initialDir"), 0, &val, params)))
		{
			ttstr lname(val);
			if(!lname.IsEmpty())
			{
				lname = TVPNormalizeStorageName(lname);
				TVPGetLocalName(lname);
				initialdir = lname.AsStdString();
				ofn.lpstrInitialDir = initialdir.c_str();
			}
		}
	
		// title
		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("title"), 0, &val, params)))
		{
			title = ttstr(val).AsStdString();
			ofn.lpstrTitle = title.c_str();
		}
		else
		{
			ofn.lpstrTitle = NULL;
		}

		// flags
		bool issave = false;
		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("save"), 0, &val, params)))
			issave = val.operator bool();

		ofn.Flags = OFN_ENABLEHOOK|OFN_EXPLORER|OFN_NOCHANGEDIR|
			OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_ENABLESIZING;


		if(!issave)
			ofn.Flags |= OFN_FILEMUSTEXIST;
		else
			ofn.Flags |= OFN_OVERWRITEPROMPT;

		// default extension
		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("defaultExt"), 0, &val, params)))
		{
			defaultext = ttstr(val).AsStdString();
			ofn.lpstrDefExt = defaultext.c_str();
		}
		else
		{
			ofn.lpstrDefExt = NULL;
		}

		// hook proc
		ofn.lpfnHook = TVPOFNHookProc;

		// show dialog box
		if(!issave)
			result = GetOpenFileName(&ofn);
		else
			result = GetSaveFileName(&ofn);


		if(!result && CommDlgExtendedError() == CDERR_STRUCTSIZE)
		{
			// for old windows
			// set lStructSize to old Windows' structure size
			ofn.lStructSize = TVP_OLD_OFN_STRUCT_SIZE;
			if(!issave)
				result = GetOpenFileName(&ofn);
			else
				result = GetSaveFileName(&ofn);
		}

		if(result)
		{
			// returns some informations

			// filter index
			val = (tjs_int)ofn.nFilterIndex;
			params->PropSet(TJS_MEMBERENSURE, TJS_W("filterIndex"), 0, &val, params);

			// file name
			val = TVPNormalizeStorageName(ttstr(filename));
			params->PropSet(TJS_MEMBERENSURE, TJS_W("name"), 0, &val, params);
		}

	}
	catch(...)
	{
		if(filter) delete [] filter;
		if(filename) delete [] filename;
		throw;
	}

	delete [] filter;
	delete [] filename;

	return 0!=result;
#endif

    std::string filename;
	std::string result;

	// get filter
	if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("filter"), 0,
		&val, params)))
	{
	}

// 		if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("filterIndex"), 0,
// 			&val, params)))
// 			ofn.nFilterIndex = (tjs_int)val;
// 		else
// 			ofn.nFilterIndex = 0;

	// initial dir
	if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("initialDir"), 0,
		&val, params)))
	{
		ttstr lname(val);
		if (!lname.IsEmpty()){
			TVPGetLocalName(lname);
			initialdir = tTJSNarrowStringHolder(lname.c_str());
		}
	}
        
	// default extension
	if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("defaultExt"), 0,
		&val, params)))
	{
		defaultext = tTJSNarrowStringHolder(val.AsStringNoAddRef()->operator const tjs_char*());
	}

	// filenames
	if (TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("name"), 0,
		&val, params)))
	{
		ttstr lname(val);
		if (!lname.IsEmpty())
		{
			if (lname.IndexOf('/') >= 0) {
				lname = TVPNormalizeStorageName(lname);
				TVPGetLocalName(lname);
				ttstr path = TVPExtractStoragePath(lname);
				ttstr name = TVPExtractStorageName(lname);
				lname = name;
				initialdir = path.AsStdString();
			} else {
			}

			if (!defaultext.empty() && TVPExtractStorageExt(lname).IsEmpty()) {
				if (defaultext[0] != '.')
					lname += TJS_W(".");
				lname += defaultext.c_str();
			}
			filename = tTJSNarrowStringHolder(lname.c_str());
		}
	}
	
	// title
	if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("title"), 0,
		&val, params)))
	{
        title = tTJSNarrowStringHolder(val.AsStringNoAddRef()->operator const tjs_char*());
	}

	// flags
	bool issave = false;
	if(TJS_SUCCEEDED(params->PropGet(TJS_MEMBERMUSTEXIST, TJS_W("save"), 0,
		&val, params)))
		issave = val.operator bool();

	// show dialog box
    result = TVPShowFileSelector(title, filename, initialdir, issave);

	if(!result.empty())
	{
		// returns some informations

		// filter index
		val = (tjs_int)0;
		params->PropSet(TJS_MEMBERENSURE, TJS_W("filterIndex"), 0,
			&val, params);

		// file name
		ttstr tresult = TVPNormalizeStorageName(ttstr(result.c_str()));
		val = tresult;
		params->PropSet(TJS_MEMBERENSURE, TJS_W("name"), 0,
			&val, params);
        return true;
	}

	return false;
}
//---------------------------------------------------------------------------






