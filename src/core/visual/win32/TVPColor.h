
#ifndef __TVP_COLOR_H__
#define __TVP_COLOR_H__

enum {
	clScrollBar = 0x80000000,
	clBackground = 0x80000001,
	clActiveCaption = 0x80000002,
	clInactiveCaption = 0x80000003,
	clMenu = 0x80000004,
	clWindow = 0x80000005,
	clWindowFrame = 0x80000006,
	clMenuText = 0x80000007,
	clWindowText = 0x80000008,
	clCaptionText = 0x80000009,
	clActiveBorder = 0x8000000a,
	clInactiveBorder = 0x8000000b,
	clAppWorkSpace = 0x8000000c,
	clHighlight = 0x8000000d,
	clHighlightText = 0x8000000e,
	clBtnFace = 0x8000000f,
	clBtnShadow = 0x80000010,
	clGrayText = 0x80000011,
	clBtnText = 0x80000012,
	clInactiveCaptionText = 0x80000013,
	clBtnHighlight = 0x80000014,
	cl3DDkShadow = 0x80000015,
	cl3DLight = 0x80000016,
	clInfoText = 0x80000017,
	clInfoBk = 0x80000018,
	clNone = 0x1fffffff,
	clAdapt= 0x01ffffff,
	clPalIdx = 0x3000000,
	clAlphaMat = 0x4000000,
	clUnknown = 0x80000019,
	clHotLight = 0x8000001a,
	clGradientActiveCaption = 0x8000001b,
	clGradientInactiveCaption = 0x8000001c,
	clMenuLight = 0x8000001d,
	clMenuBar = 0x8000001e,
};

unsigned long ColorToRGB(unsigned int col);

#endif

