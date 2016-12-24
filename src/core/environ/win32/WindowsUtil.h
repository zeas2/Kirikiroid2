
#ifndef __WINDOWS_ERROR_EXCEPTION_H__
#define __WINDOWS_ERROR_EXCEPTION_H__

extern void TVPThrowWindowsErrorException();
extern void TVPOutputWindowsErrorToDebugMessage();
extern void TVPOutputWindowsErrorToConsole( const char* file=NULL, int line=0 );

#define TVP_WINDOWS_ERROR_LOG TVPOutputWindowsErrorToConsole( __FILE__, __LINE__ );

#endif	// __WINDOWS_ERROR_EXCEPTION_H__
