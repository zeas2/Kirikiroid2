//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text Editor
//---------------------------------------------------------------------------
#ifndef PadImplH
#define PadImplH
//---------------------------------------------------------------------------
#include "tjsNative.h"
#include "PadIntf.h"

class TTVPPadForm;
//---------------------------------------------------------------------------
// tTJSNI_Pad : Pad Class C++ Native Instance
//---------------------------------------------------------------------------
class tTJSNI_Pad : public tTJSNI_BasePad
{
	TTVPPadForm *Form;
public:
	virtual tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *dsp);
	virtual void TJS_INTF_METHOD Invalidate();

// methods
	virtual void OpenFromStorage(const ttstr & name);
	virtual void SaveToStorage(const ttstr & name);

// properties
	virtual tjs_uint32 GetColor() const;
	virtual void SetColor(tjs_uint32 color);
	virtual bool GetVisible() const;
	virtual void SetVisible(bool state);
	virtual ttstr GetFileName() const;
	virtual void SetFileName(const ttstr &name);
	virtual void SetText(const ttstr &content);
	virtual ttstr GetText() const;
	virtual void SetTitle(const ttstr &title);
	virtual ttstr GetTitle() const;

	virtual tjs_uint32 GetFontColor() const;
	virtual void SetFontColor(tjs_uint32 color);
	virtual tjs_int GetFontHeight() const;	// pixel
	virtual void SetFontHeight(tjs_int height);
	virtual tjs_int GetFontSize() const;	// point
	virtual void SetFontSize(tjs_int size);
	virtual bool ContainsFontStyle(tjs_int style) const;
	virtual void AddFontStyle(tjs_int style);
	virtual void RemoveFontStyle(tjs_int style);
	virtual ttstr GetFontName(void) const;
	virtual void SetFontName(const ttstr & name);
	virtual bool IsReadOnly(void) const;
	virtual void SetReadOnly(bool ro);
	virtual bool GetWordWrap(void) const;
	virtual void SetWordWrap(bool ww);
	virtual tjs_int GetOpacity(void) const;
	virtual void SetOpacity(tjs_int opa);
	virtual bool GetStatusBarVisible(void) const;
	virtual void SetStatusBarVisible(bool vis);
	virtual tjs_int GetScrollBarsVisible(void) const;
	virtual void SetScrollBarsVisible(tjs_int vis);
	virtual tjs_int GetBorderStyle(void) const;
	virtual void SetBorderStyle(tjs_int style);
	virtual ttstr GetStatusText() const;
	virtual void SetStatusText(const ttstr &title);

	// form position and size
	virtual tjs_int GetFormHeight(void) const;
	virtual tjs_int GetFormWidth(void) const;
	virtual tjs_int GetFormTop(void) const;
	virtual tjs_int GetFormLeft(void) const;
	virtual void SetFormHeight(tjs_int value);
	virtual void SetFormWidth(tjs_int value);
	virtual void SetFormTop(tjs_int value);
	virtual void SetFormLeft(tjs_int value);

	// 
	virtual bool GetUserCreationMode(void) const;
	virtual void SetUserCreationMode(bool mode);

protected:
	bool UserCreationMode; // true if this form was created by the userscript,
						// otherwise (when created by the system as "Script Editor") this will be false
	bool MultilineMode;

private:
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif

