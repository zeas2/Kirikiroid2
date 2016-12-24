
#include "tjsCommHead.h"

#include "tjsError.h"
#include "Application.h"
#include "DebugIntf.h"
#include "WindowsUtil.h"

void TVPThrowWindowsErrorException() {
	ttstr mes;
	LPVOID lpMsgBuf;
	::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, ::GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL );
	mes = ttstr( (LPCWSTR)lpMsgBuf );
	::LocalFree(lpMsgBuf);
	throw new TJS::eTJSError( mes );
}

void TVPOutputWindowsErrorToDebugMessage() {
	LPVOID lpMsgBuf;
	::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL );
	::OutputDebugString( (LPCWSTR)lpMsgBuf );
	::LocalFree(lpMsgBuf);
}

void TVPOutputWindowsErrorToConsole( const char* file, int line ) {
	LPVOID lpMsgBuf;
	DWORD len = ::FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL );
	if( len > 0 ) {
#ifdef _DEBUG
		ttstr str(ttstr(L"(error) Windows Error : ") + ttstr(L"file : ") + ttstr(file) + ttstr(L", line : ") + ttstr(line) + ttstr(L", message : ") + ttstr((LPCWSTR)lpMsgBuf));
#else
		ttstr str(ttstr(L"(error) Windows Error : ") + ttstr((LPCWSTR)lpMsgBuf));
#endif
		TVPAddImportantLog( str );
	}
	::LocalFree(lpMsgBuf);
}

