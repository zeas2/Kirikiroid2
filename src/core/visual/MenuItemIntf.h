//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "MenuItem" class interface
//---------------------------------------------------------------------------
#ifndef MenuItemIntfH
#define MenuItemIntfH

#include "tjsNative.h"
#include "EventIntf.h"
#include "ObjectList.h"
#include <algorithm>

template <typename T>
struct ObjectVector : public std::vector<T*> { // thread safe vector
	tTJSSpinLock Lock;

	tjs_int Find(const T * object) const {
		tTJSSpinLockHolder holder(((ObjectVector*)this)->Lock);
		auto it = std::find(this->begin(), this->end(), object);
		if (it == this->end()) return -1;
		return it - this->begin();
	}

	bool Add(T* object, tjs_int idx = -1) {
		tTJSSpinLockHolder holder(Lock);
		if (std::find(this->begin(), this->end(), object) != this->end()) return false;
		if (idx == -1)
			this->push_back(object);
		else
			this->insert(this->begin() + idx, object);
		return true;
	}

	bool Remove(T* object) {
		tTJSSpinLockHolder holder(Lock);
		auto it = std::find(this->begin(), this->end(), object);
		if (it == this->end()) return false;
		this->erase(it);
		return true;
	}
};

//---------------------------------------------------------------------------
// tTJSNI_MenuItem
//---------------------------------------------------------------------------
class tTJSNI_Window;
class tTJSNI_MenuItem;
class tTJSNI_BaseMenuItem : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;
	friend class tTJSNI_MenuItem;
protected:
	ObjectVector<tTJSNI_BaseMenuItem> Children;
	tTJSNI_Window *Window;

	iTJSDispatch2 *Owner;
	tTJSVariantClosure ActionOwner; // object to send action

	tTJSNI_BaseMenuItem *Parent;

	bool ChildrenArrayValid;
	iTJSDispatch2 * ChildrenArray;
	iTJSDispatch2 * ArrayClearMethod;

public:
	tTJSNI_BaseMenuItem();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

public:
	static tTJSNI_MenuItem * CastFromVariant(const tTJSVariant & from);

protected:
	virtual bool CanDeliverEvents() const = 0; // must be implemented in each platforms

protected:
	void AddChild(tTJSNI_BaseMenuItem *item);
	void RemoveChild(tTJSNI_BaseMenuItem *item);

public:
	virtual void Add(tTJSNI_MenuItem * item) = 0;
	virtual void Insert(tTJSNI_MenuItem *item, tjs_int index) = 0;
	virtual void Remove(tTJSNI_MenuItem *item) = 0;

public:
	tTJSVariantClosure GetActionOwnerNoAddRef() const { return ActionOwner; }

	iTJSDispatch2 * GetOwnerNoAddRef() const { return Owner; }

	tTJSNI_BaseMenuItem * GetParent() const { return Parent; }

	tTJSNI_BaseMenuItem * GetRootMenuItem() const;

	tTJSNI_Window * GetWindow() const { return Window; }

	iTJSDispatch2 * GetChildrenArrayNoAddRef();


public:
	void OnClick(void); // fire onClick event
};
//---------------------------------------------------------------------------

#include "MenuItemImpl.h" // must define tTJSNI_MenuItem class

//---------------------------------------------------------------------------
// tTJSNC_MenuItem : TJS MenuItem class
//---------------------------------------------------------------------------
class tTJSNC_MenuItem : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_MenuItem();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
	/*
		implement this in each platform.
		this must return a proper instance of tTJSNC_MenuItem.
	*/
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_MenuItem();
	/*
		implement this in each platform.
		this must return a proper instance of tTJSNC_MenuItem.
		usually simple returns: new tTJSNC_MenuItem();
	*/
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern iTJSDispatch2 * TVPCreateMenuItemObject(iTJSDispatch2 * window);
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTVPMenuItemOnClickInputEvent : onClick input event class
//---------------------------------------------------------------------------
class tTVPOnMenuItemClickInputEvent : public tTVPBaseInputEvent
{
	static tTVPUniqueTagForInputEvent Tag;
public:
	tTVPOnMenuItemClickInputEvent(tTJSNI_BaseMenuItem *menu) :
		tTVPBaseInputEvent(menu, Tag) {};
	void Deliver() const
	{ ((tTJSNI_BaseMenuItem*)GetSource())->OnClick(); }
};
//---------------------------------------------------------------------------

#endif
