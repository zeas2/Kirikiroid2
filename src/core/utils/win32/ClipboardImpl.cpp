//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Clipboard Class interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
#include "MsgIntf.h"
#include "Exception.h"
#include "ClipboardIntf.h"

//---------------------------------------------------------------------------
// clipboard related functions
//---------------------------------------------------------------------------
bool TVPClipboardHasFormat(tTVPClipboardFormat format)
{
	switch(format) {
		case cbfText: {
		bool result = false;
#if 0
		if( ::OpenClipboard(0) ) {
			result = 0 != ::IsClipboardFormatAvailable(CF_TEXT);
			if( result == false ) {
				result = 0 != ::IsClipboardFormatAvailable(CF_UNICODETEXT);
			}
			::CloseClipboard();
		}
#endif
		return result; // ANSI text or UNICODE text
		}
	default:
		return false;
	}
}
//---------------------------------------------------------------------------
void TVPClipboardSetText(const ttstr & text)
{
#if 0
	if( ::OpenClipboard(0) ) {
		HGLOBAL ansihandle = NULL;
		HGLOBAL unicodehandle = NULL;
		try {
			// store ANSI string
			std::string ansistr = text.AsNarrowStdString();
			int ansistrlen = (int)((ansistr.length() + 1)*sizeof(char));
			ansihandle = ::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, ansistrlen);
			if( !ansihandle ) TVPThrowExceptionMessage( TVPFaildClipboardCopy );

			char *mem = (char*)::GlobalLock(ansihandle);
			if(mem) strncpy_s(mem, ansistrlen, ansistr.c_str(),ansistrlen);
			::GlobalUnlock(ansihandle);

			::SetClipboardData( CF_TEXT, ansihandle );
			ansihandle = NULL;

			// store UNICODE string
			unicodehandle = ::GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, (text.GetLen() + 1) * sizeof(tjs_char));
			if(!unicodehandle) TVPThrowExceptionMessage( TVPFaildClipboardCopy );;

			tjs_char *unimem = (tjs_char*)::GlobalLock(unicodehandle);
			if(unimem) TJS_strcpy(unimem, text.c_str());
			::GlobalUnlock(unicodehandle);

			::SetClipboardData( CF_UNICODETEXT, unicodehandle );
			unicodehandle = NULL;
		} catch(...) {
			if(ansihandle) ::GlobalFree(ansihandle);
			if(unicodehandle) ::GlobalFree(unicodehandle);
			::CloseClipboard();
			throw;
		}
		::CloseClipboard();
	}
#endif
}
//---------------------------------------------------------------------------
bool TVPClipboardGetText(ttstr & text)
{
#if 0
	if(!::OpenClipboard(NULL)) return false;

	bool result = false;
	try
	{
		// select CF_UNICODETEXT or CF_TEXT
		UINT formats[2] = { CF_UNICODETEXT, CF_TEXT};
		int format = ::GetPriorityClipboardFormat(formats, 2);

		if(format == CF_UNICODETEXT)
		{
			// try to read unicode text
			HGLOBAL hglb = (HGLOBAL)::GetClipboardData(CF_UNICODETEXT);
			if(hglb != NULL)
			{
				const tjs_char *p = (const tjs_char *)::GlobalLock(hglb);
				if(p)
				{
					try
					{
						text = ttstr(p);
						result = true;
					}
					catch(...)
					{
						::GlobalUnlock(hglb);
						throw;
					}
					::GlobalUnlock(hglb);
				}
			}
		}
		else if(format == CF_TEXT)
		{
			// try to read ansi text
			HGLOBAL hglb = (HGLOBAL)::GetClipboardData(CF_TEXT);
			if(hglb != NULL)
			{
				const char *p = (const char *)::GlobalLock(hglb);
				if(p)
				{
					try
					{
						text = ttstr(p);
						result = true;
					}
					catch(...)
					{
						::GlobalUnlock(hglb);
						throw;
					}
					::GlobalUnlock(hglb);
				}
			}
		}
	}
	catch(...)
	{
		::CloseClipboard();
		throw;
	}
	::CloseClipboard();
	return result;
#endif
	return false;
}
//---------------------------------------------------------------------------

