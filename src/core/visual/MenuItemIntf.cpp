//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "MenuItem" class interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "MsgIntf.h"
#include "MenuItemIntf.h"
#include "WindowIntf.h"
#include "EventIntf.h"
#include "tjsArray.h"
#include "SystemImpl.h"


//---------------------------------------------------------------------------
// Input Events
//---------------------------------------------------------------------------
// For input event tag
tTVPUniqueTagForInputEvent tTVPOnMenuItemClickInputEvent      ::Tag;
//---------------------------------------------------------------------------

static const tjs_char* TVPSpecifyMenuItem = TJS_W("Please specity MenuItem class object.");


//---------------------------------------------------------------------------
// tTJSNI_BaseMenuItem
//---------------------------------------------------------------------------
tTJSNI_BaseMenuItem::tTJSNI_BaseMenuItem()
{
	Owner = NULL;
	Window = NULL;
	Parent = NULL;
	ChildrenArrayValid = false;
	ChildrenArray = NULL;
	ArrayClearMethod = NULL;

	ActionOwner.Object = ActionOwner.ObjThis = NULL;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
		tTJSNI_BaseMenuItem::Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj)
{
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	ActionOwner = param[0]->AsObjectClosure();
	Owner = tjs_obj;

	if(numparams >= 2)
	{
		if(param[1]->Type() == tvtObject)
		{
			// is this Window instance ?
			tTJSVariantClosure clo = param[1]->AsObjectClosureNoAddRef();
			if(clo.Object == NULL) TVPThrowExceptionMessage(TVPSpecifyWindow);
			tTJSNI_Window *win = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Window::ClassID, (iTJSNativeInstance**)&win)))
				TVPThrowExceptionMessage(TVPSpecifyWindow);
			Window = win;
		}
	}

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseMenuItem::Invalidate()
{
	TVPCancelSourceEvents(Owner);
	TVPCancelInputEvents(this);

	{ // locked
		tTJSSpinLockHolder holder(Children.Lock);
		tjs_int count = Children.size();
		for(tjs_int i = 0; i < count; i++)
		{
			tTJSNI_BaseMenuItem *item = Children.at(i);
			if(!item) continue;

			if(item->Owner)
			{
				item->Owner->Invalidate(0, NULL, NULL, item->Owner);
				item->Owner->Release();
			}
		}
		Children.clear();
	} // locked

//	Owner = NULL;
	Window = NULL;
	Parent = NULL;

	if(ChildrenArray) ChildrenArray->Release(), ChildrenArray = NULL;
	if(ArrayClearMethod) ArrayClearMethod->Release(), ArrayClearMethod = NULL;

	ActionOwner.Release();
	ActionOwner.ObjThis = ActionOwner.Object = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
tTJSNI_MenuItem * tTJSNI_BaseMenuItem::CastFromVariant(const tTJSVariant & from)
{
	if(from.Type() == tvtObject)
	{
		// is this Window instance ?
		tTJSVariantClosure clo = from.AsObjectClosureNoAddRef();
		if(clo.Object == NULL) TVPThrowExceptionMessage(TVPSpecifyMenuItem);
		tTJSNI_MenuItem *menuitem = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_MenuItem::ClassID, (iTJSNativeInstance**)&menuitem)))
			TVPThrowExceptionMessage(TVPSpecifyMenuItem);
		return menuitem;
	}
	TVPThrowExceptionMessage(TVPSpecifyMenuItem);
	return NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseMenuItem::AddChild(tTJSNI_BaseMenuItem *item)
{
	if(Children.Add(item))
	{
		ChildrenArrayValid = false;
		if(item->Owner) item->Owner->AddRef();
		item->Parent = this;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseMenuItem::RemoveChild(tTJSNI_BaseMenuItem *item)
{
	if(Children.Remove(item))
	{
		ChildrenArrayValid = false;
		if(item->Owner) item->Owner->Release();
		item->Parent = NULL;
	}
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseMenuItem::GetChildrenArrayNoAddRef()
{
	if(!ChildrenArray)
	{
		// create an Array object
		iTJSDispatch2 * classobj;
		ChildrenArray = TJSCreateArrayObject(&classobj);
		try
		{
			tTJSVariant val;
			tjs_error er;
			er = classobj->PropGet(0, TJS_W("clear"), NULL, &val, classobj);
				// retrieve clear method
			if(TJS_FAILED(er)) TVPThrowInternalError;
			ArrayClearMethod = val.AsObject();
		}
		catch(...)
		{
			ChildrenArray->Release();
			ChildrenArray = NULL;
			classobj->Release();
			throw;
		}
		classobj->Release();
	}

	if(!ChildrenArrayValid)
	{
		// re-create children list
		ArrayClearMethod->FuncCall(0, NULL, NULL, NULL, 0, NULL, ChildrenArray);
			// clear array

		{ // locked
			tTJSSpinLockHolder holder(Children.Lock);
			tjs_int count = Children.size();
			tjs_int itemcount = 0;
			for(tjs_int i = 0; i < count; i++)
			{
				tTJSNI_BaseMenuItem *item = Children.at(i);
				if(!item) continue;

				iTJSDispatch2 * dsp = item->Owner;
				tTJSVariant val(dsp, dsp);
				ChildrenArray->PropSetByNum(TJS_MEMBERENSURE, itemcount, &val,
					ChildrenArray);
				itemcount++;
			}
		} // locked

		ChildrenArrayValid = true;
	}

	return ChildrenArray;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseMenuItem::OnClick(void)
{
	// fire onClick event
	if(!CanDeliverEvents()) return;

	// also check window
	tTJSNI_BaseMenuItem *item = this;
	while(!item->Window)
	{
		if(!item->Parent) break;
		item = item->Parent;
	}
	if(!item->Window) return;
	if(!item->Window->CanDeliverEvents()) return;

	// fire event
	static ttstr eventname(TJS_W("onClick"));
	TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	TVPDoSaveSystemVariables();
}
//---------------------------------------------------------------------------
tTJSNI_BaseMenuItem * tTJSNI_BaseMenuItem::GetRootMenuItem() const
{
	tTJSNI_BaseMenuItem * current = const_cast<tTJSNI_BaseMenuItem*>(this);
	tTJSNI_BaseMenuItem * parent = current->GetParent();
	while (parent)
	{
		current = parent;
		parent = current->GetParent();
	}
	return current;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNC_MenuItem
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_MenuItem::ClassID = -1;
tTJSNC_MenuItem::tTJSNC_MenuItem() : inherited(TJS_W("MenuItem"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(MenuItem) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_MenuItem,
	/*TJS class name*/MenuItem)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/MenuItem)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/add)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
 	tTJSNI_MenuItem * item = tTJSNI_BaseMenuItem::CastFromVariant(*param[0]);
 	_this->Add(item);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/add)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/insert)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
 	tTJSNI_MenuItem * item = tTJSNI_BaseMenuItem::CastFromVariant(*param[0]);
 	tjs_int index = *param[1];
 	_this->Insert(item, index);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/insert)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/remove)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
 	tTJSNI_MenuItem * item = tTJSNI_BaseMenuItem::CastFromVariant(*param[0]);
 	_this->Remove(item);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/remove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/popup) // not trackPopup
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	tjs_uint32 flags = (tTVInteger)*param[0];
	tjs_int x = *param[1];
	tjs_int y = *param[2];
	tjs_int rv = _this->TrackPopup(flags, x, y);
	if (result) *result = rv;
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/popup) // not trackPopup
//---------------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MenuItem);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onClick", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onClick)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fireClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this,
		/*var. type*/tTJSNI_MenuItem);

	_this->OnClick();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fireClick)
