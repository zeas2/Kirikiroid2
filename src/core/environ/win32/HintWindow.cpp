//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Half-transparent Hint Window Support
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "HintWindow.h"
#include "WindowFormUnit.h"
#include "tvpgl.h"

//---------------------------------------------------------------------------
// TVP_TB_SetGrayscalePalette
//---------------------------------------------------------------------------
static void TVP_TB_SetGrayscalePalette(Graphics::TBitmap *bmp)
{
	int psize;
	int icol;

	psize=256;

	icol=255/(psize-1);

	int i;
	int c=0;

#pragma pack(push,1)
	struct tagpal
	{
		WORD palversion;
		WORD numentries;
		PALETTEENTRY entry[256];
	} pal;
#pragma pack(pop)
	pal.palversion = 0x300;
	pal.numentries = psize;

	for(i=0;i<psize;i++)
	{
		if(c>=256) c=255;
		pal.entry[i].peRed=pal.entry[i].peGreen=pal.entry[i].peBlue=(BYTE)c;
		pal.entry[i].peFlags=0;
		c+=icol;
	}
	bmp->Palette = CreatePalette((LOGPALETTE*)&pal);
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// TVPHintWindowRegister
//---------------------------------------------------------------------------
static void TVPHintWindowRegister(void)
{
	HintWindowClass=__classid(TTVPHintWindow); // register hint window class
}
#ifdef __BORLANDC__
#pragma startup TVPHintWindowRegister
#endif
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// Most Recent Hint Window
//---------------------------------------------------------------------------
static TTVPHintWindow *TVPMostRecentHintWindow = NULL;
//---------------------------------------------------------------------------
HDWP TVPShowHintWindowTop(HDWP hdwp)
{
	if(TVPFullScreenedWindow != NULL && TVPMostRecentHintWindow)
		hdwp = TVPMostRecentHintWindow->ShowTop(hdwp);
	return hdwp;
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// TTVPHintWindow
//---------------------------------------------------------------------------
__fastcall TTVPHintWindow::TTVPHintWindow(TComponent *AOwner):
	THintWindow(AOwner)
{
	Init();
}
//---------------------------------------------------------------------------
__fastcall TTVPHintWindow::TTVPHintWindow(HWND ParentWindow):
	THintWindow(ParentWindow)
{
	Init();
}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::Init(void)
{
	BackBMP=new Graphics::TBitmap();
}
//---------------------------------------------------------------------------
__fastcall TTVPHintWindow::~TTVPHintWindow()
{
	TVPMostRecentHintWindow = NULL;
	delete BackBMP;
}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::ActivateHint(const TRect &ARect,const AnsiString
		AHint)
{
	SetWindowPos(Handle, HWND_TOPMOST, 0, 0, 0,
		0, SWP_HIDEWINDOW | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER);

	// change the window style
	SetWindowLong(Handle,GWL_STYLE,GetWindowLong(Handle,GWL_STYLE) & ~WS_BORDER);

	// adjust position
	TRect Rect=ARect;

	Caption=AHint;
	Rect.Bottom+=4;
	UpdateBoundsRect(Rect);

	if(Rect.Top + Height > Screen->DesktopHeight)
		Rect.Top = Screen->DesktopHeight - Height;
	if(Rect.Left + Width > Screen->DesktopWidth)
		Rect.Left = Screen->DesktopWidth - Width;
	if(Rect.Left < Screen->DesktopLeft)
		Rect.Left = Screen->DesktopLeft;
	if(Rect.Bottom < Screen->DesktopTop)
		Rect.Bottom = Screen->DesktopTop;

	// save bitmap which to be laid under the window
	BackBMP->Width=Rect.Right-Rect.Left;
	BackBMP->Height=Rect.Bottom-Rect.Top;
	BackBMP->PixelFormat=pf24bit;

	HDC dc = GetDC(0);

	TRect destrect;
	destrect.Left=0;
	destrect.Top=0;
	destrect.Right=BackBMP->Width;
	destrect.Bottom=BackBMP->Height;

	BitBlt(BackBMP->Canvas->Handle,
		0, 0, BackBMP->Width, BackBMP->Height,
		dc, Rect.Left, Rect.Top, SRCCOPY);

	ReleaseDC(0, dc);

	BackBMP->PixelFormat = pf32bit;

	// window reposition and showing
	SetWindowPos(Handle, HWND_TOPMOST, Rect.Left, Rect.Top, Width,
		Height, SWP_SHOWWINDOW | SWP_NOACTIVATE);

	TVPMostRecentHintWindow = this;
}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::ActivateHintData(const TRect &Rect,
	const AnsiString AHint,void *AData)
{
	ActivateHint(Rect,AHint);
}
//---------------------------------------------------------------------------
TRect __fastcall TTVPHintWindow::CalcHintRect(int MaxWidth,
	const AnsiString AHint, void *AData)
{
	RECT result;
	result.left=0;
	result.top=0;
	result.right=MaxWidth;
	result.bottom=0;
	DrawText(Canvas->Handle,AHint.c_str(), -1, &result,
		DT_CALCRECT | DT_LEFT | DT_WORDBREAK | DT_NOPREFIX);
	result.right+=6;
	result.bottom+=2;
	return TRect(result);
}
//---------------------------------------------------------------------------
bool __fastcall TTVPHintWindow::IsHintMsg(tagMSG &Msg)
{
	bool b = THintWindow::IsHintMsg(Msg);
	if(b)
	{
		// to preventing disappering hint window by keys
		if((Msg.message >= WM_KEYFIRST) && (Msg.message <= WM_KEYLAST))
			return false;
	}
	return b;
}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::Paint(void)
{
	// create bitmap to be piled with a mask bitmap
	Graphics::TBitmap *mask=new Graphics::TBitmap();
	Graphics::TBitmap *disp=new Graphics::TBitmap();
	Graphics::TBitmap *main=new Graphics::TBitmap();

	// set pixelformat and size
	mask->PixelFormat=pf8bit;
	TVP_TB_SetGrayscalePalette(mask);
	mask->Width=BackBMP->Width;
	mask->Height=BackBMP->Height;
	disp->PixelFormat=pf32bit;
	disp->Width=BackBMP->Width;
	disp->Height=BackBMP->Height;
	main->PixelFormat=pf32bit;
	main->Width=BackBMP->Width;
	main->Height=BackBMP->Height;

	// set up bmprect ...
	RECT bmprect;
	bmprect.left=0;
	bmprect.top=0;
	bmprect.right=BackBMP->Width;
	bmprect.bottom=BackBMP->Height;

	// fill mask with gray
	for(int i = 0; i<mask->Height; i++)
		memset(mask->ScanLine[i], 192, mask->Width);

	// fill main with clInfoBk
	main->Canvas->Brush->Color=clInfoBk;
	main->Canvas->Brush->Style=bsSolid;
	main->Canvas->FillRect(TRect(bmprect)); 

	// draw text
	mask->Canvas->Font=Canvas->Font;
	main->Canvas->Font=Canvas->Font;
	mask->Canvas->Brush->Style=bsClear;
	main->Canvas->Brush->Style=bsClear;
	main->Canvas->Font->Color=clInfoBk;
	RECT r=RECT(ClientRect);
	r.left+=3;
	r.right+=3;
	r.top+=3;
	r.bottom+=3;

	main->Canvas->Font->Color=clInfoBk;
	mask->Canvas->Font->Color=(TColor)0x00e0e0e0;
	{
		RECT r2=r;
		r2.left --;
		r2.top --;
		r2.right --;
		r2.bottom --;
		DrawText(mask->Canvas->Handle, Caption.c_str(), -1, &r2,
			DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	}

	{
		RECT r2=r;
		r2.left ++;
		r2.top --;
		r2.right ++;
		r2.bottom --;
		DrawText(mask->Canvas->Handle, Caption.c_str(), -1, &r2,
			DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	}

	{
		RECT r2=r;
		r2.left --;
		r2.top ++;
		r2.right --;
		r2.bottom ++;
		DrawText(mask->Canvas->Handle, Caption.c_str(), -1, &r2,
			DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	}

	{
		RECT r2=r;
		r2.left ++;
		r2.top ++;
		r2.right ++;
		r2.bottom ++;
		DrawText(mask->Canvas->Handle, Caption.c_str(), -1, &r2,
			DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	}


	main->Canvas->Font->Color=clInfoText;
	DrawText(main->Canvas->Handle, Caption.c_str(), -1, &r,
		DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);

	mask->Canvas->Font->Color=(TColor)0x00ffffff;

	DrawText(mask->Canvas->Handle, Caption.c_str(), -1, &r,
		DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);

	// draw frame
	mask->Canvas->Pen->Width=1;
	mask->Canvas->Pen->Color=(TColor)0x00ffffff;
	mask->Canvas->MoveTo(0,0);
	mask->Canvas->LineTo(mask->Width-1,0);
	mask->Canvas->LineTo(mask->Width-1,mask->Height-1);
	mask->Canvas->LineTo(0,mask->Height-1);
	mask->Canvas->LineTo(0,0);

	// bind
	for(int i = 0; i<mask->Height; i++)
		TVPBindMaskToMain((tjs_uint32*)main->ScanLine[i],
			(const tjs_uint8*)mask->ScanLine[i], mask->Width);

	// copy BackBMP to disp
	for(int i = 0; i<mask->Height; i++)
		memcpy(disp->ScanLine[i],
			BackBMP->ScanLine[i], BackBMP->Width * sizeof(tjs_uint32));

	// pile main to disp via pixel alpha blending
	for(int i = 0; i<mask->Height; i++)
		TVPAlphaBlend((tjs_uint32*)disp->ScanLine[i],
			(const tjs_uint32*)main->ScanLine[i], mask->Width);

	// draw to the canvas
	Canvas->Draw(0, 0, disp);

	// delete bitmaps
	delete disp;
	delete mask;
	delete main;

}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::WMNCPaint(TMessage &message)
{
	// nothing to do
}
//---------------------------------------------------------------------------
void __fastcall TTVPHintWindow::NCPaint(HDC dc)
{
}
//---------------------------------------------------------------------------
HDWP __fastcall TTVPHintWindow::ShowTop(HDWP hdwp)
{
	if(HandleAllocated() && Visible)
	{
		hdwp = DeferWindowPos(hdwp, Handle, HWND_TOPMOST, 0, 0, 0, 0,
			SWP_NOSENDCHANGING|SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOREPOSITION|
			SWP_NOSIZE|WM_SHOWWINDOW);
		Invalidate();
	}
	return hdwp;
}
//---------------------------------------------------------------------------

