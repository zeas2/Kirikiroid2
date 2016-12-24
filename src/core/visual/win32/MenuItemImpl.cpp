//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "MenuItem" class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"


#include "EventIntf.h"
#include "MenuItemImpl.h"
#include "MsgIntf.h"
#include "WindowIntf.h"
#include "Application.h"
#include "tjsDictionary.h"
#include "vkdefine.h"
#include "ScriptMgnIntf.h"
#include <map>

static std::map<tTVInteger, iTJSDispatch2*> MENU_LIST;
static void AddMenuDispatch(tTVInteger hWnd, iTJSDispatch2* menu) {
	MENU_LIST.insert(std::map<tTVInteger, iTJSDispatch2*>::value_type(hWnd, menu));
}
iTJSDispatch2* TVPGetMenuDispatch(tTVInteger hWnd) {
	std::map<tTVInteger, iTJSDispatch2*>::iterator i = MENU_LIST.find(hWnd);
	if (i != MENU_LIST.end()) {
		return i->second;
	}
	return NULL;
}
static void DelMenuDispatch(tTVInteger hWnd) {
	MENU_LIST.erase(hWnd);
}
static bool _IsWindow(tTVInteger hWnd) {
	tjs_int count = TVPGetWindowCount();
	for (tjs_int i = 0; i < count; ++i) {
		if (TVPGetWindowListAt(i) == (tTJSNI_Window*)(hWnd))
			return true;
	}
	return false;
}

static void UpdateMenuList() {
	std::map<tTVInteger, iTJSDispatch2*>::iterator i = MENU_LIST.begin();
	for (; i != MENU_LIST.end();) {
		tTVInteger hWnd = i->first;
		bool exist = _IsWindow(hWnd);
		if (exist == false) {
			// 既になくなったWindow
			std::map<tTVInteger, iTJSDispatch2*>::iterator target = i;
			i++;
			iTJSDispatch2* menu = target->second;
			MENU_LIST.erase(target);
			menu->Release();
			//TVPDeleteAcceleratorKeyTable(hWnd);
		} else {
			i++;
		}
	}
}

class WindowMenuProperty : public tTJSDispatch {
	tjs_error TJS_INTF_METHOD PropGet(tjs_uint32 flag, const tjs_char * membername, tjs_uint32 *hint, tTJSVariant *result, iTJSDispatch2 *objthis) {
		tTJSVariant var;
		if (TJS_FAILED(objthis->PropGet(0, TJS_W("HWND"), NULL, &var, objthis))) {
			return TJS_E_INVALIDOBJECT;
		}
		tTVInteger hWnd = var.AsInteger();
		iTJSDispatch2* menu = TVPGetMenuDispatch(hWnd);
		if (menu == NULL) {
			UpdateMenuList();
			menu = TVPCreateMenuItemObject(objthis);
			AddMenuDispatch(hWnd, menu);
		}
		*result = tTJSVariant(menu, menu);
		return TJS_S_OK;
	}
	tjs_error TJS_INTF_METHOD PropSet(tjs_uint32 flag, const tjs_char *membername, tjs_uint32 *hint, const tTJSVariant *param, iTJSDispatch2 *objthis) {
		return TJS_E_ACCESSDENYED;
	}
} *gWindowMenuProperty;