//----------------------------------------------------------------------

//--properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(caption)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		ttstr res;
		_this->GetCaption(res);
		*result = res;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetCaption(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(caption)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(checked)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		*result = _this->GetChecked();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetChecked(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(checked)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enabled)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		*result = _this->GetEnabled();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetEnabled(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(enabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(group)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		*result = _this->GetGroup();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetGroup((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(group)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(radio)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		*result = _this->GetRadio();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetRadio(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(radio)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(shortcut)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		ttstr res;
		_this->GetShortcut(res);
		*result = res;
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetShortcut(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(shortcut)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		*result = _this->GetVisible();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetVisible(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(visible)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(parent)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		tTJSNI_BaseMenuItem *parent = _this->GetParent();
		if(parent)
		{
			iTJSDispatch2 *dsp = parent->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2 *)NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(parent)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(children)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		iTJSDispatch2 *dsp = _this->GetChildrenArrayNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(children)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(root) // not rootMenuItem
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		tTJSNI_BaseMenuItem * root = _this->GetRootMenuItem();
		if (root)
		{
			iTJSDispatch2 *dsp = root->GetOwnerNoAddRef();
			if (result) *result = tTJSVariant(dsp, dsp);
		}
		else
		{
			if (result) *result = tTJSVariant((iTJSDispatch2 *)NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(root) // not rootMenuItem
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(window)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		tTJSNI_Window * window = _this->GetWindow();
		if (window)
		{
			iTJSDispatch2 *dsp = window->GetOwnerNoAddRef();
			if (result) *result = tTJSVariant(dsp, dsp);
		}
		else
		{
			if (result) *result = tTJSVariant((iTJSDispatch2 *)NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(window)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(index)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		if (result) *result = (tTVInteger)_this->GetIndex();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_MenuItem);
		_this->SetIndex((tjs_int)(*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(index)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateMenuItemObject(iTJSDispatch2 * window)
{
	struct tHolder
	{
		iTJSDispatch2 * Obj;
		tHolder() { Obj = new tTJSNC_MenuItem(); }
		~tHolder() { Obj->Release(); }
	} static menuitemclass;

	iTJSDispatch2 *out;
	tTJSVariant param(window);
	tTJSVariant *pparam[2] = {&param, &param};
	if(TJS_FAILED(menuitemclass.Obj->CreateNew(0, NULL, NULL, &out, 2, pparam,
		menuitemclass.Obj)))
		TVPThrowExceptionMessage(TVPInternalError,
			TJS_W("TVPCreateMenuItemObject"));

	return out;
}
//---------------------------------------------------------------------------

