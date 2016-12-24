
#ifndef __TVP_SCREEN_H__
#define __TVP_SCREEN_H__
class tTVPScreen {
public:
	tTVPScreen();
	static int GetWidth();
	static int GetHeight();
//	static void GetDesktopRect( RECT& r );
	static int GetDesktopLeft();
	static int GetDesktopTop();
	static int GetDesktopWidth();
	static int GetDesktopHeight();
};

#endif // __TVP_SCREEN_H__
