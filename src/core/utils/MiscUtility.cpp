
#include "tjsCommHead.h"
#include "CharacterSet.h"
#if 0
#ifdef _WIN32
#include <mmsystem.h>
#endif

//---------------------------------------------------------------------------
// TVPGetRoughTickCount
// 32bit値のtickカウントを得る
//---------------------------------------------------------------------------
tjs_uint32 TVPGetRoughTickCount32()
{
#ifdef _WIN32
	return timeGetTime();
#else
	#error Not implemented yet.
#endif
}
#endif
//---------------------------------------------------------------------------
bool TVPEncodeUTF8ToUTF16(ttstr &output, const std::string &source)
{
	tjs_int len = TVPUtf8ToWideCharString( source.c_str(), NULL );
	if(len == -1) return false;
	std::vector<tjs_char> outbuf( len+1, 0 );
	tjs_int ret = TVPUtf8ToWideCharString( source.c_str(), &(outbuf[0]));
	if( ret ) {
		outbuf[ret] = L'\0';
		output = &(outbuf[0]);
		return true;
	}
	return false;
}
