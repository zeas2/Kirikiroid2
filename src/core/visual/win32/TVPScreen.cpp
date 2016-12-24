#include "cocos2d.h"
#include "tjsCommHead.h"

#include "TVPScreen.h"
#include "Application.h"

int tTVPScreen::GetWidth() {
	return 2048;
}
int tTVPScreen::GetHeight() {
	const cocos2d::Size &size = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
	int w = GetWidth();
	int h = w * (size.height / size.width);
	return w;
}

int tTVPScreen::GetDesktopLeft() {
	return 0;
}
int tTVPScreen::GetDesktopTop() {
	return 0;
}
int tTVPScreen::GetDesktopWidth() {
	return GetWidth();
}
int tTVPScreen::GetDesktopHeight() {
	return GetHeight();
}

