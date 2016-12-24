//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Half-transparent Hint Window Support
//---------------------------------------------------------------------------
#ifndef HintWindowH
#define HintWindowH
#include <vcl.h>
#include <controls.hpp>
//---------------------------------------------------------------------------
extern HDWP TVPShowHintWindowTop(HDWP hdwp);
//---------------------------------------------------------------------------
class TTVPHintWindow : public THintWindow
{
	void __fastcall Init();

protected:

	void __fastcall virtual WMNCPaint(TMessage &Message);
	void __fastcall NCPaint(HDC dc);

BEGIN_MESSAGE_MAP
	VCL_MESSAGE_HANDLER(WM_NCPAINT, TMessage, WMNCPaint)
	VCL_MESSAGE_HANDLER(WM_ERASEBKGND,TMessage,WMNCPaint)
END_MESSAGE_MAP(THintWindow)

	Graphics::TBitmap *BackBMP;

public:
#ifdef __BORLANDC__
	__fastcall virtual TTVPHintWindow(TComponent *AOwner);
	__fastcall virtual TTVPHintWindow(HWND ParentWindow);
#else
	TTVPHintWindow(TComponent *AOwner);
	TTVPHintWindow(HWND ParentWindow);
#endif
	__fastcall virtual ~TTVPHintWindow();

	virtual void __fastcall ActivateHint(const TRect &Rect,const AnsiString
		AHint);
	virtual void __fastcall ActivateHintData(const TRect &Rect,const AnsiString
		AHint,void *AData);
	virtual TRect __fastcall CalcHintRect(int MaxWidth,const AnsiString AHint,
		void *AData);
	virtual bool __fastcall IsHintMsg(tagMSG &Msg);

	void __fastcall Paint(void);

	HDWP __fastcall ShowTop(HDWP hdwp);
};
//---------------------------------------------------------------------------
#endif