//---------------------------------------------------------------------------
// tTJSNI_MenuItem
//---------------------------------------------------------------------------
tTJSNI_MenuItem::tTJSNI_MenuItem()
{
	IsChecked = false;
	IsAttched = true;
	IsEnabled = true;
	IsRadio = false;
	IsVisible = true;
	GroupIndex = 0;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSNI_MenuItem::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjs_obj)
{
	inherited::Construct(numparams, param, tjs_obj);

	// create or attach MenuItem object
	if(Window)
	{
		//MenuItem = Window->GetRootMenuItem();
		IsAttched = true;
	}
	else
	{
// 		MenuItem = new TMenuItem();
// 		MenuItem->OnClick = std::bind(&tTJSNI_MenuItem::MenuItemClick, this);
		IsAttched = false;
	}

	// fetch initial caption
	if(!Window && numparams >= 2)
	{
		Caption = *param[1];
		//MenuItem->setCaption (Caption);
	}

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_MenuItem::Invalidate()
{
	// invalidate inherited
	inherited::Invalidate();  // this sets Owner = NULL

	// delete VCL object
	//if (!IsAttched && MenuItem) delete MenuItem, MenuItem = NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::MenuItemClick()
{
	// VCL event handler
	// post to the event queue
	TVPPostInputEvent(new tTVPOnMenuItemClickInputEvent(this));
}
//---------------------------------------------------------------------------
bool tTJSNI_MenuItem::CanDeliverEvents() const
{
	// returns whether events can be delivered
	//if(!MenuItem) return false;
	bool enabled = true;
	
	const tTJSNI_MenuItem *item = this;
	while(item)
	{
		if (!item->GetEnabled())
		{
			enabled = false;
			break;
		}
		item = static_cast<const tTJSNI_MenuItem*>(item->GetParent());
	}
	return enabled;
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::Add(tTJSNI_MenuItem * item)
{
// 	if(MenuItem && item->MenuItem)
// 	{
// 		MenuItem->Add(item->MenuItem);
		AddChild(item);
	//}
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::Insert(tTJSNI_MenuItem *item, tjs_int index)
{
// 	if(MenuItem && item->MenuItem)
// 	{
// 		MenuItem->Insert(index, item->MenuItem);
	if (Children.Add(item, index))
	{
		ChildrenArrayValid = false;
		if (item->Owner) item->Owner->AddRef();
		item->Parent = this;
	}
		//AddChild(item);
	//}
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::Remove(tTJSNI_MenuItem *item)
{
// 	if(MenuItem && item->MenuItem)
// 	{
// 		int index = MenuItem->IndexOf(item->MenuItem);
// 		if(index == -1) TVPThrowExceptionMessage(TVPNotChildMenuItem);
// 
// 		MenuItem->Delete(index);
		RemoveChild(item);
	//}
}
//---------------------------------------------------------------------------
void *tTJSNI_MenuItem::GetMenuItemHandleForPlugin() const
{
	/*if(!MenuItem)*/ return NULL;
	//return MenuItem->getHandle();
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_MenuItem::GetIndex() const
{
	if (!Parent) return 0;
	//if(!MenuItem) return 0;
	//return MenuItem->getMenuIndex();
	tjs_int idx = Parent->Children.Find((tTJSNI_BaseMenuItem*)this);
	if (idx < 0) idx = 0;
	return idx;
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetIndex(tjs_int newIndex)
{
// 	if(!MenuItem) return;
// 	MenuItem->setMenuIndex (newIndex);
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetCaption(const ttstr & caption)
{
	//if(!MenuItem) return;
	Caption = caption;
// 	MenuItem->setAutoHotkeys (maManual);
// 	MenuItem->setCaption (caption);
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::GetCaption(ttstr & caption) const
{
	//if(!MenuItem) caption.Clear();
	caption = Caption;
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetChecked(bool b)
{
	//if(!MenuItem) return;
	//MenuItem->setChecked (b);
	if (b && IsRadio && Parent) {
		for (tTJSNI_BaseMenuItem *_item : Parent->Children) {
			tTJSNI_MenuItem *item = static_cast<tTJSNI_MenuItem*>(_item);
			if (item->IsRadio && item->GroupIndex == GroupIndex)
				item->IsChecked = false;
		}
	}
	IsChecked = b;
}
//---------------------------------------------------------------------------
bool tTJSNI_MenuItem::GetChecked() const
{
// 	if(!MenuItem) return false;
// 	return MenuItem->getChecked();
	return IsChecked;
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetEnabled(bool b)
{
	IsEnabled = b;
// 	if(!MenuItem) return;
// 	MenuItem->setEnabled (b);
}
//---------------------------------------------------------------------------
bool tTJSNI_MenuItem::GetEnabled() const
{
	return IsEnabled;
// 	if(!MenuItem) return false;
// 	return MenuItem->getEnabled();
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetGroup(tjs_int g)
{
	GroupIndex = g;
// 	if(!MenuItem) return;
// 	MenuItem->setGroupIndex ((BYTE)g);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_MenuItem::GetGroup() const
{
	return GroupIndex;
// 	if(!MenuItem) return 0;
// 	return MenuItem->getGroupIndex();
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetRadio(bool b)
{
	IsRadio = b;
// 	if(!MenuItem) return;
// 	MenuItem->setRadioItem (b);
}
//---------------------------------------------------------------------------
bool tTJSNI_MenuItem::GetRadio() const
{
	return IsRadio;
// 	if(!MenuItem) return false;
// 	return MenuItem->getRadioItem();
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetShortcut(const ttstr & shortcut)
{
	//if(!MenuItem) return;
//	MenuItem->setShortCut (TextToShortCut(shortcut.AsAnsiString()));
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::GetShortcut(ttstr & shortcut) const
{
	/*if(!MenuItem)*/ shortcut.Clear();
//	shortcut = ShortCutToText(MenuItem->getShortCut());
}
//---------------------------------------------------------------------------
void tTJSNI_MenuItem::SetVisible(bool b)
{
	IsVisible = b;
// 	if(!MenuItem) return;
// 	if(Window) Window->SetMenuBarVisible(b); else MenuItem->setVisible (b);
}
//---------------------------------------------------------------------------
bool tTJSNI_MenuItem::GetVisible() const
{
	return IsVisible;
// 	if(!MenuItem) return false;
// 	if(Window) return Window->GetMenuBarVisible(); else return MenuItem->getVisible();
}

void TVPShowPopMenu(tTJSNI_MenuItem* menu);

//---------------------------------------------------------------------------
tjs_int tTJSNI_MenuItem::TrackPopup(tjs_uint32 flags, tjs_int x, tjs_int y) const
{
	//if (!MenuItem) return 0;
	// TODO
	TVPShowPopMenu((tTJSNI_MenuItem*)this);
	return 1;
// 
// 	HWND  hWindow;
// 	if (GetRootMenuItem() && GetRootMenuItem()->GetWindow()) {
// 		hWindow = GetRootMenuItem()->GetWindow()->GetMenuOwnerWindowHandle();
// 	} else {
// 		return 0;
// 	}
// 	HMENU hMenuItem = GetMenuItemHandleForPlugin();
// 
// 	// we assume where that x and y are in client coordinates.
// 	// TrackPopupMenuEx requires screen coordinates, so here converts them.
// 	POINT scrPoint;	// screen
// 	scrPoint.x = x;
// 	scrPoint.y = y;
// 	BOOL rvScr = ::ClientToScreen(hWindow, &scrPoint);
// 	if (!rvScr)
// 	{
// 		// TODO
// 	}
// 
// 	BOOL rvPopup = TrackPopupMenuEx(hMenuItem, flags, scrPoint.x, scrPoint.y, hWindow, NULL);
// 	if (!rvPopup)
// 	{
// 		// TODO
// 		// should raise an exception when the API fails
// 	}
// 
// 	return rvPopup;
}


static iTJSDispatch2* textToKeycodeMap = nullptr;
static iTJSDispatch2* keycodeToTextList = nullptr;

static bool SetShortCutKeyCode(ttstr text, int key, bool force) {
	tTJSVariant vtext(text);
	tTJSVariant vkey(key);

	text.ToLowerCase();
	if (TJS_FAILED(textToKeycodeMap->PropSet(TJS_MEMBERENSURE, text.c_str(), nullptr, &vkey, textToKeycodeMap)))
		return false;
	if (force == false) {
		tTJSVariant var;
		keycodeToTextList->PropGetByNum(0, key, &var, keycodeToTextList);
		if (var.Type() == tvtString) return true;
	}
	return TJS_SUCCEEDED(keycodeToTextList->PropSetByNum(TJS_MEMBERENSURE, key, &vtext, keycodeToTextList));
}

static void CreateShortCutKeyCodeTable() {
	textToKeycodeMap = TJSCreateDictionaryObject();
	keycodeToTextList = TJSCreateArrayObject();
	if (textToKeycodeMap == nullptr || keycodeToTextList == nullptr) return;
#if 0
	tjs_char tempKeyText[32];
	for (int key = 8; key <= 255; key++) {
		int code = (::MapVirtualKey(key, 0) << 16) | (1 << 25);
		if (::GetKeyNameText(code, tempKeyText, 32) > 0) {
			ttstr text(tempKeyText);
			// special for NumPad key
			if (TJS_strnicmp(text.c_str(), TJS_W("Num "), 4) == 0) {
				bool numpad = (key >= VK_NUMPAD0 && key <= VK_DIVIDE);
				if (!numpad && ::GetKeyNameText(code | (1 << 24), tempKeyText, 32) > 0) {
					text = tempKeyText;
				}
			}
			SetShortCutKeyCode(text, key, true);
		}
	}
#endif

	// 吉里吉里２互換用ショートカット文字列
	SetShortCutKeyCode(TJS_W("BkSp"), VK_BACK, false);
	SetShortCutKeyCode(TJS_W("PgUp"), VK_PRIOR, false);
	SetShortCutKeyCode(TJS_W("PgDn"), VK_NEXT, false);
}

//---------------------------------------------------------------------------
// tTJSNC_MenuItem::CreateNativeInstance
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_MenuItem::CreateNativeInstance()
{
	return new tTJSNI_MenuItem();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPCreateNativeClass_MenuItem
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_MenuItem()
{
	tTJSNativeClass *cls = new tTJSNC_MenuItem();
	static tjs_uint32 TJS_NCM_CLASSID;
	TJS_NCM_CLASSID = tTJSNC_MenuItem::ClassID;

//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(HMENU)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		if (result) *result = (tTVInteger)(void*)_this->GetMenuItemHandleForPlugin();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, HMENU)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(textToKeycode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		if (result) *result = tTJSVariant(textToKeycodeMap, textToKeycodeMap);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, textToKeycode)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(keycodeToText)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
	if (result) *result = tTJSVariant(keycodeToTextList, keycodeToTextList);
	return TJS_S_OK;
}
	TJS_END_NATIVE_PROP_GETTER

		TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL_OUTER(cls, keycodeToText)
//---------------------------------------------------------------------------

	if (!textToKeycodeMap) {
		CreateShortCutKeyCodeTable();

		tTJSVariant val;

		iTJSDispatch2 * global = TVPGetScriptDispatch();

		{
			gWindowMenuProperty = new WindowMenuProperty();
			val = tTJSVariant(gWindowMenuProperty);
			gWindowMenuProperty->Release();
			tTJSVariant win;
			if (TJS_SUCCEEDED(global->PropGet(0, TJS_W("Window"), NULL, &win, global))) {
				iTJSDispatch2* obj = win.AsObjectNoAddRef();
				obj->PropSet(TJS_MEMBERENSURE, TJS_W("menu"), NULL, &val, obj);
				win.Clear();
			}
			val.Clear();

			//-----------------------------------------------------------------------
			iTJSDispatch2 * tjsclass = TVPCreateNativeClass_MenuItem();
			val = tTJSVariant(tjsclass);
			tjsclass->Release();
			global->PropSet(TJS_MEMBERENSURE, TJS_W("MenuItem"), NULL, &val, global);
			//-----------------------------------------------------------------------

		}

		global->Release();
	}

	return cls;
}
//---------------------------------------------------------------------------

