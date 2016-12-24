//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "MenuItem" class implementation
//---------------------------------------------------------------------------
#ifndef MenuItemImplH
#define MenuItemImplH

//#include <Menus.hpp>
#include "MenuItemIntf.h"
//#include "Menus.hpp"

//---------------------------------------------------------------------------
// tTJSNI_MenuItem : MenuItem Native Instance
//---------------------------------------------------------------------------
class tTJSNI_MenuItem : public tTJSNI_BaseMenuItem
{
	typedef tTJSNI_BaseMenuItem inherited;

	//TMenuItem* MenuItem;

	ttstr Caption;
	ttstr Shortcut;
	bool IsAttched;
	bool IsChecked;
	bool IsEnabled;
	bool IsRadio;
	bool IsVisible;
	
	tjs_int GroupIndex;

public:
	tTJSNI_MenuItem();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

private:
	void MenuItemClick();

protected:
	bool CanDeliverEvents() const;
		// tTJSNI_BaseMenuItem::CanDeliverEvents override


public:
	void Add(tTJSNI_MenuItem * item);
	void Insert(tTJSNI_MenuItem *item, tjs_int index);
	void Remove(tTJSNI_MenuItem *item);

	void SetCaption(const ttstr & caption);
	void GetCaption(ttstr & caption) const;

	void SetChecked(bool b);
	bool GetChecked() const;

	void SetEnabled(bool b);
	bool GetEnabled() const;

	void SetGroup(tjs_int g);
	tjs_int GetGroup() const;

	void SetRadio(bool b);
	bool GetRadio() const;

	void SetShortcut(const ttstr & shortcut);
	void GetShortcut(ttstr & shortcut) const;

	void SetVisible(bool b);
	bool GetVisible() const;

	tjs_int GetIndex() const;
	void SetIndex(tjs_int newIndex);

	tjs_int TrackPopup(tjs_uint32 flags, tjs_int x, tjs_int y) const;

	const ObjectVector<tTJSNI_BaseMenuItem>& GetChildren() const { return Children; }

//-- interface to plugin
	/*HMENU*/void* GetMenuItemHandleForPlugin() const;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
