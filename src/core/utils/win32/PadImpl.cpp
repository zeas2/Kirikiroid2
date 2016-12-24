//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text Editor
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "PadImpl.h"
#include "PadIntf.h"
#if 0
#include "PadFormUnit.h"


//---------------------------------------------------------------------------
// tTJSNI_Pad : Pad Class C++ Native Instance
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_Pad::Construct(tjs_int numparams, tTJSVariant **param,
	iTJSDispatch2 * dsp)
{
	HRESULT hr = tTJSNI_BasePad::Construct(numparams, param, dsp);
	if(TJS_FAILED(hr)) return hr;

	Form = new TTVPPadForm(Application);
	Form->SetExecButtonEnabled(false);

	// コンストラクタが起動された→スクリプトエディタではない、と判断してOK？
// 保留にします。
//	SetUserCreationMode(true);

	return S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD
tTJSNI_Pad::Invalidate()
{
	if(Form) delete Form, Form = NULL;
	tTJSNI_BasePad::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::OpenFromStorage(const ttstr & name)
{
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SaveToStorage(const ttstr & name)
{
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetText(const ttstr &content)
{
	Form->SetLines(content);
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Pad::GetText() const
{
	return Form->GetLines();
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSNI_Pad::GetColor() const
{
	return Form->GetEditColor();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetColor(tjs_uint32 color)
{
	Form->SetEditColor(color);
}
//---------------------------------------------------------------------------
bool tTJSNI_Pad::GetVisible() const
{
	return Form->Visible;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetVisible(bool state)
{
	Form->Visible = state;
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Pad::GetFileName() const
{
	return Form->GetFileName();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFileName(const ttstr & name)
{
	Form->SetFileName(name);
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetTitle(const ttstr &title)
{
	Form->Caption = title.AsAnsiString();
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Pad::GetTitle() const
{
	return Form->Caption;
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSNI_Pad::GetFontColor() const
{
	return Form->GetFontColor();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFontColor(tjs_uint32 color)
{
	Form->SetFontColor(color);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFontHeight() const	// pixel
{
	return Form->GetFontHeight();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFontHeight(tjs_int t)
{
	Form->SetFontHeight(t);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFontSize() const	// point
{
	return Form->GetFontSize();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFontSize(tjs_int t)
{
	Form->SetFontSize(t);
}
//---------------------------------------------------------------------------
bool tTJSNI_Pad::ContainsFontStyle(tjs_int style) const
{
	return Form->ContainsFontStyle(style);
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::AddFontStyle(tjs_int style)
{
	Form->AddFontStyle(style);
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::RemoveFontStyle(tjs_int style)
{
	Form->RemoveFontStyle(style);
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Pad::GetFontName(void) const
{
	return Form->GetFontName();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFontName(const ttstr & name)
{
	return Form->SetFontName(name);
}
//---------------------------------------------------------------------------
bool tTJSNI_Pad::IsReadOnly(void) const
{
	return Form->GetReadOnly();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetReadOnly(bool ro)
{
	Form->SetReadOnly(ro);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetOpacity(void) const
{
	return Form->GetOpacity();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetOpacity(tjs_int opa)
{
	Form->SetOpacity(opa);
}
//---------------------------------------------------------------------------
bool tTJSNI_Pad::GetWordWrap(void) const
{
	return Form->GetWordWrap();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetWordWrap(bool ww)
{
	Form->SetWordWrap(ww);
}
//---------------------------------------------------------------------------
bool tTJSNI_Pad::GetStatusBarVisible(void) const
{
	return Form->GetStatusBarVisible();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetStatusBarVisible(bool vis)
{
	Form->SetStatusBarVisible(vis);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetScrollBarsVisible(void) const
{
	return Form->GetScrollBarsVisible();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetScrollBarsVisible(tjs_int vis)
{
	Form->SetScrollBarsVisible(vis);
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetStatusText(const ttstr &title)
{
	Form->SetStatusText(title);
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Pad::GetStatusText() const
{
	return Form->GetStatusText();
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetBorderStyle() const
{
	return Form->GetBorderStyle();
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetBorderStyle(tjs_int style)
{
	Form->SetBorderStyle(style);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFormHeight() const
{
	return Form->Height;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFormHeight(tjs_int value)
{
	Form->Height = value;
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFormWidth() const
{
	return Form->Width;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFormWidth(tjs_int value)
{
	Form->Width = value;
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFormTop() const
{
	return Form->Top;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFormTop(tjs_int value)
{
	Form->Top = value;
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Pad::GetFormLeft() const
{
	return Form->Left;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetFormLeft(tjs_int value)
{
	Form->Left = value;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
bool tTJSNI_Pad::GetUserCreationMode(void) const
{
	return UserCreationMode;
}
//---------------------------------------------------------------------------
void tTJSNI_Pad::SetUserCreationMode(bool user)
{
	UserCreationMode = user;
	if (user)
	{
		Form->ToolBar->Visible = false;
		Form->ToolBar->Enabled = false;
		Form->ExecuteButton->Visible = false;
		Form->ExecuteButton->Enabled = false;
		Form->StatusBar->Left = 0;

		Form->SetEditColor(clWindow);
		Form->SetFontColor(clWindowText);
		Form->StatusBar->Width = Form->Width;
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Pad
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Pad::CreateNativeInstance()
{
	return new tTJSNI_Pad();
}
//---------------------------------------------------------------------------

#endif

