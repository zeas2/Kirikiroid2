//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Layer Management
//---------------------------------------------------------------------------
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "tjsCommHead.h"

#include <math.h>
#include <cstdlib>
#include <cmath>

#include "tjsArray.h"
#include "LayerIntf.h"
#include "MsgIntf.h"
#include "LayerBitmapIntf.h"
#include "LayerTreeOwner.h"
#include "GraphicsLoaderIntf.h"
#include "StorageIntf.h"
#include "tvpgl.h"
#include "EventIntf.h"
#include "SysInitIntf.h"
#include "TickCount.h"
#include "DebugIntf.h"
#include "LayerManager.h"
#include "BitmapIntf.h"

#include "TVPColor.h"
//#include "TVPSysFont.h"
#include "FontRasterizer.h"
#include "RectItf.h"
#include "FontSystem.h"
#include "tjsDictionary.h"
#include "ConfigManager/IndividualConfigManager.h"
#include "vkdefine.h"
#include "RenderManager.h"
#include <cstdlib>
#include "FontImpl.h"

extern void TVPSetFontRasterizer( tjs_int index );
extern tjs_int TVPGetFontRasterizer();
extern FontRasterizer* GetCurrentRasterizer();
extern void TVPMapPrerenderedFont(const tTVPFont & font, const ttstr & storage);
extern void TVPUnmapPrerenderedFont(const tTVPFont & font);
extern tjs_int TVPGetCursor(const ttstr & name);
//---------------------------------------------------------------------------
// global flags
//---------------------------------------------------------------------------
bool TVPFreeUnusedLayerCache = false;
	// set true to free unused layer cache bitmap
	// (layer cache is not freed until system compact event if this is false)
//---------------------------------------------------------------------------

static bool IsGPU() {
	static bool isGPU = !TVPIsSoftwareRenderManager()
		&& !IndividualConfigManager::GetInstance()->GetValue<bool>("ogl_accurate_render", false);
	return isGPU;
}

//---------------------------------------------------------------------------
// temporary bitmap management
//---------------------------------------------------------------------------
class tTVPTempBitmapHolder;
static tTVPTempBitmapHolder * TVPTempBitmapHolder = NULL;
class tTVPTempBitmapHolder : public tTVPCompactEventCallbackIntf
{
	/*
		Initial layer bitmap and temporary bitmaps ( for window updating ) holder

		TVP(kirikiri) will be sometimes executed as a child process,
		simply returns a argument information, or simply manages the restarting
		of itself.
		object that is not necessary at first should not be created at
		beginning of the process :-)
	*/

	tTVPBaseTexture *Bitmap;

	std::vector<tTVPBaseTexture *> Temporaries;
	tjs_uint TempLevel;
	bool TempCompactInit;

private:
	tjs_int RefCount;
	tTVPTempBitmapHolder() : TempLevel(0), TempCompactInit(false)
	{
		// the default image must be a transparent, white colored rectangle
		RefCount = 1;
        Bitmap = new tTVPBaseTexture(32, 32);
		Bitmap->Fill(tTVPRect(0, 0, 32, 32), TVP_RGBA2COLOR(255, 255, 255, 0));
	}

	~tTVPTempBitmapHolder()
	{
		std::vector<tTVPBaseTexture*>::iterator i;
		for(i = Temporaries.begin(); i != Temporaries.end(); i++)
		{
			delete (*i);
		}
		if(TempCompactInit) TVPRemoveCompactEventHook(this);
        if(Bitmap) delete Bitmap;
	}

	tTVPBaseTexture * InternalGetTemp(tjs_uint w, tjs_uint h, bool fit)
	{
		// compact initialization
		if(!TempCompactInit)
		{
			TVPAddCompactEventHook(this);
			TempCompactInit = true;
		}

		// align width to even
		if(!fit) w += (w & 1);

		// get temporary bitmap (nested)
		TempLevel++;
		if(TempLevel > Temporaries.size())
		{
			// increase buffer size
			tTVPBaseTexture *bmp = new tTVPBaseTexture(w, h);
			Temporaries.push_back(bmp);
			return bmp;
		}
		else
		{
			tTVPBaseTexture *bmp = Temporaries[TempLevel - 1];
			if(!fit)
			{
				tjs_uint bw = bmp->GetWidth();
				tjs_uint bh = bmp->GetHeight();
				if(bw < w || bh < h)
				{
					// increase image size
					bmp->SetSize(bw > w ? bw:w, bh > h ? bh:h, false);
				}
			}
			else
			{
				// the size must be fitted
				tjs_uint bw = bmp->GetWidth();
				tjs_uint bh = bmp->GetHeight();
				if(bw != w || bh != h)
					bmp->SetSize(w, h, false);
			}
			return bmp;
		}
	}

	void InternalFreeTemp()
	{
		if(TempLevel == 0) return ; // this must be a logical failure
		TempLevel--;
	}

	void CompactTempBitmap()
	{
		// compact tmporary bitmap cache
		std::vector<tTVPBaseTexture*>::iterator i;
		for(i = Temporaries.begin() + TempLevel; i != Temporaries.end();
			i++)
		{
			delete (*i);
		}

		Temporaries.resize(TempLevel);
	}


	void TJS_INTF_METHOD OnCompact(tjs_int level)
	{
		// OnCompact method from tTVPCompactEventCallbackIntf
		// called when the application is idle, deactivated, minimized, or etc...
		if(level >= TVP_COMPACT_LEVEL_DEACTIVATE) CompactTempBitmap();
	}

public:
	static void AddRef()
	{
		if(!TVPTempBitmapHolder)
			TVPTempBitmapHolder = new tTVPTempBitmapHolder();
		else
			TVPTempBitmapHolder ->RefCount++;
	}

	static void Release()
	{
		if(!TVPTempBitmapHolder) return;
		if(TVPTempBitmapHolder->RefCount == 1)
		{
			delete TVPTempBitmapHolder;
			TVPTempBitmapHolder = NULL;
		}
		else
		{
			TVPTempBitmapHolder->RefCount --;
		}
	}

	static const tTVPBaseTexture * Get()
		{ return TVPTempBitmapHolder->Bitmap; }


	static tTVPBaseTexture * GetTemp(tjs_uint w, tjs_uint h, bool fit = false)
		{ return TVPTempBitmapHolder->InternalGetTemp(w, h, fit); }

	static void FreeTemp()
		{ TVPTempBitmapHolder->InternalFreeTemp(); }
};
//---------------------------------------------------------------------------
const tTVPBaseTexture & TVPGetInitialBitmap()
{
	tTVPTempBitmapHolder::AddRef(); // ensure default bitmap
	const tTVPBaseTexture *bmp = TVPTempBitmapHolder->Get();
	tTVPTempBitmapHolder::Release();

	return *bmp;
}
//---------------------------------------------------------------------------
void TVPTempBitmapHolderAddRef()
{
	tTVPTempBitmapHolder::AddRef(); // ensure default bitmap
}
//---------------------------------------------------------------------------
void TVPTempBitmapHolderRelease()
{
	tTVPTempBitmapHolder::Release();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// FOR EACH CHILD
//---------------------------------------------------------------------------
#define TVP_LAYER_FOR_EACH_CHILD_BEGIN(varname)              \
	{ \
		tObjectListSafeLockHolder<tTJSNI_BaseLayer> __holder(Children); \
		tjs_int __count = Children.GetSafeLockedObjectCount(); \
		tjs_int __i; \
		for(__i = 0; __i < __count; __i++)                                 \
		{                                                                  \
			tTJSNI_BaseLayer * varname = Children.GetSafeLockedObjectAt(__i); \
			if(!varname) continue;
               
#define TVP_LAYER_FOR_EACH_CHILD_END \
		} \
		ChildrenArrayValid = false; \
		ChildrenOrderIndexValid = false; \
		if(Manager) Manager->InvalidateOverallIndex(); \
	}
//---------------------------------------------------------------------------
// for each child ( for static only access )
#define TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(varname)              \
	{ \
		tjs_int __count = Children.GetCount(); \
		tjs_int __i; \
		for(__i = 0; __i < __count; __i++)                                 \
		{                                                                  \
			tTJSNI_BaseLayer * varname = Children[__i]; \
			if(!varname) continue;

#define TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END \
		} \
	}

#define TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BACKWARD_BEGIN(varname)              \
	{ \
		tjs_int __count = Children.GetCount(); \
		tjs_int __i; \
		for(__i = __count-1; __i >= 0; __i--)                                 \
		{                                                                  \
			tTJSNI_BaseLayer * varname = Children[__i]; \
			if(!varname) continue;

#define TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BACKWARD_END \
		} \
	}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Recursive Call
//---------------------------------------------------------------------------
#define TVP_LAYER_REC_CALL(funccall, action)              \
	action;                                                        \
	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)                  \
		child -> funccall;                                  \
	TVP_LAYER_FOR_EACH_CHILD_END
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// global options
//---------------------------------------------------------------------------
tTVPGraphicSplitOperationType TVPGraphicSplitOperationType = gsotNone;
bool TVPDefaultHoldAlpha = false;
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// below is the tTJSNI_BaseLayer implementation ( pretty large )
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// object lifetime stuff
//---------------------------------------------------------------------------
tTJSNI_BaseLayer::tTJSNI_BaseLayer()
{
	// creates bitmap holder
	tTVPTempBitmapHolder::AddRef();

	// object lifetime stuff
	Owner = NULL;
	ActionOwner.ObjThis = ActionOwner.Object = NULL;
	Shutdown = false;
	CompactEventHookInit = false;

	// interface to layer manager
	Manager = NULL;

	// tree management
	Parent = NULL;
	Visible = false;
	Opacity = 255;
	VisibleChildrenCount = -1;
	ChildrenArray = NULL;
	ChildrenArrayValid = false;
	ArrayClearMethod = NULL;
	OrderIndex = 0;
	OverallOrderIndex = 0;
	ChildrenOrderIndexValid = false;
	AbsoluteOrderMode = false; // initially relative mode
	AbsoluteOrderIndex = 0;

	// layer type management
	DisplayType = Type = ltAlpha;
		// later reset this if the layer becomes a primary layer
	NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);

	// geographical management
	ExposedRegionValid = false;
	Rect.left = 0;
	Rect.top = 0;
	Rect.right = 32;
	Rect.bottom = 32;

	// input event / hit test management
	HitType = htMask;
	HitThreshold = 16;
	Cursor = 0; // 0 = crDefault
	CursorX_Work = 0;
	ShowParentHint = true;
	IgnoreHintSensing = false;
	UseAttention = false;
	ImeMode = ::imDisable;
	AttentionLeft = AttentionTop = 0;

	Enabled = true;
	Focusable = false;
	JoinFocusChain = true;

	// image buffer management
	MainImage = NULL;
	CanHaveImage = true;
	ProvinceImage = NULL;
	ImageLeft = 0;
	ImageTop = 0;

	// cache management
	CacheEnabledCount = 0;
	CacheBitmap = NULL;
	Cached = false;

	// drawing function stuff
	Face = dfAuto;
	UpdateDrawFace();
	ImageModified = false;
	HoldAlpha = TVPDefaultHoldAlpha;
	ClipRect.left = 0;
	ClipRect.right = 0;
	ClipRect.top = 0;
	ClipRect.bottom = 0;

	// Updating management
	CallOnPaint = false;
	InCompletion = false;

	// transition management
	DivisibleTransHandler = NULL;
	GiveUpdateTransHandler = NULL;
	TransDest = NULL;
	TransDestObj = NULL;
	TransSrc = NULL;
	TransSrcObj = NULL;
	InTransition = false;
	TransWithChildren = false;
	DestSLP = NULL;
	SrcSLP = NULL;
	TransCompEventPrevented = false;
	UseTransTickCallback = false;
	TransTickCallback = tTJSVariantClosure(NULL, NULL);

	// allocate the default image
	AllocateDefaultImage();

	// interface to font object
	Font = MainImage->GetFont(); // retrieve default font
	FontObject = NULL;

#if 0
	// province management
	ClearProvinceInformation();
#endif
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer::~tTJSNI_BaseLayer()
{
	tTVPTempBitmapHolder::Release();
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_BaseLayer::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	Owner = tjs_obj; // no addref

	// get the window native instance
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	//if(clo.Object == NULL) TVPThrowExceptionMessage(TVPSpecifyWindow);
	if(clo.Object == NULL) TVPThrowExceptionMessage(TJS_W("Please specify layerTreeOwnerInterface object"));

	class iTVPLayerTreeOwner* lto = NULL;
	tTJSVariant iface_v;
	if(TJS_FAILED(clo.PropGet(0, TJS_W("layerTreeOwnerInterface"), NULL, &iface_v, NULL)))
		TVPThrowExceptionMessage( TJS_W("Cannot Retrive Layer Tree Owner Interface.") );
	lto = reinterpret_cast<iTVPLayerTreeOwner *>((tjs_intptr_t)(tjs_int64)iface_v);

	// get the layer native instance
	clo = param[1]->AsObjectClosureNoAddRef();
	tTJSNI_Layer *lay = NULL;
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&lay)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}

	// retrieve manager
	// layer manager is the same as the parent, if the parent is given
	if(lay)
	{
		Manager = lay->GetManager();
		if (Manager)
			Manager->AddRef(); // lock manager
	}

	// register to parent layer
	if(lay) Join(lay);

	// is primarylayer ?
	// ask window to create layer manager 
	if(!lay)
	{
		Manager = new tTVPLayerManager(lto);
		Manager->AttachPrimary(this);
		Manager->RegisterSelfToWindow();

		Type = DisplayType = ltOpaque; // initially ltOpaque
		NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 255);
		UpdateDrawFace();
		HitThreshold = 0;
	}
//	IncCacheEnabledCount(); ///// -------------------- test

	ActionOwner = param[0]->AsObjectClosure();

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD
tTJSNI_BaseLayer::Invalidate()
{
	Shutdown = true;

	// stop transition
	StopTransition();
	if(TransDest) TransDest->StopTransition();
	if(DestSLP) DestSLP->Release(), DestSLP = NULL;
	if(SrcSLP) SrcSLP->Release(), SrcSLP = NULL;

	// cancel events
	TVPCancelSourceEvents(Owner);

	// release all objects
	if(IsPrimary())
	{
		if(Manager) Manager->DetachPrimary();
		// also detach from draw device
		Manager->UnregisterSelfFromWindow();
	}

	if(Manager)
	{
		Manager->Release(); // no longer used in this context
		Manager = NULL;
	}

	// part from the parent
	Part();

	// sever all children
	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
		child->Part();
	TVP_LAYER_FOR_EACH_CHILD_END

	// invalidate font object
	if(FontObject)
	{
		FontObject->Invalidate(0, NULL, NULL, FontObject);
		FontObject->Release();
	}

	// deallocate image
	DeallocateImage();

	// free cache image
	DeallocateCache();

	// release the owner
	ActionOwner.Release();
	ActionOwner.ObjThis = ActionOwner.Object = NULL;

	// release Children array
	if(ChildrenArray) ChildrenArray->Release(), ChildrenArray = NULL;
	if(ArrayClearMethod) ArrayClearMethod->Release(), ArrayClearMethod = NULL;

	// unregister from compact event hook
	if(CompactEventHookInit) TVPRemoveCompactEventHook(this);

	// cancel events once more
	TVPCancelSourceEvents(Owner);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::RegisterCompactEventHook()
{
	if(!CompactEventHookInit)
	{
		TVPAddCompactEventHook(this);
		CompactEventHookInit = true;
	}
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_BaseLayer::OnCompact(tjs_int level)
{
	// method from tTVPCompactEventCallbackIntf
	if(level >= TVP_COMPACT_LEVEL_DEACTIVATE) CompactCache();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Interface to Manager
//---------------------------------------------------------------------------
iTVPLayerTreeOwner * tTJSNI_BaseLayer::GetLayerTreeOwner() const
{
	if(!Manager) return NULL;
	return Manager->GetLayerTreeOwner();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tree management
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Join(tTJSNI_BaseLayer *parent)
{
	if(parent == this)
		TVPThrowExceptionMessage(TVPCannotSetParentSelf);
	if(parent && parent->Manager != Manager)
		TVPThrowExceptionMessage(TVPCannotMoveToUnderOtherPrimaryLayer);
	if(Parent) Part();
	Parent = parent;
	if(Parent) parent->AddChild(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Part()
{
	Update();

	if(Manager) Manager->NotifyPart(this);

	if(Parent != NULL)
		Parent->SeverChild(this), Parent = NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AddChild(tTJSNI_BaseLayer *child)
{
	NotifyChildrenVisualStateChanged();
	Children.Add(child);
	if(AbsoluteOrderMode)
	{
		// first insertion
		Children.Compact();
		tjs_int count = Children.GetCount();
		if(count >= 2)
		{
			tTJSNI_BaseLayer *last = Children[count-2];
			child->AbsoluteOrderIndex = last->GetAbsoluteOrderIndex() +1;
		}
	}
	ChildrenArrayValid = false;
	ChildrenOrderIndexValid = false;
	if(Manager) Manager->CheckTreeFocusableState(child); // check focusable state of child
	if(Manager) Manager->InvalidateOverallIndex();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SeverChild(tTJSNI_BaseLayer *child)
{
	if(Manager) Manager->BlurTree(child); // remove focus from "child"
	NotifyChildrenVisualStateChanged();
	Children.Remove(child);
	ChildrenArrayValid = false;
	ChildrenOrderIndexValid = false;
	if(Manager) Manager->InvalidateOverallIndex();
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseLayer::GetChildrenArrayObjectNoAddRef()
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

		tjs_int count = 0;
		TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
			iTJSDispatch2 *dsp = child->Owner;
			tTJSVariant val(dsp, dsp);
			ChildrenArray->PropSetByNum(TJS_MEMBERENSURE, count, &val,
				ChildrenArray);
			count++;
		TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

		ChildrenArrayValid = true;
	}

	return ChildrenArray;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer * tTJSNI_BaseLayer::GetAncestorChild(tTJSNI_BaseLayer *ancestor)
{
	// retrieve "ancestor"'s child that is ancestor of this ( can be thisself )
	tTJSNI_BaseLayer *c = this;
	tTJSNI_BaseLayer *p = Parent;
	while(p)
	{
		if(p == ancestor) return c;
		c = p;
		p = p->Parent;
	}
	return NULL;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::IsAncestor(tTJSNI_BaseLayer *ancestor)
{
	// is "ancestor" is ancestor of this layer ? (cannot be itself)
	tTJSNI_BaseLayer *p = Parent;
	while(p)
	{
		if(p == ancestor) return true;
		p = p->Parent;
	}
	return false;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::RecreateOverallOrderIndex(tjs_uint& index,
		std::vector<tTJSNI_BaseLayer*>& nodes)
{
	OverallOrderIndex = index;
	index++;

	nodes.push_back(this);

	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		child->RecreateOverallOrderIndex(index, nodes);
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Exchange(tTJSNI_BaseLayer *target, bool keepchildren)
{
	// exchange this for the other layer
	if(this == target) return;

	tTJSNI_BaseLayer *this_ancestor_child = this->GetAncestorChild(target);
	tTJSNI_BaseLayer *target_ancestor_child = target->GetAncestorChild(this);

	tTJSNI_BaseLayer *this_parent = this->Parent;
	tTJSNI_BaseLayer *target_parent = target->Parent;

	bool this_primary = this->IsPrimary();
	bool target_primary = target->IsPrimary();

	bool this_parent_absolute = true;
	if(this->Parent) this_parent_absolute = this->Parent->AbsoluteOrderMode;
	tjs_int this_index = this->GetAbsoluteOrderIndex();

	bool target_parent_absolute = true;
	if(target->Parent) target_parent_absolute = target->Parent->AbsoluteOrderMode;
	tjs_int target_index = target->GetAbsoluteOrderIndex();

	// remove primary
	if(Manager)
	{
		if(this_primary) Manager->DetachPrimary();
		if(target_primary) Manager->DetachPrimary();
	}

	// part from each parent
	this->Part();
	target->Part();

	tTJSNI_BaseLayer * this_joined_parent;
	tTJSNI_BaseLayer * target_joined_parent;
	if(this_ancestor_child)
	{
		// "this" is a descendant of the "target"
		if(this_ancestor_child != this) this_ancestor_child->Part();

		if(!keepchildren)
		{
			// join to each target's parent
			this->Join(this_joined_parent = target_parent);
			if(target == this_parent)
				target->Join(target_joined_parent = this);
			else
				target->Join(target_joined_parent = this_parent);
		}
		else
		{
			// sever children
			std::vector<tjs_int> this_orders;
			std::vector<tjs_int> target_orders;
			tObjectList<tTJSNI_BaseLayer> this_children(this->Children);
			tObjectList<tTJSNI_BaseLayer> target_children(target->Children);
			tjs_int this_children_count = this_children.GetActualCount();
			tjs_int target_children_count = target_children.GetActualCount();

			for(int i = 0; i < this_children_count; i++)
			{
				this_orders.push_back(this_children[i]->GetAbsoluteOrderIndex());
				this_children[i]->Part();
			}
	
			for(int i = 0; i < target_children_count; i++)
			{
				target_orders.push_back(target_children[i]->GetAbsoluteOrderIndex());
				target_children[i]->Part();
			}

			// join to each target's parent
			this->Join(this_joined_parent = target_parent);
			if(target == this_parent)
				target->Join(target_joined_parent = this);
			else
				target->Join(target_joined_parent = this_parent);

			// let children join
			for(int i = 0; i < this_children_count; i++)
				this_children[i]->Join(target);
			for(int i = 0; i < target_children_count; i++)
				target_children[i]->Join(this);

			if(this->AbsoluteOrderMode && target->AbsoluteOrderMode)
			{
				// reset order index
				for(int i = 0; i < this_children_count; i++)
					this_children[i]->SetAbsoluteOrderIndex(this_orders[i]);
				for(int i = 0; i < target_children_count; i++)
					target_children[i]->SetAbsoluteOrderIndex(target_orders[i]);
			}
		}


		if(this_ancestor_child != this) this_ancestor_child->Join(this);
	}
	else if(target_ancestor_child)
	{
		// "target" is a descendant of "this"
		if(target_ancestor_child != target) target_ancestor_child->Part();

		if(!keepchildren)
		{
			// join to each target's parent
			if(this == target_parent)
				this->Join(this_joined_parent = target);
			else
				this->Join(this_joined_parent = target_parent);
			target->Join(target_joined_parent = this_parent);
		}
		else
		{
			// sever children
			std::vector<tjs_int> this_orders;
			std::vector<tjs_int> target_orders;
			tObjectList<tTJSNI_BaseLayer> this_children(this->Children);
			tObjectList<tTJSNI_BaseLayer> target_children(target->Children);
			tjs_int this_children_count = this_children.GetActualCount();
			tjs_int target_children_count = target_children.GetActualCount();

			for(int i = 0; i < this_children_count; i++)
			{
				this_orders.push_back(this_children[i]->GetAbsoluteOrderIndex());
				this_children[i]->Part();
			}
			for(int i = 0; i < target_children_count; i++)
			{
				target_orders.push_back(target_children[i]->GetAbsoluteOrderIndex());
				target_children[i]->Part();
			}

			// join to each target's parent
			if(this == target_parent)
				this->Join(this_joined_parent = target);
			else
				this->Join(this_joined_parent = target_parent);
			target->Join(target_joined_parent = this_parent);

			// let children join
			for(int i = 0; i < this_children_count; i++)
				this_children[i]->Join(target);
			for(int i = 0; i < target_children_count; i++)
				target_children[i]->Join(this);

			if(this->AbsoluteOrderMode && target->AbsoluteOrderMode)
			{
				// reset order index
				for(int i = 0; i < this_children_count; i++)
					this_children[i]->SetAbsoluteOrderIndex(this_orders[i]);
				for(int i = 0; i < target_children_count; i++)
					target_children[i]->SetAbsoluteOrderIndex(target_orders[i]);
			}
		}

		if(target_ancestor_child != target) target_ancestor_child->Join(target);
	}
	else
	{
		// two layers have no parent-child relationship
		if(!keepchildren)
		{
			// join to each target's parent
			this->Join(this_joined_parent = target_parent);
			target->Join(target_joined_parent = this_parent);
		}
		else
		{
			// sever children
			std::vector<tjs_int> this_orders;
			std::vector<tjs_int> target_orders;
			tObjectList<tTJSNI_BaseLayer> this_children(this->Children);
			tObjectList<tTJSNI_BaseLayer> target_children(target->Children);
			tjs_int this_children_count = this_children.GetActualCount();
			tjs_int target_children_count = target_children.GetActualCount();

			for(int i = 0; i < this_children_count; i++)
			{
				this_orders.push_back(this_children[i]->GetAbsoluteOrderIndex());
				this_children[i]->Part();
			}
			for(int i = 0; i < target_children_count; i++)
			{
				target_orders.push_back(target_children[i]->GetAbsoluteOrderIndex());
				target_children[i]->Part();
			}

			// join to each target's parent
			this->Join(this_joined_parent = target_parent);
			target->Join(target_joined_parent = this_parent);

			// let children join
			for(int i = 0; i < this_children_count; i++)
				this_children[i]->Join(target);
			for(int i = 0; i < target_children_count; i++)
				target_children[i]->Join(this);

			if(this->AbsoluteOrderMode && target->AbsoluteOrderMode)
			{
				// reset order index
				for(int i = 0; i < this_children_count; i++)
					this_children[i]->SetAbsoluteOrderIndex(this_orders[i]);
				for(int i = 0; i < target_children_count; i++)
					target_children[i]->SetAbsoluteOrderIndex(target_orders[i]);
			}
		}
	}

	// attach primary
	if(Manager)
	{
		if(this_primary) Manager->AttachPrimary(target);
		if(target_primary) Manager->AttachPrimary(this);
	}

	// reset order index
	if(target_joined_parent == this_joined_parent &&
		target_joined_parent &&
		target_joined_parent->AbsoluteOrderMode == target_parent_absolute &&
		this_joined_parent->AbsoluteOrderMode == this_parent_absolute &&
		target_parent_absolute == this_parent_absolute &&
		target_parent_absolute == false)
	{
		// two layers have the same parent and the same order mode
		if(this_index < target_index)
		{
			target->SetOrderIndex(this_index);
			this->SetOrderIndex(target_index);
		}
		else
		{
			this->SetOrderIndex(target_index);
			target->SetOrderIndex(this_index);
		}
	}
	else
	{
		if(target_joined_parent &&
			target_joined_parent->AbsoluteOrderMode == target_parent_absolute)
		{
			if(target_parent_absolute)
				target->SetAbsoluteOrderIndex(target_index);
			else
				target->SetOrderIndex(target_index);
		}

		if(this_joined_parent &&
			this_joined_parent->AbsoluteOrderMode == this_parent_absolute)
		{
			if(this_parent_absolute)
				this->SetAbsoluteOrderIndex(this_index);
			else
				this->SetOrderIndex(this_index);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CheckChildrenVisibleState()
{
	if(!GetCount())
	{
		VisibleChildrenCount = 0;
		return;
	}
	VisibleChildrenCount = 0;

	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		if(child->GetVisible() && child->GetOpacity() != 0)
			VisibleChildrenCount++;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::NotifyChildrenVisualStateChanged()
{
	VisibleChildrenCount = -1;
	SetToCreateExposedRegion(); // in geographical management
	if(Manager) Manager->NotifyVisualStateChanged();
}
//---------------------------------------------------------------------------
tjs_uint tTJSNI_BaseLayer::GetOverallOrderIndex()
{
	if(!Manager) return 0;
	Manager->RecreateOverallOrderIndex();
	return OverallOrderIndex;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::RecreateOrderIndex()
{
	// recreate order index information
	tjs_uint index = 0;

	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		child->OrderIndex = index;
		index++;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

	ChildrenOrderIndexValid = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetVisible(bool st)
{
	if(Visible != st)
	{
		if(IsPrimary() && !st)
			TVPThrowExceptionMessage(TVPCannotSetPrimaryInvisible);
		if(!st) Update();
		Visible = st;
		if(st) Update();
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		if(Visible)
		{
			if(Manager) Manager->CheckTreeFocusableState(this);
		}
		else
		{
			if(Manager) Manager->BlurTree(this); // in input/keyboard focus management
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetOpacity(tjs_int opa)
{
	if(Opacity != opa)
	{
		if(IsPrimary() && opa!=255)
			TVPThrowExceptionMessage(TVPCannotSetPrimaryInvisible);
		Opacity = opa;
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		Update();
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::IsPrimary() const
{
	if(!Manager) return false;
	return Manager->GetPrimaryLayer() == this;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetParentVisible() const
{
	// is parent visible? this does not check opacity
	tTJSNI_BaseLayer *par = Parent;
	while(par)
	{
		if(!par->Visible) { return false; }
		par = par->Parent;
	}

	return true;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer * tTJSNI_BaseLayer::GetNeighborAbove(bool loop)
{
	if(!Manager) return NULL;

	tjs_uint index = GetOverallOrderIndex();
	std::vector<tTJSNI_BaseLayer *> & allnodes = Manager->GetAllNodes();

	if(allnodes.size() == 0) return NULL; // must be an error !!

	if(index == 0)
	{
		// first ( primary )
		if(loop)
			return *(allnodes.end()-1);
		else
			return NULL;
	}

	return *(allnodes.begin() + index -1);
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer * tTJSNI_BaseLayer::GetNeighborBelow(bool loop)
{
	if(!Manager) return NULL;

	tjs_uint index = GetOverallOrderIndex();
	std::vector<tTJSNI_BaseLayer *> & allnodes = Manager->GetAllNodes();

	if(allnodes.size() == 0) return NULL; // must be an error !!

	if(index == allnodes.size() -1)
	{
		// last
		if(loop)
			return *(allnodes.begin());
		else
			return NULL;
	}

	return *(allnodes.begin() + index +1);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CheckZOrderMoveRule(tTJSNI_BaseLayer *lay)
{
	if(!Parent) TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);
	if(Parent->Children.GetActualCount() <= 1)
		TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);
	if(lay == this)
		TVPThrowExceptionMessage(TVPCannotMoveNextToSelfOrNotSiblings);
	if(lay->Parent != Parent)
		TVPThrowExceptionMessage(TVPCannotMoveNextToSelfOrNotSiblings);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ChildChangeOrder(tjs_int from, tjs_int to)
{
	// called from children; change child's order from "from" to "to"
	// given orders are treated as orders before re-ordering.
	if(from == to) return; // no action

	tTJSNI_BaseLayer *fromlay;
	tTVPComplexRect rects;
	Children.Compact();
	if(from < to)
	{
		// forward

		// rotate
		fromlay = Children[from];
		for(tjs_int i = from; i < to; i++)
		{
			Children[i] = Children[i + 1];
			tTVPRect r = fromlay->Rect;
			if(TVPIntersectRect(&r, r, Children[i]->Rect))
				rects.Or(r); // add rectangle to update
		}
		Children[to] = fromlay;
	}
	else
	{
		// backward

		// rotate
		fromlay = Children[from];
		for(tjs_int i = from; i > to; i--)
		{
			Children[i] = Children[i - 1];
			tTVPRect r = fromlay->Rect;
			if(TVPIntersectRect(&r, r, Children[i]->Rect))
				rects.Or(r);
		}
		Children[to] = fromlay;
	}

	// update
	Update(rects);

	// clear caches
	ChildrenArrayValid = false;
	ChildrenOrderIndexValid = false;
	if(Manager) Manager->InvalidateOverallIndex();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ChildChangeAbsoluteOrder(tjs_int from, tjs_int abs_to)
{
	// find index order
	tjs_int to = 0;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		if(child->AbsoluteOrderIndex >= abs_to) break;
		to++;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

	if(from<to) to--;

	ChildChangeOrder(from, to);

}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::MoveBefore(tTJSNI_BaseLayer *lay)
{
	// move before sibling : lay
	// lay must not be a null
	if(Parent) Parent->SetAbsoluteOrderMode(false);

	CheckZOrderMoveRule(lay);

	tjs_int this_order = GetOrderIndex();
	tjs_int lay_order = lay->GetOrderIndex();

	if(this_order < lay_order)
		Parent->ChildChangeOrder(this_order, lay_order); // move forward
	else
		Parent->ChildChangeOrder(this_order, lay_order + 1); // move backward
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::MoveBehind(tTJSNI_BaseLayer *lay)
{
	// move behind sibling : lay
	// lay must not be a null
	if(Parent) Parent->SetAbsoluteOrderMode(false);

	CheckZOrderMoveRule(lay);

	tjs_int this_order = GetOrderIndex();
	tjs_int lay_order = lay->GetOrderIndex();

	if(this_order < lay_order)
		Parent->ChildChangeOrder(this_order, lay_order - 1); // move forward
	else
		Parent->ChildChangeOrder(this_order, lay_order); // move backward
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetOrderIndex(tjs_int index)
{
	// change order index
	if(!Parent) TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);

	Parent->SetAbsoluteOrderMode(false);

	if(index < 0) index = 0;
	if(index >= (tjs_int)Parent->Children.GetActualCount())
		index = Parent->Children.GetActualCount()-1;

	Parent->ChildChangeOrder(GetOrderIndex(), index);
	Parent->NotifyChildrenVisualStateChanged();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::BringToBack()
{
	// to most back position
	if(!Parent) TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);

	Parent->SetAbsoluteOrderMode(false);

	SetOrderIndex(0);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::BringToFront()
{
	// to most front position
	if(!Parent) TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);

	Parent->SetAbsoluteOrderMode(false);

	SetOrderIndex(Parent->Children.GetActualCount() - 1);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetAbsoluteOrderIndex()
{
	// retrieve order index in absolute position
	if(!Parent) return 0;
	if(Parent->AbsoluteOrderMode)
		return AbsoluteOrderIndex;
	return GetOrderIndex();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetAbsoluteOrderIndex(tjs_int index)
{
	if(!Parent) TVPThrowExceptionMessage(TVPCannotMovePrimaryOrSiblingless);

	Parent->SetAbsoluteOrderMode(true);

	Parent->ChildChangeAbsoluteOrder(GetOrderIndex(), index);

	AbsoluteOrderIndex = index;
	Parent->NotifyChildrenVisualStateChanged();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetAbsoluteOrderMode(bool b)
{
	// set absolute order index mode
	if(AbsoluteOrderMode != b)
	{
		AbsoluteOrderMode = b;
		if(b)
		{
			// to absolute order mode
			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
				child->AbsoluteOrderIndex = child->GetOrderIndex();
			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
		}
		else
		{
			// to relative order mode

			// nothing to do
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DumpStructure(int level)
{
	tjs_char *indent = new tjs_char[level*2+1];
	try
	{
		for(tjs_int i =0; i<level*2; i++) indent[i] = TJS_W(' ');
		indent[level*2] = 0;

		ttstr name = Name;
		if(name.IsEmpty()) name = TJS_W("<noname>");
		tjs_char ptr[80];
		TJS_snprintf(ptr, sizeof(ptr)/sizeof(tjs_char), TJS_W(" (object 0x%p)"), Owner);
		tjs_char ptr2[80];
		TJS_snprintf(ptr2, sizeof(ptr2)/sizeof(tjs_char), TJS_W(" (native 0x%p)"), this);

		TVPAddLog(ttstr(indent) + name +
			ttstr(ptr) + ttstr(ptr2) +
			ttstr(TJS_W(" (")) + ttstr(Rect.left) + ttstr(TJS_W(",")) +
			ttstr(Rect.top) + ttstr(TJS_W(")-(")) + ttstr(Rect.right) +
			ttstr(TJS_W(",")) + ttstr(Rect.bottom) + ttstr(TJS_W(") (")) +
			ttstr(Rect.get_width()) + ttstr(TJS_W("x"))+ ttstr(Rect.get_height()) +
			ttstr(TJS_W(")")) + ttstr(TJS_W(" ")) +
				ttstr(GetVisible()?TJS_W("visible"):TJS_W("invisible")) +
				TJS_W(" index=") + ttstr(GetAbsoluteOrderIndex()) +
				ttstr(ProvinceImage?TJS_W(" p"):TJS_W("")) +
				TJS_W(" ") + ttstr(GetTypeNameString()));
	}
	catch(...)
	{
		delete [] indent;
		throw;
	}

	delete [] indent;

	level++;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		child->DumpStructure(level);
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
}
//---------------------------------------------------------------------------










//---------------------------------------------------------------------------
// layer type management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::NotifyLayerTypeChange()
{
	UpdateDrawFace();

	if(Parent) Parent->NotifyLayerTypeChange();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UpdateDrawFace()
{
	// set DrawFace from Face and Type
	if(Face == dfAuto)
	{
		// DrawFace is chosen automatically from the layer type
		switch(DisplayType)
		{
	//	case ltBinder:
		case ltOpaque:				DrawFace = dfOpaque;			break;
		case ltAlpha:				DrawFace = dfAlpha;				break;
		case ltAdditive:			DrawFace = dfOpaque;			break;
		case ltSubtractive:			DrawFace = dfOpaque;			break;
		case ltMultiplicative:		DrawFace = dfOpaque;			break;
	//	case ltEffect:
	//	case ltFilter:
		case ltDodge:				DrawFace = dfOpaque;			break;
		case ltDarken:				DrawFace = dfOpaque;			break;
		case ltLighten:				DrawFace = dfOpaque;			break;
		case ltScreen:				DrawFace = dfOpaque;			break;
		case ltAddAlpha:			DrawFace = dfAddAlpha;			break;
		case ltPsNormal:			DrawFace = dfAlpha;				break;
		case ltPsAdditive:			DrawFace = dfAlpha;				break;
		case ltPsSubtractive:		DrawFace = dfAlpha;				break;
		case ltPsMultiplicative:	DrawFace = dfAlpha;				break;
		case ltPsScreen:			DrawFace = dfAlpha;				break;
		case ltPsOverlay:			DrawFace = dfAlpha;				break;
		case ltPsHardLight:			DrawFace = dfAlpha;				break;
		case ltPsSoftLight:			DrawFace = dfAlpha;				break;
		case ltPsColorDodge:		DrawFace = dfAlpha;				break;
		case ltPsColorDodge5:		DrawFace = dfAlpha;				break;
		case ltPsColorBurn:			DrawFace = dfAlpha;				break;
		case ltPsLighten:			DrawFace = dfAlpha;				break;
		case ltPsDarken:			DrawFace = dfAlpha;				break;
		case ltPsDifference:	 	DrawFace = dfAlpha;				break;
		case ltPsDifference5:	 	DrawFace = dfAlpha;				break;
		case ltPsExclusion:			DrawFace = dfAlpha;				break;
		default:
							DrawFace = dfOpaque;			break;
		}
	}
	else
	{
		DrawFace = Face;
	}
}
//---------------------------------------------------------------------------
tTVPBlendOperationMode tTJSNI_BaseLayer::GetOperationModeFromType() const
{
	// returns corresponding blend operation mode from layer type

	switch(DisplayType)
	{
//	case ltBinder:
	case ltOpaque:			return omOpaque;			 
	case ltAlpha:			return omAlpha;
	case ltAdditive:		return omAdditive;
	case ltSubtractive:		return omSubtractive;
	case ltMultiplicative:	return omMultiplicative;	 
//	case ltEffect:
//	case ltFilter:
	case ltDodge:			return omDodge;
	case ltDarken:			return omDarken;
	case ltLighten:			return omLighten;
	case ltScreen:			return omScreen;
	case ltAddAlpha:		return omAddAlpha;
	case ltPsNormal:		return omPsNormal;
	case ltPsAdditive:		return omPsAdditive;
	case ltPsSubtractive:	return omPsSubtractive;
	case ltPsMultiplicative:return omPsMultiplicative;
	case ltPsScreen:		return omPsScreen;
	case ltPsOverlay:		return omPsOverlay;
	case ltPsHardLight:		return omPsHardLight;
	case ltPsSoftLight:		return omPsSoftLight;
	case ltPsColorDodge:	return omPsColorDodge;
	case ltPsColorDodge5:	return omPsColorDodge5;
	case ltPsColorBurn:		return omPsColorBurn;
	case ltPsLighten:		return omPsLighten;
	case ltPsDarken:		return omPsDarken;
	case ltPsDifference:	return omPsDifference;
	case ltPsDifference5:	return omPsDifference5;
	case ltPsExclusion:		return omPsExclusion;


	default:
							return omOpaque;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetType(tTVPLayerType type)
{
	// set layer type to "type"
	if(Type != type)
	{
		Type = type;
		switch(Type)
		{
		case ltBinder:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = false;
			DeallocateImage();
			break;

		case ltOpaque: // formerly ltCoverRect
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltAlpha: // formerly ltTransparent
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltAdditive:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltSubtractive:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltMultiplicative:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltEffect:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = ltBinder;  // TODO: retrieve actual DrawType
			CanHaveImage = false;
			DeallocateImage();
			break;

		case ltFilter:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = ltBinder;  // TODO: retrieve actual DisplayType
			CanHaveImage = false;
			DeallocateImage();
			break;

		case ltDodge:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltDarken:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltLighten:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltScreen:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltAddAlpha:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsNormal:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsAdditive:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsSubtractive:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsMultiplicative:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsScreen:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsOverlay:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(128, 128, 128, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsHardLight:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(128, 128, 128, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsSoftLight:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(128, 128, 128, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsColorDodge:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsColorDodge5:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsColorBurn:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsLighten:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsDarken:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(255, 255, 255, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsDifference:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsDifference5:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;

		case ltPsExclusion:
			NeutralColor = TransparentColor = TVP_RGBA2COLOR(0, 0, 0, 0);
			DisplayType = Type;
			CanHaveImage = true;
			AllocateImage();
			break;


		}
		NotifyLayerTypeChange();
		SetToCreateExposedRegion();
		Update();
	}
}
//---------------------------------------------------------------------------
const tjs_char * tTJSNI_BaseLayer::GetTypeNameString()
{
	switch(Type)
	{
	case ltBinder:			return TJS_W("ltBinder");
	case ltOpaque:			return TJS_W("ltOpaque");
	case ltAlpha:			return TJS_W("ltAlpha");
	case ltAdditive:		return TJS_W("ltAdditive");
	case ltSubtractive:		return TJS_W("ltSubtractive");
	case ltMultiplicative:	return TJS_W("ltMultiplicative");
	case ltEffect:			return TJS_W("ltEffect");
	case ltFilter:			return TJS_W("ltFilter");
	case ltDodge:			return TJS_W("ltDodge");
	case ltDarken:			return TJS_W("ltDarken");
	case ltLighten:			return TJS_W("ltLighten");
	case ltScreen:			return TJS_W("ltScreen");
	case ltAddAlpha:		return TJS_W("ltAddAlpha");
	case ltPsNormal:		return TJS_W("PsNormal");
	case ltPsAdditive:		return TJS_W("PsAdditive");
	case ltPsSubtractive:	return TJS_W("PsSubtractive");
	case ltPsMultiplicative:return TJS_W("PsMultiplicative");
	case ltPsScreen:		return TJS_W("PsScreen");
	case ltPsOverlay:		return TJS_W("PsOverlay");
	case ltPsHardLight:		return TJS_W("PsHardLight");
	case ltPsSoftLight:		return TJS_W("PsSoftLight");
	case ltPsColorDodge:	return TJS_W("PsColorDodge");
	case ltPsColorDodge5:	return TJS_W("PsColorDodge5");
	case ltPsColorBurn:		return TJS_W("PsColorBurn");
	case ltPsLighten:		return TJS_W("PsLighten");
	case ltPsDarken:		return TJS_W("PsDarken");
	case ltPsDifference:	return TJS_W("PsDifference");
	case ltPsDifference5:	return TJS_W("PsDifference5");
	case ltPsExclusion:		return TJS_W("PsExclusion");

	default:				return TJS_W("unknown");
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ConvertLayerType(tTVPDrawFace fromtype)
{
	// convert layer pixel representation method

	if(DrawFace == dfAddAlpha && fromtype == dfAlpha)
	{
		// alpha -> additive alpha
		if(MainImage) MainImage->ConvertAlphaToAddAlpha();
	}
	else if(DrawFace == dfAlpha && fromtype == dfAddAlpha)
	{
		// additive alpha -> alpha
		// this may loose additive stuff
		if(MainImage) MainImage->ConvertAddAlphaToAlpha();
	}
	else
	{
		// throw an error
		TVPThrowExceptionMessage(TVPCannotConvertLayerTypeUsingGivenDirection);
	}

	ImageModified = true;

	Update();
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// geographical management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CreateExposedRegion()
{
	// create exposed/overlapped region information

	// find region which is not piled by any children
	ExposedRegion.Clear();
	OverlappedRegion.Clear();

	tTVPRect rect;
	rect.left = rect.top = 0;
	rect.right = Rect.get_width();
	rect.bottom = Rect.get_height();

	if(MainImage != NULL)
	{
		// the layer has image

		if(GetVisibleChildrenCount() > TVP_EXPOSED_UNITE_LIMIT)
		{
			ExposedRegion.Or(rect);

			bool first = true;

			tTVPRect r2;

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)

				if(child->IsSeen())
				{
					tTVPRect r(child->GetRect());
					if(TVPIntersectRect(&r, r, rect))
					{
						if(first)
						{
							r2 = child->GetRect();
							first = false;
						}
						else
						{
							TVPUnionRect(&r2, r2, r);
						}
					}
				}

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

			OverlappedRegion.Or(r2);

			ExposedRegion.Sub(OverlappedRegion);
		}
		else
		{
			tTVPRect rect;
			rect.left = rect.top = 0;
			rect.right = Rect.get_width();
			rect.bottom = Rect.get_height();
			ExposedRegion.Or(rect);

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)

				if(child->IsSeen())
				{
					tTVPRect r(child->GetRect());
					if(TVPIntersectRect(&r, r, rect))
						OverlappedRegion.Or(r);
				}

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

			ExposedRegion.Sub(OverlappedRegion);
		}
	}
	else
	{
		// the layer has no image
		// ExposedRegion : child layer can directly transfer the image to the parent's target
		// OverlappedRegion : Inverse of ExposedRegion

		ExposedRegion.Clear();
		OverlappedRegion.Clear();
		OverlappedRegion.Or(rect);

		// ExposedRegion is a region with is only one child layer piled
		// under the parent layer.
		// Recalculating this is pretty high-cost operation, 
		if(GetVisibleChildrenCount() < TVP_DIRECT_UNITE_LIMIT)
		{
			tTVPComplexRect & one = ExposedRegion; // alias of ExposedRegion
			tTVPComplexRect   two; // region which is more than two layers piled

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)

				if(child->IsSeen())
				{
					tTVPRect r(child->GetRect());
					if(
						child->DisplayType == this->DisplayType &&
						child->Opacity == 255)
					{
						tTVPComplexRect one_and_r(one);
						one_and_r.And(r);
						tTVPComplexRect two_and_r(two);
						two_and_r.And(r);
						one.Sub(one_and_r);
						two.Or(one_and_r);
						two.Or(two_and_r);
						tTVPComplexRect tmp; tmp.Or(r);
						tmp.Sub(one_and_r);
						tmp.Sub(two_and_r);
						one.Or(tmp);
					}
					else
					{
						two.Or(r);
						one.Sub(r);
					}
				}

			TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
		}

		OverlappedRegion.Sub(ExposedRegion);
	}

	ExposedRegionValid = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalSetSize(tjs_uint width, tjs_uint height)
{
	if(Rect.get_width() != (tjs_int)width ||
		Rect.get_height() != (tjs_int)height)
	{
		Update(false);
		Rect.set_width(width);
		Rect.set_height(height);
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		SetToCreateExposedRegion();
		ImageLayerSizeChanged();
		Update(false);
		if(IsPrimary() && Manager) Manager->NotifyLayerResize();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalSetBounds(const tTVPRect &rect)
{
	tjs_int width = rect.right - rect.left;
	tjs_int height = rect.bottom - rect.top;
	if(width < 0 || height < 0) TVPThrowExceptionMessage(TVPInvalidParam);

	if(Rect.left != rect.left || Rect.top != rect.top)
	{
		bool visible = GetVisible() || GetNodeVisible();
		if(IsPrimary() && (rect.left != 0 || rect.top != 0))
			TVPThrowExceptionMessage(TVPCannotMovePrimary);

		if(visible) ParentUpdate();
		Rect.set_offsets(rect.left, rect.top);
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		SetToCreateExposedRegion();
		if(visible) ParentUpdate();
	}

	InternalSetSize(width, height);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetLeft(tjs_int left)
{
	if(Rect.left != left)
	{
		bool visible = GetVisible() || GetNodeVisible();
		if(IsPrimary() && left != 0)
			TVPThrowExceptionMessage(TVPCannotMovePrimary);
		if(visible) ParentUpdate();
		tjs_int w;
		w = Rect.get_width();
		Rect.left = left;
		Rect.right = w + Rect.left;
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		// TODO: SetLeft
		if(visible) ParentUpdate();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetTop(tjs_int top)
{
	if(Rect.top != top)
	{
		bool visible = GetVisible() || GetNodeVisible();
		if(IsPrimary() && top != 0)
			TVPThrowExceptionMessage(TVPCannotMovePrimary);
		if(visible) ParentUpdate();
		tjs_int h;
		h = Rect.get_height();
		Rect.top = top;
		Rect.bottom = h + Rect.top;
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		// TODO: SetTop;
		if(visible) ParentUpdate();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetPosition(tjs_int left, tjs_int top)
{
	if(Rect.left != left || Rect.top != top)
	{
		bool visible = GetVisible() || GetNodeVisible();
		if(IsPrimary() && (left != 0 || top != 0))
			TVPThrowExceptionMessage(TVPCannotMovePrimary);
		if(visible) ParentUpdate();
		Rect.set_offsets(left, top);
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		// TODO: SetPosition
		if(visible) ParentUpdate();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetWidth(tjs_uint width)
{
	if(Rect.get_width() != (tjs_int)width)
	{
		Update(false);
		Rect.set_width(width);
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		SetToCreateExposedRegion();
		ImageLayerSizeChanged();
		Update(false);
		if(IsPrimary() && Manager) Manager->NotifyLayerResize();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetHeight(tjs_uint height)
{
	if(Rect.get_height() != (tjs_int)height)
	{
		Update(false);
		Rect.set_height(height);
		if(Parent) Parent->NotifyChildrenVisualStateChanged();
		SetToCreateExposedRegion();
		ImageLayerSizeChanged();
		Update(false);
		if(IsPrimary() && Manager) Manager->NotifyLayerResize();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetSize(tjs_uint width, tjs_uint height)
{
	InternalSetSize(width, height);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetBounds(const tTVPRect &rect)
{
	// TODO: SetBounds
	InternalSetBounds(rect);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ParentRectToChildRect(tTVPRect &rect)
{
	// note that this function does not convert transformed layer coordinates.
	rect.left -= Rect.left;
	rect.right -= Rect.left;
	rect.top -= Rect.top;
	rect.bottom -= Rect.top;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ToPrimaryCoordinates(tjs_int &x, tjs_int &y) const
{
	const tTJSNI_BaseLayer *l = this;

	while(l && !l->IsPrimary())
	{
		x += l->Rect.left; y += l->Rect.top;
		l = l->Parent;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FromPrimaryCoordinates(tjs_int &x, tjs_int &y) const
{
	const tTJSNI_BaseLayer *l = this;

	while(l && !l->IsPrimary())
	{
		x -= l->Rect.left; y -= l->Rect.top;
		l = l->Parent;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FromPrimaryCoordinates(tjs_real &x, tjs_real &y) const
{
	const tTJSNI_BaseLayer *l = this;

	while(l && !l->IsPrimary())
	{
		x -= (tjs_real)l->Rect.left; y -= (tjs_real)l->Rect.top;
		l = l->Parent;
	}
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// image buffer management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ChangeImageSize(tjs_uint width, tjs_uint height)
{
	// be called from geographical management
	if(!width || !height)
		TVPThrowExceptionMessage(TVPCannotCreateEmptyLayerImage);

	if(MainImage) MainImage->SetSizeWithFill(width, height, NeutralColor);
	if(ProvinceImage) ProvinceImage->SetSizeWithFill(width, height, 0);

	if(MainImage) ResetClip();  // cliprect is reset

	ImageModified = true;

	ResizeCache(); // in cache management
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AllocateImage()
{
	if(!MainImage)
	{
		ImageLeft = 0;
		ImageTop = 0;
        MainImage = new tTVPBaseTexture(Rect.get_width(), Rect.get_height());
		MainImage->Fill(tTVPRect(0, 0, Rect.get_width(), Rect.get_height()), NeutralColor);
		MainImage->SetFont(Font); // set font
	}

	if(MainImage) ResetClip();  // cliprect is reset

	if(ProvinceImage)
	{
		ProvinceImage->SetSizeWithFill(MainImage->GetWidth(),
			MainImage->GetHeight(), 0);
	}

	FontChanged = true; // invalidate font assignment cache
	ImageModified = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DeallocateImage()
{
	if(MainImage) delete MainImage, MainImage = NULL;
	if(ProvinceImage) delete ProvinceImage, ProvinceImage = NULL;

	ImageModified = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AllocateProvinceImage()
{
	tjs_uint neww = MainImage?MainImage->GetWidth():Rect.get_width();
	tjs_uint newh = MainImage?MainImage->GetHeight():Rect.get_height();

	if(!ProvinceImage)
	{
		ProvinceImage = new tTVPBaseBitmap(neww, newh, 8);
		ProvinceImage->Fill(tTVPRect(0, 0, neww, newh), 0);
	}
	else
	{
		ProvinceImage->SetSizeWithFill(neww, newh, 0);
	}

	ImageModified = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DeallocateProvinceImage()
{
	if(ProvinceImage) delete ProvinceImage, ProvinceImage = NULL;
	ImageModified = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AllocateDefaultImage()
{
	if(!MainImage)
		MainImage = new tTVPBaseTexture(*TVPTempBitmapHolder->Get());
	else
		MainImage->Assign(*TVPTempBitmapHolder->Get());

	FontChanged = true; // invalidate font assignment cache
	ResetClip();  // cliprect is reset

	ImageModified = true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AssignImages(tTJSNI_BaseLayer *src)
{
	// assign images
	bool main_changed = true;

	if(src->MainImage)
	{
		if(MainImage)
			main_changed = MainImage->Assign(* src->MainImage);
		else
			MainImage = new tTVPBaseTexture(* src->MainImage);
		FontChanged = true; // invalidate font assignment cache
	}
	else
	{
		DeallocateImage();
	}

	if(src->ProvinceImage)
	{
		if(ProvinceImage)
			ProvinceImage->Assign(* src->ProvinceImage);
		else
			ProvinceImage = new tTVPBaseBitmap(* src->ProvinceImage);
	}
	else
	{
		DeallocateProvinceImage();
	}

	if(main_changed && MainImage)
	{
		InternalSetImageSize(MainImage->GetWidth(), MainImage->GetHeight());
			// adjust position
	}

	ImageModified = true;

	if(MainImage) ResetClip();  // cliprect is reset

	if(main_changed) Update(false); // update
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AssignMainImageWithUpdate(iTVPBaseBitmap *bmp)
{
	// assign images
	bool main_changed = true;

	if(bmp)
	{
		if(MainImage)
			main_changed = MainImage->Assign(*bmp);
		else
			MainImage = new tTVPBaseTexture(*bmp);
		FontChanged = true; // invalidate font assignment cache
	}
	else
	{
		DeallocateImage();
	}

	if(main_changed && MainImage)
	{
		InternalSetImageSize(MainImage->GetWidth(), MainImage->GetHeight());
			// adjust position
	}

	ImageModified = true;

	if(MainImage) ResetClip();  // cliprect is reset

	if(main_changed) Update(false); // update
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AssignMainImage(iTVPBaseBitmap *bmp)
{
	// assign single main bitmap image. the image size assigned must be
	// identical to the destination layer bitmap.
	// destination bitmap must have a layer bitmap

	if(!MainImage ||
		bmp->GetWidth() != MainImage->GetWidth() ||
		bmp->GetHeight() != MainImage->GetHeight() )
	{
		// destination layer does not have a main image or
		// the size is not identical to the source layer bitmap
		TVPThrowInternalError;
	}

	bool main_changed = MainImage->Assign(*bmp);

	if(main_changed)
	{
		ImageModified = true;
		Update(false); // update
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CopyFromMainImage( tTJSNI_Bitmap* bmp )
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	bmp->CopyFrom( MainImage );
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetHasImage(bool b)
{
	if(!CanHaveImage && b)
		TVPThrowExceptionMessage(TVPLayerCannotHaveImage);
	if(b) AllocateImage(); else DeallocateImage();
	NotifyChildrenVisualStateChanged();
	Update();
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetHasImage() const
{
	return MainImage != NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImageLeft(tjs_int left)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(ImageLeft != left)
	{
		if(left > 0) TVPThrowExceptionMessage(TVPInvalidImagePosition);
		if((tjs_int)(MainImage->GetWidth()) + left < Rect.get_width())
			TVPThrowExceptionMessage(TVPInvalidImagePosition);
		ImageLeft = left;
		Update();
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetImageLeft() const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	return ImageLeft;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImageTop(tjs_int top)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(ImageTop != top)
	{
		if(top > 0) TVPThrowExceptionMessage(TVPInvalidImagePosition);
		if((tjs_int)(MainImage->GetHeight()) + top < Rect.get_height())
			TVPThrowExceptionMessage(TVPInvalidImagePosition);
		ImageTop = top;
		Update();
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetImageTop() const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	return ImageTop;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImagePosition(tjs_int left, tjs_int top)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(ImageLeft != left || ImageTop != top)
	{
		if(left > 0) TVPThrowExceptionMessage(TVPInvalidImagePosition);
		if(top > 0) TVPThrowExceptionMessage(TVPInvalidImagePosition);
		if((tjs_int)(MainImage->GetWidth()) + left < Rect.get_width())
			TVPThrowExceptionMessage(TVPInvalidImagePosition);
		if((tjs_int)(MainImage->GetHeight()) + top < Rect.get_height())
			TVPThrowExceptionMessage(TVPInvalidImagePosition);
		ImageLeft = left;
		ImageTop = top;
		Update();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImageWidth(tjs_uint width)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(width == MainImage->GetWidth()) return;

	// adjust position
	if((tjs_int)width < Rect.get_width())
	{
		ImageLeft = 0;
		SetWidth(width); // change layer size
	}

	if((tjs_int)(width) + ImageLeft < Rect.get_width())
	{
		ImageLeft = Rect.get_width() - width;
	}

	// change image size...
	ChangeImageSize(width, MainImage->GetHeight());
}
//---------------------------------------------------------------------------
tjs_uint tTJSNI_BaseLayer::GetImageWidth() const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	return MainImage->GetWidth();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImageHeight(tjs_uint height)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(height == MainImage->GetHeight()) return;

	// adjust position
	if((tjs_int)height < Rect.get_height())
	{
		ImageTop = 0;
		SetHeight(height); // change layer size
	}

	if((tjs_int)(height + ImageTop) < Rect.get_height())
	{
		ImageTop = Rect.get_height() - height;
	}

	// change image size...
	ChangeImageSize(MainImage->GetWidth(), height);
}
//---------------------------------------------------------------------------
tjs_uint tTJSNI_BaseLayer::GetImageHeight() const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	return MainImage->GetHeight();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalSetImageSize(tjs_uint width, tjs_uint height)
{
	// adjust position
	if((tjs_int)width < Rect.get_width())
	{
		ImageLeft = 0;
		SetWidth(width); // change layer size
	}
	if((tjs_int)(width + ImageLeft) < Rect.get_width())
	{
		ImageLeft = Rect.get_width() - width;
	}

	if((tjs_int)height < Rect.get_height())
	{
		ImageTop = 0;
		SetHeight(height); // change layer size
	}
	if((tjs_int)(height + ImageTop) < Rect.get_height())
	{
		ImageTop = Rect.get_height() - height;
	}

	ChangeImageSize(width, height);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImageSize(tjs_uint width, tjs_uint height)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(width == MainImage->GetWidth() && height == MainImage->GetHeight()) return;

	InternalSetImageSize(width, height);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ImageLayerSizeChanged()
{
	// called from geographical management
	if(!MainImage) return;

	if((tjs_int)MainImage->GetWidth() < Rect.get_width())
	{
		ChangeImageSize(Rect.get_width(), MainImage->GetHeight());
	}
	if((tjs_int)(MainImage->GetWidth() + ImageLeft) < Rect.get_width())
	{
		ImageLeft = Rect.get_width() - MainImage->GetWidth();
		Update();
	}

	if((tjs_int)MainImage->GetHeight() < Rect.get_height())
	{
		ChangeImageSize(MainImage->GetWidth(), Rect.get_height());
	}
	if((tjs_int)(MainImage->GetHeight() + ImageTop) < Rect.get_height())
	{
		ImageTop = Rect.get_height() - MainImage->GetHeight();
		Update();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::IndependMainImage(bool copy)
{
	if(MainImage)
	{
		if(copy)
			MainImage->Independ();
		else
			MainImage->IndependNoCopy();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::IndependProvinceImage(bool copy)
{
	if(ProvinceImage)
	{
		if(copy)
			ProvinceImage->Independ();
		else
			ProvinceImage->IndependNoCopy();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SaveLayerImage(const ttstr &name, const ttstr &type)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	
	iTJSDispatch2 *dic = TJSCreateDictionaryObject();
	try {
		tTJSVariant val;
		switch(Type) {
		case ltOpaque:				val = tTJSVariant(TJS_W("opaque"));		break;
		case ltAlpha:				val = tTJSVariant(TJS_W("alpha"));		break;
		case ltAdditive:			val = tTJSVariant(TJS_W("add"));		break;
		case ltSubtractive:			val = tTJSVariant(TJS_W("sub"));		break;
		case ltMultiplicative:		val = tTJSVariant(TJS_W("mul"));		break;
		case ltDodge:				val = tTJSVariant(TJS_W("dodge"));		break;
		case ltDarken:				val = tTJSVariant(TJS_W("darken"));		break;
		case ltLighten:				val = tTJSVariant(TJS_W("lighten"));	break;
		case ltScreen:				val = tTJSVariant(TJS_W("screen"));		break;
		case ltAddAlpha:			val = tTJSVariant(TJS_W("addalpha"));	break;
		case ltPsNormal:			val = tTJSVariant(TJS_W("psnormal"));	break;
		case ltPsAdditive:			val = tTJSVariant(TJS_W("psadd"));		break;
		case ltPsSubtractive:		val = tTJSVariant(TJS_W("pssub"));		break;
		case ltPsMultiplicative:	val = tTJSVariant(TJS_W("psmul"));		break;
		case ltPsScreen:			val = tTJSVariant(TJS_W("psscreen"));	break;
		case ltPsOverlay:			val = tTJSVariant(TJS_W("psoverlay"));	break;
		case ltPsHardLight:			val = tTJSVariant(TJS_W("pshlight"));	break;
		case ltPsSoftLight:			val = tTJSVariant(TJS_W("psslight"));	break;
		case ltPsColorDodge:		val = tTJSVariant(TJS_W("psdodge"));	break;
		case ltPsColorDodge5:		val = tTJSVariant(TJS_W("psdodge5"));	break;
		case ltPsColorBurn:			val = tTJSVariant(TJS_W("psburn"));		break;
		case ltPsLighten:			val = tTJSVariant(TJS_W("pslighten"));	break;
		case ltPsDarken:			val = tTJSVariant(TJS_W("psdarken"));	break;
		case ltPsDifference:	 	val = tTJSVariant(TJS_W("psdiff"));		break;
		case ltPsDifference5:	 	val = tTJSVariant(TJS_W("psdiff5"));	break;
		case ltPsExclusion:			val = tTJSVariant(TJS_W("psexcl"));		break;
		default:					val = tTJSVariant(TJS_W("opaque"));		break;
		}
		dic->PropSet(TJS_MEMBERENSURE, TJS_W("mode"), 0, &val, dic );

		if( ImageLeft > 0 ) {
			val = tTJSVariant(ImageLeft);
			dic->PropSet(TJS_MEMBERENSURE, TJS_W("offs_x"), 0, &val, dic );
		}
		if( ImageTop > 0 ) {
			val = tTJSVariant(ImageTop);
			dic->PropSet(TJS_MEMBERENSURE, TJS_W("offs_y"), 0, &val, dic );
		}
		if( ImageLeft > 0 || ImageTop > 0 ) {
			val = tTJSVariant(TJS_W("pixel"));
			dic->PropSet(TJS_MEMBERENSURE, TJS_W("offs_unit"), 0, &val, dic );
		}
		TVPSaveImage( name, type, MainImage, dic );
	} catch(...) {
		dic->Release();
		throw;
	}
	dic->Release();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AssignTexture(iTVPTexture2D *tex)
{
	if (!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	MainImage->AssignTexture(tex);
	InternalSetImageSize(MainImage->GetWidth(), MainImage->GetHeight());
	ImageModified = true;
	ResetClip();  // cliprect is reset
	Update(false);
}

//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseLayer::LoadImages(const ttstr &name, tjs_uint32 colorkey)
{
	// loads image(s) from specified storage.
	// colorkey must be a color that should be transparent, or:
	// 0x 01 ff ff ff (clAdapt) : the color key will be automatically chosen from
	//                            target image, by selecting most used color from
	//                            the top line of the image.
	// 0x 1f ff ff ff (clNone)  : does not apply the colorkey, or uses image alpha
	//                            channel.
	// 0x30000000 (clPalIdx) + nn ( nn = palette index )
	//                          : select the color key by specified palette index.
	// 0x40000000 (TVP_clAlphaMat) + 0xRRGGBB ( 0xRRGGBB = matting color )
	//                          : do matting with the color using alpha blending.
	// returns graphic image metainfo.

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	ttstr provincename;
	iTJSDispatch2 * metainfo = NULL;

	TVPLoadGraphic(MainImage, name, colorkey, 0, 0, glmNormal, &provincename, &metainfo);
	try
	{

		InternalSetImageSize(MainImage->GetWidth(), MainImage->GetHeight());

		if(!provincename.IsEmpty())
		{
			// province image exists
			AllocateProvinceImage();

			try
			{
				TVPLoadGraphicProvince(ProvinceImage, provincename, 0,
					MainImage->GetWidth(), MainImage->GetHeight());

				if(ProvinceImage->GetWidth() != MainImage->GetWidth() ||
					ProvinceImage->GetHeight() != MainImage->GetHeight())
					TVPThrowExceptionMessage(TVPProvinceSizeMismatch, provincename);
			}
			catch(...)
			{
				DeallocateProvinceImage();
				throw;
			}
		}
		else
		{
			// province image does not exist
			DeallocateProvinceImage();
		}

		ImageModified = true;

		ResetClip();  // cliprect is reset

		Update(false);
	}
	catch(...)
	{
		if(metainfo) metainfo->Release();
		throw;
	}

	return metainfo;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::LoadProvinceImage(const ttstr &name)
{
	// load an image as a province image
	
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	AllocateProvinceImage();

	try
	{
		TVPLoadGraphicProvince(ProvinceImage, name, 0,
			MainImage->GetWidth(), MainImage->GetHeight());

		if(ProvinceImage->GetWidth() != MainImage->GetWidth() ||
			ProvinceImage->GetHeight() != MainImage->GetHeight())
			TVPThrowExceptionMessage(TVPProvinceSizeMismatch, name);
	}
	catch(...)
	{
		DeallocateProvinceImage();
		throw;
	}

	ImageModified = true;
}
//---------------------------------------------------------------------------
tjs_uint32 tTJSNI_BaseLayer::GetMainPixel(tjs_int x, tjs_int y) const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	return TVPFromActualColor(MainImage->GetPoint(x, y) & 0xffffff);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetMainPixel(tjs_int x, tjs_int y, tjs_uint32 color)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(x < ClipRect.left || y < ClipRect.top ||
		x >= ClipRect.right || y >= ClipRect.bottom) return; // out of clipping rectangle

	MainImage->SetPointMain(x, y, TVPToActualColor(color));

	ImageModified = true;
	tTVPRect r;
	r.left = ImageLeft + x;
	r.top = ImageTop + y;
	r.right = r.left + 1;
	r.bottom = r.top + 1;
	Update(r);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetMaskPixel(tjs_int x, tjs_int y) const
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	return (MainImage->GetPoint(x, y) & 0xff000000) >> 24;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetMaskPixel(tjs_int x, tjs_int y, tjs_int mask)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(x < ClipRect.left || y < ClipRect.top ||
		x >= ClipRect.right || y >= ClipRect.bottom) return; // out of clipping rectangle

	MainImage->SetPointMask(x, y, mask);

	ImageModified = true;
	tTVPRect r;
	r.left = ImageLeft + x;
	r.top = ImageTop + y;
	r.right = r.left + 1;
	r.bottom = r.top + 1;
	Update(r);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetProvincePixel(tjs_int x, tjs_int y) const
{
	if(!ProvinceImage) return 0;

	if(x < 0 || y < 0 || x >= (tjs_int)ProvinceImage->GetWidth() ||
		y >= (tjs_int)ProvinceImage->GetHeight()) return 0;

	return ProvinceImage->GetPoint(x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetProvincePixel(tjs_int x, tjs_int y, tjs_int n)
{
	if(!ProvinceImage) AllocateProvinceImage();

	if(x < ClipRect.left || y < ClipRect.top ||
		x >= ClipRect.right || y >= ClipRect.bottom) return; // out of clipping rectangle

	ProvinceImage->SetPoint(x, y, n);

	ImageModified = true;
	tTVPRect r;
	r.left = ImageLeft + x;
	r.top = ImageTop + y;
	r.right = r.left + 1;
	r.bottom = r.top + 1;
	Update(r);
}
//---------------------------------------------------------------------------
const void * tTJSNI_BaseLayer::GetMainImagePixelBuffer() const
{
	if(!MainImage) return NULL;
	return MainImage->GetScanLine(0);
}
//---------------------------------------------------------------------------
void * tTJSNI_BaseLayer::GetMainImagePixelBufferForWrite()
{
	if(!MainImage) return NULL;
	ImageModified = true;
	return MainImage->GetScanLineForWrite(0);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetMainImagePixelBufferPitch() const
{
	if(!MainImage) return 0;
	return MainImage->GetPitchBytes();
}
//---------------------------------------------------------------------------
const void * tTJSNI_BaseLayer::GetProvinceImagePixelBuffer() const
{
	if(!ProvinceImage) return NULL;
	return ProvinceImage->GetScanLine(0);
}
//---------------------------------------------------------------------------
void * tTJSNI_BaseLayer::GetProvinceImagePixelBufferForWrite()
{
	if(!ProvinceImage) AllocateProvinceImage();
	ImageModified = true;
	return ProvinceImage->GetScanLineForWrite(0);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetProvinceImagePixelBufferPitch() const
{
	if(!ProvinceImage) return 0;
	return ProvinceImage->GetPitchBytes();
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// input event / hit test management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCursorByStorage(const ttstr &storage)
{
	Cursor = TVPGetCursor(storage);
	if(Manager) Manager->NotifyMouseCursorChange(this, GetLayerActiveCursor());
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCursorByNumber(tjs_int num)
{
	Cursor = num;
	if(Manager) Manager->NotifyMouseCursorChange(this, GetLayerActiveCursor());
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetLayerActiveCursor()
{
	// return layer's actual (active) mouse cursor
	tjs_int cursor = Cursor;
	tTJSNI_BaseLayer *p = this;
	while(cursor == 0) // while cursor is 0 ( crDefault ) .. look up parent layer
	{
		p = p->Parent;
		if(!p) break;
		cursor = p->Cursor;
	}
	return cursor;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCurrentCursorToWindow(void)
{
	// set current layer cusor to the window
	if(Manager)
	{
		Manager->SetMouseCursor(GetLayerActiveCursor());
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetCursorX()
{
	tjs_int x, y;
	if(!Manager) return 0;
	Manager->GetCursorPos(x, y);

	tTJSNI_BaseLayer *p = this;
	while(p)
	{
		if(!p->Parent) break;
		x -= p->Rect.left;
		p = p->Parent;
	}

	return x;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCursorX(tjs_int x)
{
	CursorX_Work = x; // once store to this variable;
	// cursor moves on call to SetCursorY
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetCursorY()
{
	tjs_int x, y;
	if(!Manager) return 0;
	Manager->GetCursorPos(x, y);

	tTJSNI_BaseLayer *p = this;
	while(p)
	{
		if(!p->Parent) break;
		y -= p->Rect.top;
		p = p->Parent;
	}

	return y;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCursorY(tjs_int y)
{
	if(!Manager) return;

	tjs_int x = CursorX_Work;
	tTJSNI_BaseLayer *p = this;
	while(p)
	{
		if(!p->Parent) break;
		x += p->Rect.left;
		y += p->Rect.top;
		p = p->Parent;
	}

	Manager->SetCursorPos(x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCursorPos(tjs_int x, tjs_int y)
{
	if(!Manager) return;

	tTJSNI_BaseLayer *p = this;
	while(p)
	{
		if(!p->Parent) break;
		x += p->Rect.left;
		y += p->Rect.top;
		p = p->Parent;
	}

	Manager->SetCursorPos(x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCurrentHintToWindow()
{
	// set current hint to the window
	if( IgnoreHintSensing ) return;
	if(Manager)
	{
		tTJSNI_BaseLayer *p = this;
		while(p->ShowParentHint)
		{
			if(!p->Parent) break;
			p = p->Parent;
		}

		Manager->SetHint( GetOwnerNoAddRef(), p->Hint);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetHint(const ttstr & hint)
{
	ShowParentHint = false;
	IgnoreHintSensing = false;
	Hint = hint;
	if(Manager)
		Manager->NotifyHintChange(this, hint);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetAttentionLeft(tjs_int l)
{
	AttentionLeft = l;
	if(Manager) Manager->NotifyAttentionStateChanged(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetAttentionTop(tjs_int t)
{
	AttentionTop = t;
	if(Manager) Manager->NotifyAttentionStateChanged(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetAttentionPoint(tjs_int l, tjs_int t)
{
	AttentionLeft = l;
	AttentionTop = t;
	if(Manager) Manager->NotifyAttentionStateChanged(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetUseAttention(bool b)
{
	UseAttention = b;
	if(Manager) Manager->NotifyAttentionStateChanged(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetImeMode(tTVPImeMode mode)
{
	ImeMode = mode;
	if(Manager) Manager->NotifyImeModeChanged(this);
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::_HitTestNoVisibleCheck(tjs_int x, tjs_int y)
{
	// do hit test.
	// this function does not check layer's visiblity

	if(HitType == htMask)
	{
		// use mask
		if(MainImage)
		{
			tjs_int px = x-ImageLeft, py = y-ImageTop;
			if(px >= 0 && py >= 0 && px < (tjs_int)MainImage->GetWidth() &&
				py < (tjs_int)MainImage->GetHeight())
			{
                if (HitThreshold <= 0) return true;
				
				tjs_uint32 cl = MainImage->GetPoint(px, py);
				if((tjs_int)(cl >> 24) < HitThreshold)
					return false;
				else
					return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// layer has no image
			// all pixels are treated as 0 alpha value
			if(x >= 0 && y >= 0 && x < Rect.get_width() && y < Rect.get_height())
			{
				if(0 < HitThreshold) return false; else return true;
			}
			return false;
		}
	}
	else if(HitType == htProvince)
	{
		// use province

		if(ProvinceImage)
		{
			tjs_int px = x-ImageLeft, py = y-ImageTop;
			if(px >= 0 && py >= 0 && px < (tjs_int)ProvinceImage->GetWidth() &&
				py < (tjs_int)ProvinceImage->GetHeight())
			{
				tjs_uint32 cl = ProvinceImage->GetPoint(px, py);
				if(cl == 0) return false; else return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::HitTestNoVisibleCheck(tjs_int x, tjs_int y)
{
	bool res = _HitTestNoVisibleCheck(x, y);

	if(res)
	{
		// call onHitTest to perform additional hittest
		if(Owner && !Shutdown)
		{
			OnHitTest_Work = true;

			tTJSVariant param[3];
			param[0] = x;
			param[1] = y;
			param[2] = true;
			static ttstr eventname(TJS_W("onHitTest"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 3, param);

			res = OnHitTest_Work;
		}
		else
		{
			return res;
		}
	}

	return res;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetMostFrontChildAt(tjs_int x, tjs_int y, tTJSNI_BaseLayer **lay,
	const tTJSNI_BaseLayer *except, bool get_disabled)
{
	// internal function

	// visible check
	if(!Visible) return false; // cannot hit invisible layer

	// convert coordinates ( the point is given by parent's coordinates )
	x -= Rect.left; y -= Rect.top;

	// rectangle test
	if(x < 0 || y < 0 || x >= Rect.get_width() || y >= Rect.get_height())
		return false; // out of the rectangle

	tjs_int ox = x, oy = y;

	{ // locked
		tObjectListSafeLockHolder<tTJSNI_BaseLayer> holder(Children);
		tjs_int i = Children.GetSafeLockedObjectCount() - 1;
		for(; i>=0; i--)
		{
			tTJSNI_BaseLayer * child = Children.GetSafeLockedObjectAt(i);
			if(!child) continue;
			bool b = child->GetMostFrontChildAt(x, y, lay, except, get_disabled);
			if(b) return true;
		}
	} // end locked

	if(except == this) return false; // exclusion

	if(HitTestNoVisibleCheck(ox, oy))
	{
		if(!get_disabled && (!GetNodeEnabled() || IsDisabledByMode()))
		{
			*lay = NULL;
			return true; // cannot hit disabled or under modal layer
		}
		*lay = this;
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer * tTJSNI_BaseLayer::GetMostFrontChildAt(tjs_int x, tjs_int y,
	bool exclude_self, bool get_disabled)
{
	// get most front layer at (x, y),
	// excluding self layer if "exclude_self" is true.

	if(!Manager) return NULL;

	// convert to primary layer's coods
	tTJSNI_BaseLayer *p = this;
	while(p)
	{
		if(!p->Parent) break;
		x += p->Rect.left;
		y += p->Rect.top;
		p = p->Parent;
	}

	// call Manager->GetMostFrontChildAt
	return Manager->GetMostFrontChildAt(x, y, exclude_self ? this : NULL, get_disabled);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireClick(tjs_int x, tjs_int y)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[2];
		param[0] = x;
		param[1] = y;
		static ttstr eventname(TJS_W("onClick"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireDoubleClick(tjs_int x, tjs_int y)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[2];
		param[0] = x;
		param[1] = y;
		static ttstr eventname(TJS_W("onDoubleClick"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb,
	tjs_uint32 flags)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[4];
		param[0] = x;
		param[1] = y;
		param[2] = (tjs_int) mb;
		param[3] = (tjs_int64)flags;
		static ttstr eventname(TJS_W("onMouseDown"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 4, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb,
	tjs_uint32 flags)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[4];
		param[0] = x;
		param[1] = y;
		param[2] = (tjs_int) mb;
		param[3] = (tjs_int64)flags;
		static ttstr eventname(TJS_W("onMouseUp"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 4, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[3];
		param[0] = x;
		param[1] = y;
		param[2] = (tjs_int64)flags;
		static ttstr eventname(TJS_W("onMouseMove"));
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE|TVP_EPT_DISCARDABLE, 3, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseEnter()
{
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onMouseEnter"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseLeave()
{
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onMouseLeave"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ReleaseCapture()
{
	if(Manager) Manager->ReleaseCapture();
		// this releases mouse capture from all layers, ignoring which layer captures.
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ReleaseTouchCapture( tjs_uint32 id, bool all ) {
	if(Manager)
	{
		if( all )
		{
			Manager->ReleaseTouchCaptureAll();
		}
		else
		{
			Manager->ReleaseTouchCapture(id);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
{
	if(Owner && !Shutdown)
	{
		tTJSVariant arg[5] = { x, y, cx, cy, (tjs_int64)id };
		static ttstr eventname(TJS_W("onTouchDown"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 5, arg);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
{
	if(Owner && !Shutdown)
	{
		tTJSVariant arg[5] = { x, y, cx, cy, (tjs_int64)id };
		static ttstr eventname(TJS_W("onTouchUp"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 5, arg);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id )
{
	if(Owner && !Shutdown)
	{
		tTJSVariant arg[5] = { x, y, cx, cy, (tjs_int64)id };
		static ttstr eventname(TJS_W("onTouchMove"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE|TVP_EPT_DISCARDABLE, 5, arg);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag )
{
	if(Owner && !Shutdown)
	{
		tTJSVariant arg[5] = { startdist, curdist, cx, cy, flag };
		static ttstr eventname(TJS_W("onTouchScaling"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 5, arg);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag )
{
	if(Owner && !Shutdown)
	{
		tTJSVariant arg[6] = { startangle, curangle, dist, cx, cy, flag };
		static ttstr eventname(TJS_W("onTouchRotate"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 6, arg);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMultiTouch()
{
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onMultiTouch"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::ParentFocusable() const
{
	tTJSNI_BaseLayer *par = Parent;
	while(par)
	{
		if(!par->Visible || !par->Enabled) { return false; }
			// note that here we do not check parent's focusable state.
		par = par->Parent;
	}

	return true;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::SetFocus(bool direction)
{
	if(Manager) return Manager->SetFocusTo(this, direction);
	return false;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFocused()
{
	if(!Manager) return false;
	return Manager->GetFocusedLayer() == this;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTJSNI_BaseLayer::SearchFirstFocusable(bool ignore_chain_focusable)
{
	if(ignore_chain_focusable)
	{
		if(GetNodeFocusable()) return this;
	}
	else
	{
		if(GetNodeFocusable() && JoinFocusChain) return this;
	}

	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		tTJSNI_BaseLayer * lay = child->SearchFirstFocusable(ignore_chain_focusable);
		if(lay) return lay;
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END

	return NULL;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTJSNI_BaseLayer::_GetPrevFocusable()
{
	// search next focusable layer backward
	tTJSNI_BaseLayer *p = this;
	tTJSNI_BaseLayer *current = this;

	p = p->GetNeighborAbove(true);
	if(current == p) return NULL;
	if(!p) return NULL;
	current = p;
	do
	{
		if(p->GetNodeFocusable() && p->JoinFocusChain) return p; // next focusable layer found
	} while(p = p->GetNeighborAbove(true), p && p != current);

	return NULL; // no layer found
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTJSNI_BaseLayer::GetPrevFocusable()
{
	// search next focusable layer backward
	FocusWork = _GetPrevFocusable();
	if(Owner && !Shutdown &&
		(!FocusWork || FocusWork->Owner))
	{
		static ttstr eventname(TJS_W("onSearchPrevFocusable"));
		tTJSVariant param[1];
		if(FocusWork)
			param[0] = tTJSVariant(FocusWork->Owner,
				FocusWork->Owner);
		else
			param[0] = tTJSVariant((iTJSDispatch2*)NULL);
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE, 1, param);

	}
	return FocusWork;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTJSNI_BaseLayer::_GetNextFocusable()
{
	// search next focusable layer forward
	tTJSNI_BaseLayer *p = this;
	tTJSNI_BaseLayer *current = this;

	p = p->GetNeighborBelow(true);
	if(current == p) return NULL;
	if(!p) return NULL;
	current = p;
	do
	{
		if(p->GetNodeFocusable() && p->JoinFocusChain) return p; // next focusable layer found
	} while(p = p->GetNeighborBelow(true), p && p != current);

	return NULL; // no layer found
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *tTJSNI_BaseLayer::GetNextFocusable()
{
	// search next focusable layer forward
	FocusWork = _GetNextFocusable();
	if(Owner && !Shutdown &&
		(!FocusWork || FocusWork->Owner))
	{
		static ttstr eventname(TJS_W("onSearchNextFocusable"));
		tTJSVariant param[1];
		if(FocusWork)
			param[0] = tTJSVariant(FocusWork->Owner,
				FocusWork->Owner);
		else
			param[0] = tTJSVariant((iTJSDispatch2*)NULL);
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE, 1, param);

	}
	return FocusWork;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *  tTJSNI_BaseLayer::FocusPrev()
{
	if(Manager) return Manager->FocusPrev(); else return NULL;
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer *  tTJSNI_BaseLayer::FocusNext()
{
	if(Manager) return Manager->FocusNext(); else return NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFocusable(bool b)
{
	if(Focusable != b)
	{
		bool bstate = GetNodeFocusable();
		Focusable = b;
		bool astate = GetNodeFocusable();
		if(bstate != astate)
		{
			if(!astate)
			{
				// remove focus from this layer
				if(Manager)
				{
					if(Manager->GetFocusedLayer() == this)
						Manager->SetFocusTo(GetNextFocusable(), true); // blur
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireBlur(tTJSNI_BaseLayer *prevfocused)
{
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onBlur"));
		tTJSVariant param[1];
		if(prevfocused)
			param[0] = tTJSVariant(prevfocused->Owner, prevfocused->Owner);
		else
			param[0] = tTJSVariant((iTJSDispatch2*)NULL);
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE, 1, param);

	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireFocus(tTJSNI_BaseLayer *prevfocused, bool direction)
{
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onFocus"));
		tTJSVariant param[2];
		if(prevfocused)
			param[0] = tTJSVariant(prevfocused->Owner, prevfocused->Owner);
		else
			param[0] = tTJSVariant((iTJSDispatch2*)NULL);
		param[1] = direction;
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE, 2, param);
	}
}
//---------------------------------------------------------------------------
tTJSNI_BaseLayer * tTJSNI_BaseLayer::FireBeforeFocus(
	tTJSNI_BaseLayer *prevfocused, bool direction)
{
	FocusWork = this;
	if(Owner && !Shutdown)
	{
		static ttstr eventname(TJS_W("onBeforeFocus"));
		tTJSVariant param[3];
		param[0] = tTJSVariant(Owner, Owner);
		if(prevfocused)
			param[1] = tTJSVariant(prevfocused->Owner, prevfocused->Owner);
		else
			param[1] = tTJSVariant((iTJSDispatch2*)NULL);
		param[2] = direction;
		TVPPostEvent(Owner, Owner, eventname, 0,
			TVP_EPT_IMMEDIATE, 3, param);

	}
	return FocusWork;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetMode()
{
	if(Manager) Manager->SetModeTo(this);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::RemoveMode()
{
	if(Manager) Manager->RemoveModeFrom(this);
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::IsDisabledByMode()
{
	// is "this" layer disable by modal layer?
	if(!Manager) return false;
	tTJSNI_BaseLayer *current = Manager->GetCurrentModalLayer();
	if(!current) return false;
	return !IsAncestorOrSelf(current);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::NotifyNodeEnabledState()
{
	bool en = GetNodeEnabled();
	if(Owner && !Shutdown && EnabledWork != en)
	{
		if(en)
		{
			static ttstr eventname(TJS_W("onNodeEnabled"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
		}
		else
		{
			static ttstr eventname(TJS_W("onNodeDisabled"));
			TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
		}
	}

	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
		child->NotifyNodeEnabledState();
	TVP_LAYER_FOR_EACH_CHILD_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SaveEnabledWork()
{
	EnabledWork = GetNodeEnabled();

	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BEGIN(child)
		child->SaveEnabledWork();
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_END
}
//---------------------------------------------------------------------------

void tTJSNI_BaseLayer::SetEnabled(bool b)
{
	// set enabled
	if(Enabled != b)
	{
		if(Manager) Manager->SaveEnabledWork();

		try
		{
			Enabled = b;

			if(Enabled)
			{
				// become enabled
				if(Manager) Manager->CheckTreeFocusableState(this);
			}
			else
			{
				// become disabled
				if(Manager) Manager->BlurTree(this);
			}
		}
		catch(...)
		{
			if(Manager) Manager->NotifyNodeEnabledState();
			throw;
		}

		if(Manager) Manager->NotifyNodeEnabledState();
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::ParentEnabled()
{
	tTJSNI_BaseLayer *par = Parent;
	while(par)
	{
		if(!par->Enabled) { return false; }
		par = par->Parent;
	}

	return true;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireKeyDown(tjs_uint key, tjs_uint32 shift)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[3];
		param[0] = (tjs_int)key;
		param[1] = (tjs_int)shift;
		param[2] = true;
		static ttstr eventname(TJS_W("onKeyDown"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 3, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireKeyUp(tjs_uint key, tjs_uint32 shift)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant param[3];
		param[0] = (tjs_int)key;
		param[1] = (tjs_int)shift;
		param[2] = true;
		static ttstr eventname(TJS_W("onKeyUp"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 3, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireKeyPress(tjs_char key)
{
	if(Owner && !Shutdown)
	{
		tjs_char buf[2];
		buf[0] = (tjs_char)key;
		buf[1] = 0;
		tTJSVariant param[2];
		param[0] = buf;
		param[1] = true;
		static ttstr eventname(TJS_W("onKeyPress"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 2, param);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FireMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x,
	tjs_int y)
{
	if(Owner && !Shutdown)
	{
		tTJSVariant val[4] = { (tjs_int)shift, delta, x, y };
		static ttstr eventname(TJS_W("onMouseWheel"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 4, val);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DefaultKeyDown(tjs_uint key, tjs_uint32 shift)
{
	// default keyboard behavior
	// this method is to be called by default onKeyDown event handler
	if(!Manager) return;

	bool no_shift_downed =  !(shift & TVP_SS_SHIFT) && !(shift & TVP_SS_ALT) &&
		!(shift & TVP_SS_CTRL);

	if((key == VK_TAB || key == VK_RIGHT || key == VK_DOWN) && no_shift_downed)
	{
		// [TAB] [<RIGHT>] [<DOWN>] : to next focusable
		Manager->FocusNext();
	}
	else if((key == VK_TAB && (shift & TVP_SS_SHIFT) && !(shift & TVP_SS_ALT) &&
		!(shift & TVP_SS_CTRL)) || key == VK_LEFT || key == VK_UP)
	{
		// [SHIFT]+[TAB] [<LEFT>] [<UP>] : to previous focusable
		Manager->FocusPrev();
	}
	else if((key == VK_RETURN || key == VK_ESCAPE) && no_shift_downed)
	{
		// [ENTER] or [ESC] : pass to parent layer
		if(Parent)
		{
			if(Parent->GetNodeEnabled()) Parent->FireKeyDown(key, shift);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DefaultKeyUp(tjs_uint key, tjs_uint32 shift)
{
	// default keyboard behavior
	if(!Manager) return;


	bool no_shift_downed =  !(shift & TVP_SS_SHIFT) && !(shift & TVP_SS_ALT) &&
		!(shift & TVP_SS_CTRL);

	if((key == VK_RETURN || key == VK_ESCAPE) && no_shift_downed)
	{
		// [ENTER] or [ESC] : pass to parent layer
		if(Parent)
		{
			if(Parent->GetNodeEnabled()) Parent->FireKeyUp(key, shift);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DefaultKeyPress(tjs_char key)
{
	// default keyboard behavior
	if(!Manager) return;

	if(key == 13/* enter */ || key == 0x1b/* esc */)
	{
		if(Parent)
		{
			if(Parent->GetNodeEnabled()) Parent->FireKeyPress(key);
		}
	}
}
//---------------------------------------------------------------------------






//---------------------------------------------------------------------------
// cache management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AllocateCache()
{
	if(!CacheBitmap)
	{
		CacheBitmap =
			new tTVPBaseTexture(Rect.get_width(), Rect.get_height(), 32);
	}
	else
	{
		CacheBitmap->SetSize(Rect.get_width(), Rect.get_height());
	}
	CacheRecalcRegion.Or(tTVPRect(0, 0, Rect.get_width(), Rect.get_height()));
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ResizeCache()
{
	// resize to Rect's size
	if(CacheBitmap && MainImage)
	{
		CacheBitmap->SetSize(MainImage->GetWidth(), MainImage->GetHeight());
	}
	CacheRecalcRegion.Or(tTVPRect(0, 0, Rect.get_width(), Rect.get_height()));
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DeallocateCache()
{
	if(CacheBitmap)
	{
		delete CacheBitmap; CacheBitmap = NULL;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DispSizeChanged()
{
	// is called from geographical management
	ResizeCache();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CompactCache()
{
	// free cache image if the cache is not needed
	if(!CacheEnabledCount) DeallocateCache();
}
//---------------------------------------------------------------------------
tjs_uint tTJSNI_BaseLayer::IncCacheEnabledCount()
{
	CacheEnabledCount++;
	if(CacheEnabledCount)
	{
		RegisterCompactEventHook();
			// register to compact event hook to call CompactCache when idle
		AllocateCache();
	}
	return CacheEnabledCount;
}
//---------------------------------------------------------------------------
tjs_uint tTJSNI_BaseLayer::DecCacheEnabledCount()
{
	CacheEnabledCount--;
	if(TVPFreeUnusedLayerCache && !CacheEnabledCount)
		DeallocateCache();
		// object is not freed until compact event, unless TVPFreeUnusedLayerCache flags
	return CacheEnabledCount;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetCached(bool b)
{
	if(b != Cached)
	{
		Cached = b;
		if(b)
			IncCacheEnabledCount();
		else
			DecCacheEnabledCount();
	}
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// drawing function stuff
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ResetClip()
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	ClipRect.left = ClipRect.top = 0;
	ClipRect.right = MainImage->GetWidth();
	ClipRect.bottom = MainImage->GetHeight();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetClip(tjs_int left, tjs_int top, tjs_int width, tjs_int height)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	ClipRect.left = left < 0 ? 0 : left;
	ClipRect.top = top < 0 ? 0 : top;
	tjs_int right = width + left;
	tjs_int bottom = height + top;
	tjs_int w = MainImage->GetWidth();
	tjs_int h = MainImage->GetHeight();
	ClipRect.right = w < right ? w : right;
	ClipRect.bottom = h < bottom ? h : bottom;
	if(ClipRect.right < ClipRect.left) ClipRect.right = ClipRect.left;
	if(ClipRect.bottom < ClipRect.top) ClipRect.bottom = ClipRect.top;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetClipLeft(tjs_int left)
{
	SetClip(left, GetClipTop(), GetClipWidth(), GetClipHeight());
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetClipTop(tjs_int top)
{
	SetClip(GetClipLeft(), top, GetClipWidth(), GetClipHeight());
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetClipWidth(tjs_int width)
{
	SetClip(GetClipLeft(), GetClipTop(), width, GetClipHeight());
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetClipHeight(tjs_int height)
{
	SetClip(GetClipLeft(), GetClipTop(), GetClipWidth(), height);
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::ClipDestPointAndSrcRect(tjs_int &dx, tjs_int &dy,
		tTVPRect &srcrectout, const tTVPRect &srcrect) const
{
	// clip (dx, dy) <- srcrect	with current clipping rectangle
	srcrectout = srcrect;
	tjs_int dr = dx + srcrect.right - srcrect.left;
	tjs_int db = dy + srcrect.bottom - srcrect.top;

	if(dx < ClipRect.left)
	{
		srcrectout.left += (ClipRect.left - dx);
		dx = ClipRect.left;
	}

	if(dr > ClipRect.right)
	{
		srcrectout.right -= (dr - ClipRect.right);
	}

	if(srcrectout.right <= srcrectout.left) return false; // out of the clipping rect

	if(dy < ClipRect.top)
	{
		srcrectout.top += (ClipRect.top - dy);
		dy = ClipRect.top;
	}

	if(db > ClipRect.bottom)
	{
		srcrectout.bottom -= (db - ClipRect.bottom);
	}

	if(srcrectout.bottom <= srcrectout.top) return false; // out of the clipping rect

	return true;
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetBltMethodFromOperationModeAndDrawFace(
		tTVPBBBltMethod & result,
		tTVPBlendOperationMode mode)
{
	// resulting corresponding  tTVPBBBltMethod value of mode and current DrawFace.
	// returns whether the method is known.
	tTVPBBBltMethod met;
	bool met_set = false;
	switch(mode)
	{
	case omPsNormal:			met_set = true; met = bmPsNormal;			break;
	case omPsAdditive:			met_set = true; met = bmPsAdditive;			break;
	case omPsSubtractive:		met_set = true; met = bmPsSubtractive;		break;
	case omPsMultiplicative:	met_set = true; met = bmPsMultiplicative;	break;
	case omPsScreen:			met_set = true; met = bmPsScreen;			break;
	case omPsOverlay:			met_set = true; met = bmPsOverlay;			break;
	case omPsHardLight:			met_set = true; met = bmPsHardLight;		break;
	case omPsSoftLight:			met_set = true; met = bmPsSoftLight;		break;
	case omPsColorDodge:		met_set = true; met = bmPsColorDodge;		break;
	case omPsColorDodge5:		met_set = true; met = bmPsColorDodge5;		break;
	case omPsColorBurn:			met_set = true; met = bmPsColorBurn;		break;
	case omPsLighten:			met_set = true; met = bmPsLighten;			break;
	case omPsDarken:			met_set = true; met = bmPsDarken;			break;
	case omPsDifference:   		met_set = true; met = bmPsDifference;		break;
	case omPsDifference5:   	met_set = true; met = bmPsDifference5;		break;
	case omPsExclusion:			met_set = true; met = bmPsExclusion;		break;
	case omAdditive:			met_set = true; met = bmAdd;				break;
	case omSubtractive:			met_set = true; met = bmSub;				break;
	case omMultiplicative:		met_set = true; met = bmMul;				break;
	case omDodge:				met_set = true; met = bmDodge;				break;
	case omDarken:				met_set = true; met = bmDarken;				break;
	case omLighten:				met_set = true; met = bmLighten;			break;
	case omScreen:				met_set = true; met = bmScreen;				break;
	case omAlpha:
		if(DrawFace == dfAlpha)
						{	met_set = true; met = bmAlphaOnAlpha; break;		}
		else if(DrawFace == dfAddAlpha)
						{	met_set = true; met = bmAlphaOnAddAlpha; break;		}
		else if(DrawFace == dfOpaque)
						{	met_set = true; met = bmAlpha; break;				}
		break;
	case omAddAlpha:
		if(DrawFace == dfAlpha)
						{	met_set = true; met = bmAddAlphaOnAlpha; break;		}
		else if(DrawFace == dfAddAlpha)
						{	met_set = true; met = bmAddAlphaOnAddAlpha; break;	}
		else if(DrawFace == dfOpaque)
						{	met_set = true; met = bmAddAlpha; break;			}
		break;
	case omOpaque:
		if(DrawFace == dfAlpha)
						{	met_set = true; met = bmCopyOnAlpha; break;			}
		else if(DrawFace == dfAddAlpha)
						{	met_set = true; met = bmCopyOnAddAlpha; break;		}
		else if(DrawFace == dfOpaque)
						{	met_set = true; met = bmCopy; break;				}
		break;
	case dfAuto:
		break;
	}

	result = met;

	return met_set;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::FillRect(const tTVPRect &rect, tjs_uint32 color)
{
	// fill given rectangle with given "color"
	// this method does not do transparent coloring.

	tTVPRect destrect;
	if(!TVPIntersectRect(&destrect, rect, ClipRect)) return; // out of the clipping rectangle

	if(DrawFace == dfAlpha || DrawFace == dfAddAlpha || (DrawFace == dfOpaque && !HoldAlpha))
	{
		// main and mask
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		color = (color & 0xff000000) + (TVPToActualColor(color&0xffffff)&0xffffff);
		ImageModified = MainImage->Fill(destrect, color) || ImageModified;
	}
	else if(DrawFace == dfOpaque)
	{
		// main only
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		color = TVPToActualColor(color);
		ImageModified = MainImage->FillColor(destrect, color, 255) || ImageModified;
	}
	else if(DrawFace == dfMask)
	{
		// mask only
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		ImageModified = MainImage->FillMask(destrect, color&0xff) || ImageModified;
	}
	else if(DrawFace == dfProvince)
	{
		// province
		color = color & 0xff;
		if(color)
		{
			if(!ProvinceImage) AllocateProvinceImage();
			if(ProvinceImage)
				ImageModified = ProvinceImage->Fill(destrect, color&0xff) || ImageModified;
		}
		else
		{
			if(ProvinceImage)
			{
				if(destrect.left == 0 && destrect.top == 0 &&
					destrect.right == (tjs_int)ProvinceImage->GetWidth() &&
					destrect.bottom == (tjs_int)ProvinceImage->GetHeight())
				{
					// entire area of the province image will be filled with 0
					DeallocateProvinceImage();
					ImageModified = true;
				}
				else
				{
					ImageModified = ProvinceImage->Fill(destrect, color&0xff) || ImageModified;
				}
			}
		}
	}

	if(ImageLeft != 0 || ImageTop != 0)
	{
		tTVPRect ur = destrect;
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(destrect);
	}

}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ColorRect(const tTVPRect &rect, tjs_uint32 color, tjs_int opa)
{
	// color given rectangle with given "color"

	tTVPRect destrect;
	if(!TVPIntersectRect(&destrect, rect, ClipRect)) return; // out of the clipping rectangle

	switch(DrawFace)
	{
	case dfAlpha: // main and mask
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(opa > 0)
		{
			color = TVPToActualColor(color);
			ImageModified = MainImage->FillColorOnAlpha(destrect, color, opa) || ImageModified;
		}
		else
		{
			ImageModified = MainImage->RemoveConstOpacity(destrect, -opa) || ImageModified;
		}
		break;

	case dfAddAlpha: // additive alpha; main and mask
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(opa >= 0)
		{
			color = TVPToActualColor(color);
			ImageModified = MainImage->FillColorOnAddAlpha(destrect, color, opa) || ImageModified;
		}
		else
		{
			TVPThrowExceptionMessage(TVPNegativeOpacityNotSupportedOnThisFace);
		}
		break;

	case dfOpaque: // main only
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		color = TVPToActualColor(color);
		ImageModified = MainImage->FillColor(destrect, color, opa) || ImageModified;
			// note that tTVPBaseBitmap::FillColor always holds destination alpha
		break;

	case dfMask: // mask ( opacity will be ignored )
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		ImageModified = MainImage->FillMask(destrect, color&0xff ) || ImageModified;
		break;

	case dfProvince: // province ( opacity will be ignored )
		color = color & 0xff;
		if(color)
		{
			if(!ProvinceImage) AllocateProvinceImage();
			if(ProvinceImage)
				ImageModified = ProvinceImage->Fill(destrect, color&0xff) || ImageModified;
		}
		else
		{
			if(ProvinceImage)
			{
				if(destrect.left == 0 && destrect.top == 0 &&
					destrect.right == (tjs_int)ProvinceImage->GetWidth() &&
					destrect.bottom == (tjs_int)ProvinceImage->GetHeight())
				{
					// entire area of the province image will be filled with 0
					DeallocateProvinceImage();
					ImageModified = true;
				}
				else
				{
					ImageModified = ProvinceImage->Fill(destrect, color&0xff) || ImageModified;
				}
			}
		}
		break;

	case dfAuto:
		break;
	}

	if(ImageLeft != 0 || ImageTop != 0)
	{
		tTVPRect ur = destrect;
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(destrect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DrawText(tjs_int x, tjs_int y, const ttstr &text,
	tjs_uint32 color, tjs_int opa, bool aa, tjs_int shadowlevel,
		tjs_uint32 shadowcolor, tjs_int shadowwidth, tjs_int shadowofsx,
		tjs_int shadowofsy)
{
	// draw text
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	tTVPBBBltMethod met;

	switch(DrawFace)
	{
	case dfAlpha:
		met = bmAlphaOnAlpha;
		break;
	case dfAddAlpha:
		if(opa<0) TVPThrowExceptionMessage(TVPNegativeOpacityNotSupportedOnThisFace);
		met = bmAlphaOnAddAlpha;
		break;
	case dfOpaque:
		met = bmAlpha;
		break;
	default:
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("drawText"));
	}

	ApplyFont();

	tTVPComplexRect r;

	color = TVPToActualColor(color);

	MainImage->DrawText(ClipRect, x, y, text, TVP_REVRGB(color), met,
		opa, HoldAlpha, aa, shadowlevel, TVP_REVRGB(shadowcolor), shadowwidth,
		shadowofsx, shadowofsy, &r);

	if(r.GetCount()) ImageModified = true;

	if(ImageLeft != 0 || ImageTop != 0)
	{
		r.AddOffsets(ImageLeft, ImageTop);
	}
	Update(r);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DrawGlyph(tjs_int x, tjs_int y, iTJSDispatch2* glyph, tjs_uint32 color,
		tjs_int opa, bool aa, tjs_int shadowlevel, tjs_uint32 shadowcolor,
		tjs_int shadowwidth, tjs_int shadowofsx, tjs_int shadowofsy)
{
	// draw text
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	tTVPBBBltMethod met;

	switch(DrawFace)
	{
	case dfAlpha:
		met = bmAlphaOnAlpha;
		break;
	case dfAddAlpha:
		if(opa<0) TVPThrowExceptionMessage(TVPNegativeOpacityNotSupportedOnThisFace);
		met = bmAlphaOnAddAlpha;
		break;
	case dfOpaque:
		met = bmAlpha;
		break;
	default:
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("drawGlyph"));
	}

	ApplyFont();

	tTVPComplexRect r;

	color = TVPToActualColor(color);

	MainImage->DrawGlyph( glyph, ClipRect, x, y, color, met,
		opa, HoldAlpha, aa, shadowlevel, shadowcolor, shadowwidth,
		shadowofsx, shadowofsy, &r);

	if(r.GetCount()) ImageModified = true;

	if(ImageLeft != 0 || ImageTop != 0)
	{
		r.AddOffsets(ImageLeft, ImageTop);
	}
	Update(r);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::PiledCopy(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect)
{
	// rectangle copy of piled layer image

	// this can transfer the piled image of the source layer
	// this ignores Drawface of this, or DrawFace of the source layer.
	// this is affected by source layer type.

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);

	tTVPRect rect;
	if(!ClipDestPointAndSrcRect(dx, dy, rect, srcrect)) return; // out of the clipping rect

	src->IncCacheEnabledCount(); // enable cache
	try
	{
        iTVPBaseBitmap *bmp = src->Complete(rect);
		tTVPRect rc(rect);
		if (IsGPU()) {
			rc.set_offsets(0, 0);
		}
		ImageModified = MainImage->CopyRect(dx, dy, bmp, rc,
			TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK) || ImageModified;
	}
	catch(...)
	{
		src->DecCacheEnabledCount();
		throw;
	}
	src->DecCacheEnabledCount(); // disable cache

	tTVPRect ur = rect;
	ur.set_offsets(dx, dy);
	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CopyRect(tjs_int dx, tjs_int dy, iTVPBaseBitmap *src, iTVPBaseBitmap *provincesrc,
	const tTVPRect &srcrect)
{
	// copy rectangle

	// this method switches automatically backward or forward copy, when
	// the distination and the source each other are overlapped.

	tTVPRect rect;
	if(!ClipDestPointAndSrcRect(dx, dy, rect, srcrect)) return; // out of the clipping rect

	switch(DrawFace)
	{
	case dfAlpha:
	case dfAddAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified = MainImage->CopyRect(dx, dy, src, rect,
			TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified = MainImage->CopyRect(dx, dy, src, rect,
			HoldAlpha?TVP_BB_COPY_MAIN:(TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK)) || ImageModified;
		break;

	case dfMask:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified = MainImage->CopyRect(dx, dy, src, rect,
			TVP_BB_COPY_MASK) || ImageModified;
		break;

	case dfProvince:
		if(!provincesrc)
		{
			// source province image is null;
			// fill destination with zero
			if(ProvinceImage) ProvinceImage->Fill(rect, 0);
			ImageModified = true;
		}
		else
		{
			// province image is not created if the image is not needed
			// allocate province image
			if(!ProvinceImage) AllocateProvinceImage();
			// then copy
			ImageModified = ProvinceImage->CopyRect(dx, dy, provincesrc, rect) || ImageModified;
		}
		break;
	case dfAuto:
		break;
	}


	tTVPRect ur = rect;
	ur.set_offsets(dx, dy);
	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::Copy9Patch( const iTVPBaseBitmap *src, tTVPRect &margin )
{
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	ImageModified = MainImage->Copy9Patch( src, margin );
	if( ImageModified )
	{
		tTVPRect ur(0,0,GetImageWidth(),GetImageHeight());
		if(ImageLeft != 0 || ImageTop != 0)
		{
			ur.add_offsets(ImageLeft, ImageTop);
			Update(ur);
		}
		else
		{
			Update(ur);
		}
	}
	return ImageModified;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StretchCopy(const tTVPRect &destrect, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBBStretchType type, tjs_real typeopt)
{
	// stretching copy
	tTVPRect ur = destrect;
	if(ur.right < ur.left) std::swap(ur.right, ur.left);
	if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
	if(!TVPIntersectRect(&ur, ur, ClipRect)) return; // out of the clipping rectangle

	switch(DrawFace)
	{
	case dfAlpha:
	case dfAddAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->StretchBlt(ClipRect, destrect, src, srcrect, bmCopy, 255, false,
				type, typeopt) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->StretchBlt(ClipRect, destrect, src, srcrect,
				bmCopy, 255, HoldAlpha, type, typeopt) || ImageModified;
		break;

	default:
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("stretchCopy"));
	}

	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffineCopy(const t2DAffineMatrix &matrix, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBBStretchType type, bool clear)
{
	// affine copy
	tTVPRect updaterect;
	bool updated;

	switch(DrawFace)
	{
	case dfAlpha:
	case dfAddAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src, srcrect, matrix,
			bmCopy, 255, &updaterect, false, type, clear, NeutralColor);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src, srcrect, matrix,
			bmCopy, 255, &updaterect, HoldAlpha, type, clear, NeutralColor);
		break;
	  }

	default:
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affineCopy"));
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffineCopy(const tTVPPointD *points, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBBStretchType type, bool clear)
{
	// affine copy
	tTVPRect updaterect;
	bool updated;

	switch(DrawFace)
	{
	case dfAlpha:
	case dfAddAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src, srcrect, points,
			bmCopy, 255, &updaterect, false, type, clear, NeutralColor);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src, srcrect, points,
			bmCopy, 255, &updaterect, HoldAlpha, type, clear, NeutralColor);
		break;
	  }

	default:
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affineCopy"));
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::PileRect(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity)
{
	// obsoleted (use OperateRect)

	// pile rectangle ( pixel alpha blend )

	// piled destination is determined by Drawface (not LayerType).
	// dfAlpha: destination alpha is considered
	// dfOpaque: destination alpha is ignored ( treated as full opaque )
	// dfMask or dfProvince : causes an error
	// this method ignores soruce layer's LayerType or DrawFace.
	// the destination alpha is held on dfAlpha if 'HoldAlpha' is true, otherwide the
	// alpha information is destroyed.

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("pileRect"));
	}

	tTVPRect rect;
	if(!ClipDestPointAndSrcRect(dx, dy, rect, srcrect)) return; // out of the clipping rect

	switch(DrawFace)
	{
	case dfAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->Blt(dx, dy, src->MainImage, rect, bmAlphaOnAlpha, opacity, HoldAlpha) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->Blt(dx, dy, src->MainImage, rect, bmAlpha, opacity, HoldAlpha) || ImageModified;
		break;

	default:
		break;
	}

	tTVPRect ur = rect;
	ur.set_offsets(dx, dy);
	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::BlendRect(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity)
{
	// obsoleted (use OperateRect)

	// blend rectangle ( constant alpha blend )

	// mostly the same as 'PileRect', but this does treat src as completely
	// opaque image. 

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("blendRect"));
	}

	tTVPRect rect;
	if(!ClipDestPointAndSrcRect(dx, dy, rect, srcrect)) return; // out of the clipping rect

	switch(DrawFace)
	{
	case dfAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->Blt(dx, dy, src->MainImage, rect, bmCopyOnAlpha, opacity, HoldAlpha) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		ImageModified =
			MainImage->Blt(dx, dy, src->MainImage, rect, bmCopy, opacity, HoldAlpha) || ImageModified;
		break;

	default:
		break;
	}

	tTVPRect ur = rect;
	ur.set_offsets(dx, dy);
	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::OperateRect(tjs_int dx, tjs_int dy, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode,
			tjs_int opacity)
{
	// operate on rectangle ( add/sub/mul/div and others )
	tTVPRect rect;
	if(!ClipDestPointAndSrcRect(dx, dy, rect, srcrect)) return; // out of the clipping rect

	// It does not throw an exception in this case perhaps
	if(mode == omAuto) TVPThrowExceptionMessage( TVPCannotAcceptModeAuto );

	// convert tTVPBlendOperationMode to tTVPBBBltMethod
	tTVPBBBltMethod met;
	if(!GetBltMethodFromOperationModeAndDrawFace(met, mode))
	{
		// unknown blt mode
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("operateRect"));
	}

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);

	ImageModified =
		MainImage->Blt(dx, dy, src, rect, met, opacity, HoldAlpha) || ImageModified;

	tTVPRect ur = rect;
	ur.set_offsets(dx, dy);
	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StretchPile(const tTVPRect &destrect, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity, tTVPBBStretchType type)
{
	// obsoleted (use OperateStretch)

	// stretching pile
	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("stretchPile"));
	}

	tTVPRect ur = destrect;
	if(ur.right < ur.left) std::swap(ur.right, ur.left);
	if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
	if(!TVPIntersectRect(&ur, ur, ClipRect)) return; // out of the clipping rectangle


	switch(DrawFace)
	{
	case dfAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
        ImageModified = MainImage->StretchBlt(ClipRect, destrect, src->MainImage, srcrect, bmAlphaOnAlpha,
			opacity, HoldAlpha, type) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
        ImageModified = MainImage->StretchBlt(ClipRect, destrect, src->MainImage, srcrect, bmAlpha,
			opacity, HoldAlpha, type) || ImageModified;
		break;

	default:
		break;
	}


	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StretchBlend(const tTVPRect &destrect, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity, tTVPBBStretchType type)
{
	// obsoleted (use OperateStretch)

	// stretching blend
	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("stretchBlend"));
	}

	tTVPRect ur = destrect;
	if(ur.right < ur.left) std::swap(ur.right, ur.left);
	if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
	if(!TVPIntersectRect(&ur, ur, ClipRect)) return; // out of the clipping rectangle


	switch(DrawFace)
	{
	case dfAlpha:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
        ImageModified = MainImage->StretchBlt(ClipRect, destrect, src->MainImage, srcrect, bmCopyOnAlpha,
			opacity, HoldAlpha, type) || ImageModified;
		break;

	case dfOpaque:
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
        ImageModified = MainImage->StretchBlt(ClipRect, destrect, src->MainImage, srcrect, bmCopy,
			opacity, HoldAlpha, type) || ImageModified;
		break;

	default:
		break;
	}

	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::OperateStretch(const tTVPRect &destrect,
	iTVPBaseBitmap *src, const tTVPRect &srcrect, tTVPBlendOperationMode mode, tjs_int opacity,
			tTVPBBStretchType type, tjs_real typeopt)
{
	// stretching operation (add/mul/sub etc.)

	tTVPRect ur = destrect;
	if(ur.right < ur.left) std::swap(ur.right, ur.left);
	if(ur.bottom < ur.top) std::swap(ur.bottom, ur.top);
	if(!TVPIntersectRect(&ur, ur, ClipRect)) return; // out of the clipping rectangle
	
	// It does not throw an exception in this case perhaps
	if(mode == omAuto) TVPThrowExceptionMessage( TVPCannotAcceptModeAuto );

	// convert tTVPBlendOperationMode to tTVPBBBltMethod
	tTVPBBBltMethod met;
	if(!GetBltMethodFromOperationModeAndDrawFace(met, mode))
	{
		// unknown blt mode
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("operateStretch"));
	}

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
	ImageModified = MainImage->StretchBlt(ClipRect, destrect, src, srcrect, met,
		opacity, HoldAlpha, type, typeopt) || ImageModified;

	if(ImageLeft != 0 || ImageTop != 0)
	{
		ur.add_offsets(ImageLeft, ImageTop);
		Update(ur);
	}
	else
	{
		Update(ur);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffinePile(const t2DAffineMatrix &matrix, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity,
	tTVPBBStretchType type)
{
	// obsoleted (use OperateAffine)

	// affine pile
	tTVPRect updaterect;
	bool updated;

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affinePile"));
	}


	switch(DrawFace)
	{
	case dfAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, matrix,
			bmAlphaOnAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, matrix,
			bmAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }
	default:
		break;
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffinePile(const tTVPPointD *points, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity,
	tTVPBBStretchType type)
{
	// obsoleted (use OperateAffine)

	// affine pile
	tTVPRect updaterect;
	bool updated;

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affinePile"));
	}


	switch(DrawFace)
	{
	case dfAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, points,
			bmAlphaOnAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, points,
			bmAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }
	default:
		break;
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffineBlend(const t2DAffineMatrix &matrix, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity,
	tTVPBBStretchType type)
{
	// obsoleted (use OperateAffine)

	// affine blend
	tTVPRect updaterect;
	bool updated;

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affineBlend"));
	}


	switch(DrawFace)
	{
	case dfAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, matrix,
			bmCopyOnAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, matrix,
			bmCopy, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	default:
		break;
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AffineBlend(const tTVPPointD *points, tTJSNI_BaseLayer *src,
	const tTVPRect &srcrect, tjs_int opacity,
	tTVPBBStretchType type)
{
	// obsoleted (use OperateAffine)

	// affine blend
	tTVPRect updaterect;
	bool updated;

	if(DrawFace != dfAlpha && DrawFace != dfOpaque)
	{
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("affineBlend"));
	}


	switch(DrawFace)
	{
	case dfAlpha:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, points,
			bmCopyOnAlpha, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	case dfOpaque:
	  {
		if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
		if(!src->MainImage) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
		updated = MainImage->AffineBlt(ClipRect, src->MainImage, srcrect, points,
			bmCopy, opacity, &updaterect, HoldAlpha, type);
		break;
	  }

	default:
		break;
	}

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::OperateAffine(const t2DAffineMatrix &matrix,
	iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode, tjs_int opacity,
		tTVPBBStretchType type)
{
	// affine operation
	tTVPRect updaterect;
	bool updated;
	
	// It does not throw an exception in this case perhaps
	if(mode == omAuto) TVPThrowExceptionMessage( TVPCannotAcceptModeAuto );

	// convert tTVPBlendOperationMode to tTVPBBBltMethod
	tTVPBBBltMethod met;
	if(!GetBltMethodFromOperationModeAndDrawFace(met, mode))
	{
		// unknown blt mode
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("operateAffine"));
	}

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
	updated = MainImage->AffineBlt(ClipRect, src, srcrect, matrix,
		met, opacity, &updaterect, HoldAlpha, type);

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}

}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::OperateAffine(const tTVPPointD *points, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode, tjs_int opacity,
		tTVPBBStretchType type)
{
	// affine operation
	tTVPRect updaterect;
	bool updated;
	
	// It does not throw an exception in this case perhaps
	if(mode == omAuto) TVPThrowExceptionMessage( TVPCannotAcceptModeAuto );

	// convert tTVPBlendOperationMode to tTVPBBBltMethod
	tTVPBBBltMethod met;
	if(!GetBltMethodFromOperationModeAndDrawFace(met, mode))
	{
		// unknown blt mode
		TVPThrowExceptionMessage(TVPNotDrawableFaceType, TJS_W("operateAffine"));
	}

	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);
	if(!src) TVPThrowExceptionMessage(TVPSourceLayerHasNoImage);
	updated = MainImage->AffineBlt(ClipRect, src, srcrect, points,
		met, opacity, &updaterect, HoldAlpha, type);

	ImageModified = updated || ImageModified;

	if(updated)
	{
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DoBoxBlur(tjs_int xblur, tjs_int yblur)
{
	// blur with box blur method
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	bool updated;

	if(DrawFace != dfAlpha)
		updated = MainImage->DoBoxBlur(ClipRect, tTVPRect(-xblur, -yblur, xblur, yblur));
	else
		updated = MainImage->DoBoxBlurForAlpha(ClipRect, tTVPRect(-xblur, -yblur, xblur, yblur));

	ImageModified = updated || ImageModified;

	if(updated)
	{
		tTVPRect updaterect(ClipRect);
		updaterect.add_offsets(ImageLeft, ImageTop);
		Update(updaterect);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AdjustGamma(const tTVPGLGammaAdjustData & data)
{
	// this is not affected by DrawFace
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	if(DrawFace == dfAddAlpha)
		MainImage->AdjustGammaForAdditiveAlpha(
			ClipRect,
			data);
	else
		MainImage->AdjustGamma(
			ClipRect,
			data);

	ImageModified = true;
	Update();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DoGrayScale()
{
	// this is not affected by DrawFace
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	MainImage->DoGrayScale(ClipRect);

	ImageModified = true;
	Update();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::LRFlip()
{
	// this is not affected by DrawFace
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	tTVPRect r(0, 0, MainImage->GetWidth(), MainImage->GetHeight());
	MainImage->LRFlip(r);
	if(ProvinceImage) ProvinceImage->LRFlip(r);

	ImageModified = true;
	Update();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UDFlip()
{
	// this is not affected by DrawFace
	if(!MainImage) TVPThrowExceptionMessage(TVPNotDrawableLayerType);

	tTVPRect r(0, 0, MainImage->GetWidth(), MainImage->GetHeight());
	MainImage->UDFlip(r);
	if(ProvinceImage) ProvinceImage->UDFlip(r);

	ImageModified = true;
	Update();
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// interface to font object
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ApplyFont()
{
	if(FontChanged && MainImage)
	{
		FontChanged = false;
		MainImage->SetFont(Font);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontFace(const ttstr & face)
{
	if(Font.Face != face)
	{
		Font.Face = face;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
ttstr tTJSNI_BaseLayer::GetFontFace() const
{
	return Font.Face;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontHeight(tjs_int height)
{
	if(height < 0) height = -height; // TVP2 does not support negative value of height

	if(Font.Height != height)
	{
		Font.Height = height;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetFontHeight() const
{
	return Font.Height;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontAngle(tjs_int angle)
{
	if(Font.Angle != angle)
	{
		angle = angle % 3600;
		if(angle < 0) angle += 3600;
		Font.Angle = angle;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetFontAngle() const
{
	return Font.Angle;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontBold(bool b)
{
	if( (0!=(Font.Flags & TVP_TF_BOLD)) != b)
	{
		Font.Flags &= ~TVP_TF_BOLD;
		if(b) Font.Flags |= TVP_TF_BOLD;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFontBold() const
{
	return 0!=(Font.Flags & TVP_TF_BOLD);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontItalic(bool b)
{
	if( (0!=(Font.Flags & TVP_TF_ITALIC)) != b)
	{
		Font.Flags &= ~TVP_TF_ITALIC;
		if(b) Font.Flags |= TVP_TF_ITALIC;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFontItalic() const
{
	return 0!=(Font.Flags & TVP_TF_ITALIC);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontStrikeout(bool b)
{
	if( (0!=(Font.Flags & TVP_TF_STRIKEOUT)) != b)
	{
		Font.Flags &= ~TVP_TF_STRIKEOUT;
		if(b) Font.Flags |= TVP_TF_STRIKEOUT;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFontStrikeout() const
{
	return 0!=(Font.Flags & TVP_TF_STRIKEOUT);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontUnderline(bool b)
{
	if( (0!=(Font.Flags & TVP_TF_UNDERLINE)) != b)
	{
		Font.Flags &= ~TVP_TF_UNDERLINE;
		if(b) Font.Flags |= TVP_TF_UNDERLINE;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFontUnderline() const
{
	return 0!=(Font.Flags & TVP_TF_UNDERLINE);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::SetFontFaceIsFileName(bool b)
{
	if( (0!=(Font.Flags & TVP_TF_FONTFILE)) != b)
	{
		Font.Flags &= ~TVP_TF_FONTFILE;
		if(b) Font.Flags |= TVP_TF_FONTFILE;
		FontChanged = true;
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_BaseLayer::GetFontFaceIsFileName() const
{
	return 0!=(Font.Flags & TVP_TF_FONTFILE);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetTextWidth(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getTextWidth"));

	ApplyFont();

	return MainImage->GetTextWidth(text);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_BaseLayer::GetTextHeight(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getTextHeight"));

	ApplyFont();

	return MainImage->GetTextHeight(text);
}
//---------------------------------------------------------------------------
double tTJSNI_BaseLayer::GetEscWidthX(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getEscWidthX"));

	ApplyFont();

	return MainImage->GetEscWidthX(text);
}
//---------------------------------------------------------------------------
double tTJSNI_BaseLayer::GetEscWidthY(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getEscWidthY"));

	ApplyFont();

	return MainImage->GetEscWidthY(text);
}
//---------------------------------------------------------------------------
double tTJSNI_BaseLayer::GetEscHeightX(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getEscHeightX"));

	ApplyFont();

	return MainImage->GetEscHeightX(text);
}
//---------------------------------------------------------------------------
double tTJSNI_BaseLayer::GetEscHeightY(const ttstr & text)
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getEscHeightY"));

	ApplyFont();

	return MainImage->GetEscHeightY(text);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::GetFontGlyphDrawRect( const ttstr & text, tTVPRect& area )
{
	if(!MainImage) TVPThrowExceptionMessage(TVPUnsupportedLayerType,
						TJS_W("getGlyphDrawRect"));

	ApplyFont();

	MainImage->GetFontGlyphDrawRect(text,area);
}
//---------------------------------------------------------------------------
#if 0
bool tTJSNI_BaseLayer::DoUserFontSelect(tjs_uint32 flags, const ttstr &caption,
		const ttstr &prompt, const ttstr &samplestring)
{
	ApplyFont();

	bool b = MainImage->SelectFont(flags, caption, prompt, samplestring,
		Font.Face);
	if(b) FontChanged = true;
	return b;
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::GetFontList(tjs_uint32 flags, std::vector<ttstr> & list)
{
	ApplyFont();

	MainImage->GetFontList(flags, list);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::MapPrerenderedFont(const ttstr & storage)
{
	ApplyFont();

	MainImage->MapPrerenderedFont(storage);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UnmapPrerenderedFont()
{
	ApplyFont();

	MainImage->UnmapPrerenderedFont();
}
//---------------------------------------------------------------------------
const tTVPFont&  tTJSNI_BaseLayer::GetFont() const
{
	return Font;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * tTJSNI_BaseLayer::GetFontObjectNoAddRef()
{
	if(FontObject) return FontObject;

	// create font object if the object is not yet created.
	if(!Owner) TVPThrowExceptionMessage(TVPLayerObjectIsNotProperlyConstructed);
	FontObject = TVPCreateFontObject(Owner);

	return FontObject;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// updating management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UpdateTransDestinationOnSelfUpdate(const tTVPComplexRect &region)
{
	if(TransDest && TransDest->InTransition && TransDest->TransSelfUpdate )
	{
		// transition, its update is performed by user code, is processing on
		// transition destination.
		// update the transition destination as transition source does.
		switch(TransDest->TransUpdateType)
		{
		case tutDivisibleFade:
		  {
			tTVPComplexRect cp(region);
			TransDest->Update(cp, true);
			break;
		  }
		default:
			TransDest->Update(true);
				// update entire area of the transition destination
				// because we cannot determine where the update affects.
			break;
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UpdateTransDestinationOnSelfUpdate(const tTVPRect &rect)
{
	// essentially the same as UpdateTransDestinationOnSelfUpdate(const tTVPComplexRect &region)
	if(TransDest && TransDest->InTransition && TransDest->TransSelfUpdate )
	{
		switch(TransDest->TransUpdateType)
		{
		case tutDivisibleFade:
			TransDest->Update(rect, true);
			break;
		default:
			TransDest->Update(true);
			break;
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UpdateChildRegion(tTJSNI_BaseLayer *child,
	const tTVPComplexRect &region,
	bool tempupdate, bool targvisible, bool addtoprimary)
{
	// called by child.  add update rect subscribed in "rect"

	tTVPRect cr;
	cr.left = cr.top = 0;
	cr.right = Rect.get_width(); cr.bottom = Rect.get_height();

	tTVPComplexRect converted;
	converted.CopyWithOffsets(region, cr, child->Rect.left, child->Rect.top);

	if(!tempupdate)
	{
		if(GetCacheEnabled())
		{
			// caching is enabled
			if(targvisible)
			{
				CacheRecalcRegion.Or(converted);
				if(CacheRecalcRegion.GetCount() > TVP_CACHE_UNITE_LIMIT)
					CacheRecalcRegion.Unite();
			}
		}
	}

	if(Parent)
	{
		Parent->UpdateChildRegion(this, converted, tempupdate, targvisible,
			addtoprimary);
	}
	else
	{
		if(addtoprimary) if(Manager) Manager->AddUpdateRegion(converted);
	}

	UpdateTransDestinationOnSelfUpdate(converted);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalUpdate(const tTVPRect &rect, bool tempupdate)
{
	tTVPRect cr;
	cr.left = cr.top = 0;
	cr.right = Rect.get_width(); cr.bottom = Rect.get_height();
	if(!TVPIntersectRect(&cr, cr, rect)) return;

	if(!tempupdate)
	{
		if(GetCacheEnabled())
		{
			// caching is enabled
			CacheRecalcRegion.Or(cr);
			if(CacheRecalcRegion.GetCount() > TVP_CACHE_UNITE_LIMIT)
				CacheRecalcRegion.Unite();
		}
	}

	if(Parent)
	{
		tTVPComplexRect c;
		c.Or(cr);
		Parent->UpdateChildRegion(this, c, tempupdate, GetVisible(),
			GetNodeVisible());
	}
	else
	{
		if(Manager) Manager->AddUpdateRegion(cr);
	}

	UpdateTransDestinationOnSelfUpdate(cr);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Update(tTVPComplexRect &rects, bool tempupdate)
{
	tTVPRect cr;
	cr.left = cr.top = 0;
	cr.right = Rect.get_width(); cr.bottom = Rect.get_height();
	rects.And(cr);

	if(!rects.GetCount()) return;

	if(!tempupdate)
	{
		// in case of tempupdate == false
		/*
			tempupdate == true indicates that the layer content is not changed, 
			but the layer need to be updated to the window.
			Mainly used by transition update.

			There is no need to update CacheRecalcRegion because the
			layer content is not changed when tempupdate == true.
		*/

		if(GetCacheEnabled())
		{
			// caching is enabled
			CacheRecalcRegion.Or(rects);
			if(CacheRecalcRegion.GetCount() > TVP_CACHE_UNITE_LIMIT)
				CacheRecalcRegion.Unite();
		}
	}

	if(Parent)
	{
		Parent->UpdateChildRegion(this, rects, tempupdate, GetVisible(),
			GetNodeVisible());
	}
	else
	{
		if(Manager) Manager->AddUpdateRegion(rects);
	}

	UpdateTransDestinationOnSelfUpdate(rects);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Update(const tTVPRect &rect, bool tempupdate)
{
	// update part of the layer
	tTVPRect cr;
	cr.left = cr.top = 0;
	cr.right = Rect.get_width(); cr.bottom = Rect.get_height();
	if(!TVPIntersectRect(&cr, cr, rect)) return;

	InternalUpdate(cr, tempupdate);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Update(bool tempupdate)
{
	// update entire of the layer
	tTVPRect rect;
	rect.left = rect.top = 0;
	rect.right = Rect.get_width();
	rect.bottom = Rect.get_height();
	InternalUpdate(rect, tempupdate);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::ParentUpdate()
{
	// called when layer moves
	if(Parent)
	{
		tTVPRect rect;
		rect.left = rect.top = 0;
		rect.right = Rect.get_width();
		rect.bottom = Rect.get_height();
		tTVPComplexRect c;
		c.Or(rect);
		Parent->UpdateChildRegion(this, c, false, GetVisible(),
			GetNodeVisible());
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::UpdateAllChildren(bool tempupdate)
{
	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)

		child->Update(tempupdate);

	TVP_LAYER_FOR_EACH_CHILD_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::BeforeCompletion()
{
	// called before the drawing is processed
	if(InCompletion) return;
		// calling during completion more than once is not allowed


	// fire onPaint
	if(CallOnPaint)
	{
		CallOnPaint = false;
		static ttstr eventname(TJS_W("onPaint"));
		TVPPostEvent(Owner, Owner, eventname, 0, TVP_EPT_IMMEDIATE, 0, NULL);
	}


	// for transition
	if(InTransition)
	{
		// transition is processing

		if(DivisibleTransHandler)
		{
			// notify start of processing unit
			tjs_error er;
			if(TransSelfUpdate)
			{
				// set TransTick here if the transition is performed by user code;
				// otherwise the TransTick is to be set at
				// tTJSNI_BaseLayer::InvokeTransition
				TransTick = GetTransTick();
			}
			er = DivisibleTransHandler->StartProcess(TransTick);
			if(er != TJS_S_TRUE) StopTransitionByHandler();
		}
		else if(GiveUpdateTransHandler)
		{
			// not yet implemented
		}
	}

	if(InTransition && TransWithChildren &&
		TransUpdateType == tutDivisible)
	{
		// complete all area of the layer
		InTransition = false; // cheat!!!
		TransDrawable.Src1Bmp = Complete();
		InTransition = true;

		if(TransSrc) TransDrawable.Src2Bmp = TransSrc->Complete();
	}

	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)

		child->BeforeCompletion();

	TVP_LAYER_FOR_EACH_CHILD_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::AfterCompletion()
{
	// called after the drawing is processed
	if(InCompletion) return;
		// calling during completion more than once is not allowed

	if(InTransition)
	{
		// transition is processing
		if(DivisibleTransHandler)
		{
			// notify start of processing unit
			tjs_error er;
			er = DivisibleTransHandler->EndProcess();
			if(er != TJS_S_TRUE) StopTransitionByHandler();
		}
		else if(GiveUpdateTransHandler)
		{
			// not yet implemented
		}

	}

	TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)

		child->AfterCompletion();

	TVP_LAYER_FOR_EACH_CHILD_END
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::QueryUpdateExcludeRect(tTVPRect &rect, bool parentvisible)
{
	// query completely opaque area

	// convert to local coordinates
	rect.left -= Rect.left;
	rect.right -= Rect.left;
	rect.top -= Rect.top;
	rect.bottom -= Rect.top;

	// recur to children
	parentvisible = parentvisible && Visible &&
		(DisplayType == ltOpaque || DisplayType == ltAlpha || DisplayType == ltAddAlpha || DisplayType == ltPsNormal) &&
		Opacity == 255; // fixed 2004/01/09 W.Dee
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BACKWARD_BEGIN(child)

		child->QueryUpdateExcludeRect(rect, parentvisible);
     
	TVP_LAYER_FOR_EACH_CHILD_NOLOCK_BACKWARD_END

	// copy current update exclude rect
	if(parentvisible) UpdateExcludeRect = rect; else UpdateExcludeRect.clear();

	// convert to parent's coordinates
	rect.left += Rect.left;
	rect.right += Rect.left;
	rect.top += Rect.top;
	rect.bottom += Rect.top;

	// check visibility & opacity
	if (parentvisible && (DisplayType == ltOpaque || (MainImage && MainImage->IsOpaque())) && Opacity == 255)
	{
		if(rect.is_empty())
		{
			rect = Rect;
		}
		else
		{
			if(!TVPIntersectRect(&rect, rect, Rect))
				rect.clear();
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::BltImage(iTVPBaseBitmap *dest, tTVPLayerType destlayertype,
	tjs_int destx,
    tjs_int desty, iTVPBaseBitmap *src, const tTVPRect &srcrect,
	tTVPLayerType drawtype, tjs_int opacity, bool hda)
{
	// draw src to dest according with layer type
/*
	// do the effect
	tTVPRect destrect;
	destrect.left = destx;
	destrect.top = desty;
	destrect.right = destx + srcrect.get_width();
	destrect.bottom = desty + srcrect.get_height();
	EffectImage(dest, destrect);
*/

	// blt to the target
	tTVPBBBltMethod met;
	switch(drawtype)
	{
	case ltBinder:
		// no action
		return;

	case ltOpaque: // formerly ltCoverRect
		// copy
		if(TVPIsTypeUsingAlpha(destlayertype))
			met = bmCopyOnAlpha;
		else if(TVPIsTypeUsingAddAlpha(destlayertype))
			met = bmCopyOnAddAlpha;
		else
			met = bmCopy;
		break;

	case ltAlpha: // formerly ltTransparent
		// alpha blend
		if(TVPIsTypeUsingAlpha(destlayertype))
			met = bmAlphaOnAlpha;
		else if(TVPIsTypeUsingAddAlpha(destlayertype))
			met = bmAlphaOnAddAlpha;
		else
			met = bmAlpha;
		break;

	case ltAdditive:
		// additive blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
			// hda = true if destination has alpha
			// ( preserving mask )
		met = bmAdd;
		break;

	case ltSubtractive:
		// subtractive blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmSub;
		break;

	case ltMultiplicative:
		// multiplicative blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmMul;
		break;

	case ltDodge:
		// color dodge ( "Ooi yaki" in Japanese )
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmDodge;
		break;

	case ltDarken:
		// darken blend (select lower luminosity)
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmDarken;
		break;

	case ltLighten:
		// lighten blend (select higher luminosity)
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmLighten;
		break;

	case ltScreen:
		// screen multiplicative blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmScreen;
		break;

	case ltAddAlpha:
		// alpha blend
		if(TVPIsTypeUsingAlpha(destlayertype))
			met = bmAddAlphaOnAlpha;
		else if(TVPIsTypeUsingAddAlpha(destlayertype))
			met = bmAddAlphaOnAddAlpha;
		else
			met = bmAddAlpha;
		break;

	case ltPsNormal:
		// Photoshop compatible normal blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsNormal;
		break;

	case ltPsAdditive:
		// Photoshop compatible additive blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsAdditive;
		break;

	case ltPsSubtractive:
		// Photoshop compatible subtractive blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsSubtractive;
		break;

	case ltPsMultiplicative:
		// Photoshop compatible multiplicative blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsMultiplicative;
		break;

	case ltPsScreen:
		// Photoshop compatible screen blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsScreen;
		break;

	case ltPsOverlay:
		// Photoshop compatible overlay blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsOverlay;
		break;

	case ltPsHardLight:
		// Photoshop compatible hard light blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsHardLight;
		break;

	case ltPsSoftLight:
		// Photoshop compatible soft light blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsSoftLight;
		break;

	case ltPsColorDodge:
		// Photoshop compatible color dodge blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsColorDodge;
		break;

	case ltPsColorDodge5:
		// Photoshop 5.x compatible color dodge blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsColorDodge5;
		break;

	case ltPsColorBurn:
		// Photoshop compatible color burn blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsColorBurn;
		break;

	case ltPsLighten:
		// Photoshop compatible compare (lighten) blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsLighten;
		break;

	case ltPsDarken:
		// Photoshop compatible compare (darken) blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsDarken;
		break;

	case ltPsDifference:
		// Photoshop compatible difference blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsDifference;
		break;

	case ltPsDifference5:
		// Photoshop 5.x compatible difference blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsDifference5;
		break;

	case ltPsExclusion:
		// Photoshop compatible exclusion blend
		hda = hda || TVPIsTypeUsingAlphaChannel(destlayertype);
		met = bmPsExclusion;
		break;

	default:
		return;
	}

	dest->Blt(destx, desty, src, srcrect, met, opacity, hda);
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DrawSelf(tTVPDrawable *target, tTVPRect &pr,
	tTVPRect &cr)
{
	if(!MainImage)
	{
		if(DisplayType == ltOpaque)
		{
			// fill destination with specified color
			tTVPBaseTexture *temp = tTVPTempBitmapHolder::GetTemp(
								cr.get_width(),
								cr.get_height());
			try
			{
				// do transition
				tTVPRect bitmaprect = cr;
				bitmaprect.set_offsets(0, 0);
				CopySelf(temp, 0, 0, bitmaprect); // this fills temp with neutral color

				// send completion message
				target->DrawCompleted(pr, temp, bitmaprect, DisplayType, Opacity);
			}
			catch(...)
			{
				tTVPTempBitmapHolder::FreeTemp();
				throw;
			}
			tTVPTempBitmapHolder::FreeTemp();
		}
		return;
	}

	// draw self MainImage(only) to target
	cr.add_offsets(-ImageLeft, -ImageTop);

	if(InTransition && !TransWithChildren && DivisibleTransHandler)
	{
		// transition without children

		// allocate temporary bitmap
		tTVPBaseTexture *temp = tTVPTempBitmapHolder::GetTemp(
							cr.get_width(),
							cr.get_height());
		try
		{
			// do transition
			tTVPRect bitmaprect = cr;
			bitmaprect.set_offsets(0, 0);
			DoDivisibleTransition(temp, 0, 0, cr);

			// send completion message
			target->DrawCompleted(pr, temp, bitmaprect, DisplayType, Opacity);
		}
		catch(...)
		{
			tTVPTempBitmapHolder::FreeTemp();
			throw;
		}
		tTVPTempBitmapHolder::FreeTemp();
	}
	else
	{
		target->DrawCompleted(pr, MainImage, cr, DisplayType, Opacity);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CopySelfForRect(iTVPBaseBitmap *dest, tjs_int destx, tjs_int desty,
	const tTVPRect &srcrect)
{
	// copy self image to the target
	tTVPRect cr = srcrect;
	cr.add_offsets(-ImageLeft, -ImageTop);

	if(InTransition && !TransWithChildren && DivisibleTransHandler)
	{
		// transition without children
		DoDivisibleTransition(dest, destx, desty, cr);
	}
	else
	{
		if(MainImage)
		{
			dest->CopyRect(destx, desty, MainImage, cr);
		}
		else
		{
			// main image does not exist
			// fill destination with TransparentColor
			// (this need to be transparent, so we do not use NeutralColor which can be
			//  set by the user unless the DisplayType is ltOpaque)
			dest->Fill(tTVPRect(destx, desty,
					destx + cr.get_width(), desty + cr.get_height()),
					DisplayType == ltOpaque ? NeutralColor : TransparentColor);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CopySelf(iTVPBaseBitmap *dest, tjs_int destx, tjs_int desty,
	const tTVPRect &r)
{
	const tTVPRect &uer = UpdateExcludeRect;
	if(uer.is_empty())
	{
		CopySelfForRect(dest, destx, desty, r);
	}
	else
	{
		if(uer.top <= r.top && uer.bottom >= r.bottom)
		{
			if(uer.left > r.left && uer.right < r.right)
			{
				// split into two
				tTVPRect r2 = r;
				r2.right = uer.left;
				CopySelfForRect(dest, destx, desty, r2);
				r2.right = r.right;
				r2.left = uer.right;
				CopySelfForRect(dest, destx + (r2.left - r.left), desty, r2);
			}
			else if(r.left >= uer.left && r.right <= uer.right)
			{
				;// nothing to do
			}
			else if(r.right <= uer.left || r.left >= uer.right)
			{
				CopySelfForRect(dest, destx, desty, r);
			}
			else if(r.right > uer.left && r.right <= uer.right)
			{
				tTVPRect r2 = r;
				r2.right = uer.left;
				CopySelfForRect(dest, destx, desty, r2);
			}
			else if(r.left >= uer.left && r.left < uer.left)
			{
				tTVPRect r2 = r;
				r2.left = uer.right;
				CopySelfForRect(dest, destx + (r2.left - r.left), desty, r2);
			}
			else
			{
				CopySelfForRect(dest, destx, desty, r);
			}

		}
		else
		{
			CopySelfForRect(dest, destx, desty, r);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::EffectImage(iTVPBaseBitmap *dest, const tTVPRect & destrect)
{
	if(Type == ltFilter)
	{
		// TODO: do filtering
	}
	else if(Type == ltEffect)
	{
		// TODO: do effect
	}
}
void tTJSNI_BaseLayer::Draw_GPU(tTVPDrawable *target, int x, int y, const tTVPRect &r, bool visiblecheck) {
    if(visiblecheck && !IsSeen()) return;

	tTVPRect rect;
    if(!TVPIntersectRect(&rect, r, Rect)) return; // no intersection
	x += rect.left - r.left;
	y += rect.top - r.top;

	tTVPRect rctar(rect);
	rctar.set_offsets(x, y);

    CurrentDrawTarget = target;

    ParentRectToChildRect(rect); // to this layer based axis

	// process drawing
	DirectTransferToParent = false;

	// caching is not enabled

	if (Opacity < 255 || (InTransition && TransWithChildren))
	{
		// rearrange pipe line for transition
		if (InTransition && TransWithChildren) {
			TransDrawable.Init(this, target);
			target = &TransDrawable;
		}
		if (GetVisibleChildrenCount() == 0) {
			DrawSelf(target, rctar, rect);
		} else {
			// rearrange pipe line for transition
			bool useTemp = false;
			if (GetCacheEnabled()) {
				UpdateBitmapForChild = CacheBitmap;
			} else {
				useTemp = true;
				UpdateBitmapForChild = tTVPTempBitmapHolder::GetTemp(
					Rect.get_width(),
					Rect.get_height());
			}
			tTVPRect rectForChild(0, 0, Rect.get_width(), Rect.get_height());

			// copy self image to UpdateBitmapForChild
			if (MainImage != NULL) {
// 				if (UpdateExcludeRect.top <= rect.top && UpdateExcludeRect.bottom >= rect.bottom &&
// 					rect.left >= UpdateExcludeRect.left && rect.right <= UpdateExcludeRect.right) {
// 				} else
					CopySelfForRect(UpdateBitmapForChild, 0, 0, rectForChild); // transfer self image
			}

			TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
			{
				// for each child...

				// visible check
				if (!child->Visible) continue;

				// intersection check
				if (!TVPIntersectRect(&UpdateRectForChild, rectForChild, child->Rect))
					continue;

				// setup UpdateOfsX/Y UpdateRectForChildOfsX/Y
				UpdateOfsX = 0;
				UpdateOfsY = 0;
 				UpdateRectForChildOfsX = UpdateRectForChild.left - child->Rect.left;
 				UpdateRectForChildOfsY = UpdateRectForChild.top - child->Rect.top;

				// call children's "Draw" method
				child->Draw_GPU((tTVPDrawable*)this, UpdateRectForChild.left, UpdateRectForChild.top, UpdateRectForChild);
			}
			TVP_LAYER_FOR_EACH_CHILD_END
			// rect.set_offsets(0, 0);
			target->DrawCompleted(rctar, UpdateBitmapForChild, rect, DisplayType, Opacity);
			if (useTemp) tTVPTempBitmapHolder::FreeTemp();
		}
	} else {
		if (GetVisibleChildrenCount() == 0) {
			DrawSelf(target, rctar, rect);
		} else {
			DrawnRegion.Clear();
			// send completion message to the target

// 			if (UpdateExcludeRect.top <= rect.top && UpdateExcludeRect.bottom >= rect.bottom &&
// 				rect.left >= UpdateExcludeRect.left && rect.right <= UpdateExcludeRect.right) {
// 			} else 
			{
				tTVPRect rc(rect);
				DrawSelf(target, rctar, rc);
			}

			TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
			{
				// for each child...

				// visible check
				if (!child->Visible) continue;

				// intersection check
				tTVPRect chrect;
				if (!TVPIntersectRect(&chrect, rect, child->Rect))
					continue;

				// call children's "Draw" method
				child->Draw_GPU(target, x, y, rect);
			}
			TVP_LAYER_FOR_EACH_CHILD_END
		}
	}

    CurrentDrawTarget = NULL;
}

//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalDrawNoCache_CPU(tTVPDrawable *target, const tTVPRect &rect)
{
    bool totalopaque = (DisplayType == ltOpaque && Opacity == 255);
    if(GetVisibleChildrenCount() == 0)
    {
        // no visible children; no action needed
        tTVPRect pr = rect;
        pr.add_offsets(Rect.left, Rect.top);
        tTVPRect cr = rect;
        DrawSelf(target, pr, cr);
    }
    else
    {
        // has at least one visible child
        const tTVPComplexRect & overlapped = GetOverlappedRegion();
        const tTVPComplexRect & exposed = GetExposedRegion();

        // process overlapped region
        // clear DrawnRegion
        tTVPComplexRect::tIterator it;


        DrawnRegion.Clear();


        it = overlapped.GetIterator();
        while(it.Step())
        {
            tTVPRect cr(*it);

            // intersection check
            if(!TVPIntersectRect(&cr, cr, rect)) continue;


            tTVPRect updaterectforchild;
            bool tempalloc = false;

            // setup UpdateBitmapForChild and "updaterectforchild"
            if(totalopaque)
            {
                // this layer is totally opaque
                UpdateBitmapForChild = target->GetDrawTargetBitmap(
                    cr, updaterectforchild);
            }
            else
            {
                // this layer is transparent

                // retrieve temporary bitmap
                UpdateBitmapForChild = tTVPTempBitmapHolder::GetTemp(
                    cr.get_width(),
                    cr.get_height());
                tempalloc = true;
                updaterectforchild.left = 0;
                updaterectforchild.top = 0;
                updaterectforchild.right = cr.get_width();
                updaterectforchild.bottom = cr.get_height();
            }

            try
            {
                // copy self image to the target
                CopySelf(UpdateBitmapForChild,
                    updaterectforchild.left, updaterectforchild.top, cr);

                TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
                {
                    // for each child...

                    // visible check
                    if(!child->Visible) continue;

                    // intersection check
                    tTVPRect chrect;
                    if(!TVPIntersectRect(&chrect, cr, child->Rect))
                        continue;

                    // setup UpdateRectForChild
                    tjs_int ox = chrect.left - cr.left;
                    tjs_int oy = chrect.top - cr.top;

                    UpdateRectForChild = updaterectforchild;
                    UpdateRectForChild.add_offsets(ox, oy);
                    UpdateRectForChildOfsX = chrect.left - child->Rect.left;
                    UpdateRectForChildOfsY = chrect.top - child->Rect.top;

                    // setup UpdateOfsX, UpdateOfsY
                    UpdateOfsX = cr.left - updaterectforchild.left;
                    UpdateOfsY = cr.top - updaterectforchild.top;

                    // call children's "Draw" method
                    child->Draw((tTVPDrawable*)this, chrect, true);
                }
                TVP_LAYER_FOR_EACH_CHILD_END

            }
            catch(...)
            {
                if(tempalloc) tTVPTempBitmapHolder::FreeTemp();
                throw;
            }

            // send completion message to the target
            if(DisplayType != ltBinder)
            {
                tTVPRect pr = cr;
                pr.add_offsets(Rect.left, Rect.top);
                target->DrawCompleted(pr, UpdateBitmapForChild, updaterectforchild,
                    DisplayType, Opacity);
            }

            // release temporary bitmap
            if(tempalloc) tTVPTempBitmapHolder::FreeTemp();


        } // overlapped region


        // process exposed region
        DirectTransferToParent = true; // this flag is used only when MainImage == NULL


        it = exposed.GetIterator();
        while(it.Step())
        {
            tTVPRect cr(*it);

            // intersection check
            if(!TVPIntersectRect(&cr, cr, rect)) continue;

            if(MainImage != NULL)
            {
                // send completion message to the target
                tTVPRect pr = cr;
                pr.add_offsets(Rect.left, Rect.top);
                DrawSelf(target, pr, cr);
            }
            else
            {
                // call children's "Draw" method

                tTVPRect cr(*it);

                // intersection check
                if(!TVPIntersectRect(&cr, cr, rect)) continue;


                tTVPRect updaterectforchild;

                TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
                {
                    // for each child...

                    // visible check
                    if(!child->Visible) continue;

                    // intersection check
                    tTVPRect chrect;
                    if(!TVPIntersectRect(&chrect, cr, child->Rect))
                        continue;

                    // call children's "Draw" method
                    child->Draw((tTVPDrawable*)this, chrect, true);
                }
                TVP_LAYER_FOR_EACH_CHILD_END
            }
        }
        DirectTransferToParent = false;
    } // has visible children/no visible children
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::Draw(tTVPDrawable *target, const tTVPRect &r, bool visiblecheck)
{
	// process updating pipe line.
	// draw the layer content to "target".
	// "r" is a rectangle to be drawn in the parent's coordinates.
	// parent has responsibility for piling the image returned from children.


	if(visiblecheck && !IsSeen()) return;

	tTVPRect rect = r;
	if(!TVPIntersectRect(&rect, rect, Rect)) return; // no intersection


	CurrentDrawTarget = target;

	ParentRectToChildRect(rect);

	if(InTransition && TransWithChildren)
	{
		// rearrange pipe line for transition
		TransDrawable.Init(this, target);
		target = &TransDrawable;
	}

	// process drawing
	DirectTransferToParent = false;
	bool totalopaque = (DisplayType == ltOpaque && Opacity == 255);

	if(GetCacheEnabled() &&
		!(InTransition && !TransWithChildren && DivisibleTransHandler))
	{
		// process must-recalc region

		tTVPComplexRect::tIterator it = CacheRecalcRegion.GetIterator();
		while(it.Step())
		{
			tTVPRect cr(*it);

			// intersection check
			if(!TVPIntersectRect(&cr, cr, rect)) continue;

			// clear DrawnRegion
			DrawnRegion.Clear();

			// setup UpdateBitmapForChild
			UpdateBitmapForChild = CacheBitmap;

			// copy self image to UpdateBitmapForChild
			if(MainImage != NULL)
			{
				CopySelf(UpdateBitmapForChild,
						cr.left,
						cr.top, cr); // transfer self image
			}

			TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
			{
				// for each child...

				// intersection check
				if(!TVPIntersectRect(&UpdateRectForChild, cr, child->Rect))
					continue;

				// setup UpdateOfsX/Y UpdateRectForChildOfsX/Y
				UpdateOfsX = 0;
				UpdateOfsY = 0;
				UpdateRectForChildOfsX = UpdateRectForChild.left - child->Rect.left;
				UpdateRectForChildOfsY = UpdateRectForChild.top - child->Rect.top;

				// call children's "Draw" method
				child->Draw((tTVPDrawable*)this, UpdateRectForChild, true);
			}
			TVP_LAYER_FOR_EACH_CHILD_END

			// special optimazation for MainImage == NULL

			if(MainImage == NULL)
			{
				tTVPComplexRect nr;
				nr.Or(cr);
				nr.Sub(DrawnRegion);
				tTVPComplexRect::tIterator it = nr.GetIterator();
				while(it.Step())
				{
					tTVPRect r(*it);
					CopySelf(UpdateBitmapForChild,
							r.left,
							r.top, r);
							// CopySelf of MainImage == NULL actually
							// fills target rectangle with full transparency
				}
			}

		}

		CacheRecalcRegion.Sub(rect);

		if(CacheRecalcRegion.GetCount() > TVP_CACHE_UNITE_LIMIT)
			CacheRecalcRegion.Unite();

		// at this point, the cache bitmap should be completed

		// send completion message to the target
		tTVPRect pr = rect;
		pr.add_offsets(Rect.left, Rect.top);
		target->DrawCompleted(pr, CacheBitmap, rect, DisplayType, Opacity);
	}
	else
	{
		// caching is not enabled

		if(GetVisibleChildrenCount() == 0)
		{
			// no visible children; no action needed
			tTVPRect pr = rect;
			pr.add_offsets(Rect.left, Rect.top);
			tTVPRect cr = rect;
			DrawSelf(target, pr, cr);
		}
		else
		{
			// has at least one visible child
			const tTVPComplexRect & overlapped = GetOverlappedRegion();
			const tTVPComplexRect & exposed = GetExposedRegion();

			// process overlapped region
			// clear DrawnRegion
			tTVPComplexRect::tIterator it;


			DrawnRegion.Clear();


			it = overlapped.GetIterator();
			while(it.Step())
			{
				tTVPRect cr(*it);

				// intersection check
				if(!TVPIntersectRect(&cr, cr, rect)) continue;


				tTVPRect updaterectforchild;
				bool tempalloc = false;

				// setup UpdateBitmapForChild and "updaterectforchild"
				if(totalopaque)
				{
					// this layer is totally opaque
					UpdateBitmapForChild = target->GetDrawTargetBitmap(
						cr, updaterectforchild);
				}
				else
				{
					// this layer is transparent

					// retrieve temporary bitmap
					UpdateBitmapForChild = tTVPTempBitmapHolder::GetTemp(
							cr.get_width(),
							cr.get_height());
					tempalloc = true;
					updaterectforchild.left = 0;
					updaterectforchild.top = 0;
					updaterectforchild.right = cr.get_width();
					updaterectforchild.bottom = cr.get_height();
				}

				try
				{
					// copy self image to the target
					CopySelf(UpdateBitmapForChild,
						updaterectforchild.left, updaterectforchild.top, cr);

					TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
					{
						// for each child...

						// visible check
						if(!child->Visible) continue;

						// intersection check
						tTVPRect chrect;
						if(!TVPIntersectRect(&chrect, cr, child->Rect))
							continue;

						// setup UpdateRectForChild
						tjs_int ox = chrect.left - cr.left;
						tjs_int oy = chrect.top - cr.top;

						UpdateRectForChild = updaterectforchild;
						UpdateRectForChild.add_offsets(ox, oy);
						UpdateRectForChildOfsX = chrect.left - child->Rect.left;
						UpdateRectForChildOfsY = chrect.top - child->Rect.top;

						// setup UpdateOfsX, UpdateOfsY
						UpdateOfsX = cr.left - updaterectforchild.left;
						UpdateOfsY = cr.top - updaterectforchild.top;

						// call children's "Draw" method
						child->Draw((tTVPDrawable*)this, chrect, true);
					}
					TVP_LAYER_FOR_EACH_CHILD_END

				}
				catch(...)
				{
					if(tempalloc) tTVPTempBitmapHolder::FreeTemp();
					throw;
				}

				// send completion message to the target
				if(DisplayType != ltBinder)
				{
					tTVPRect pr = cr;
					pr.add_offsets(Rect.left, Rect.top);
					target->DrawCompleted(pr, UpdateBitmapForChild, updaterectforchild,
						DisplayType, Opacity);
				}

				// release temporary bitmap
				if(tempalloc) tTVPTempBitmapHolder::FreeTemp();


			} // overlapped region


			// process exposed region
			DirectTransferToParent = true; // this flag is used only when MainImage == NULL


			it = exposed.GetIterator();
			while(it.Step())
			{
				tTVPRect cr(*it);

				// intersection check
				if(!TVPIntersectRect(&cr, cr, rect)) continue;

				if(MainImage != NULL)
				{
					// send completion message to the target
					tTVPRect pr = cr;
					pr.add_offsets(Rect.left, Rect.top);
					DrawSelf(target, pr, cr);
				}
				else
				{
					// call children's "Draw" method

					tTVPRect cr(*it);

					// intersection check
					if(!TVPIntersectRect(&cr, cr, rect)) continue;


					tTVPRect updaterectforchild;

					TVP_LAYER_FOR_EACH_CHILD_BEGIN(child)
					{
						// for each child...

						// visible check
						if(!child->Visible) continue;

						// intersection check
						tTVPRect chrect;
						if(!TVPIntersectRect(&chrect, cr, child->Rect))
							continue;

						// call children's "Draw" method
						child->Draw((tTVPDrawable*)this, chrect, true);
					}
					TVP_LAYER_FOR_EACH_CHILD_END

				}
			}
			DirectTransferToParent = false;
		} // has visible children/no visible children
	} // cache enabled/disabled

	CurrentDrawTarget = NULL;
}
//---------------------------------------------------------------------------
tTVPBaseTexture * tTJSNI_BaseLayer::GetDrawTargetBitmap(const tTVPRect &rect,
	tTVPRect &cliprect)
{
	// called from children to get the image buffer drawn to.
	if(DisplayType == ltBinder || (MainImage == NULL && DirectTransferToParent))
	{
		tTVPRect _rect(rect);
		_rect.add_offsets(Rect.left, Rect.top);
		tTVPBaseTexture * bmp = CurrentDrawTarget->GetDrawTargetBitmap(_rect, cliprect);
		return bmp;
	}
	tjs_int w = rect.get_width();
	tjs_int h = rect.get_height();
	if(UpdateRectForChild.get_width() < w || UpdateRectForChild.get_height() < h)
		TVPThrowExceptionMessage(TVPInternalError);
	cliprect = UpdateRectForChild;
	cliprect.add_offsets(rect.left - UpdateRectForChildOfsX,
		rect.top - UpdateRectForChildOfsY);
	return UpdateBitmapForChild;
}
//---------------------------------------------------------------------------
tTVPLayerType tTJSNI_BaseLayer::GetTargetLayerType()
{
	if(DisplayType == ltBinder) // return parent's display layer type when DisplayType == ltBinder
		return Parent ? Parent->DisplayType : ltOpaque;
	return DisplayType;
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DrawCompleted(const tTVPRect &destrect,
	tTVPBaseTexture *bmp,
	const tTVPRect &cliprect,
	tTVPLayerType type, tjs_int opacity)
{
	// called from children to notify that the image drawing is completed.
	// blend the image to the target unless bmp is the same as UpdateBitmapForChild.
	if(DisplayType == ltBinder || (MainImage == NULL && DirectTransferToParent))
	{
		tTVPRect _destrect(destrect);
		tTVPRect _cliprect(cliprect);
		_destrect.add_offsets(Rect.left, Rect.top);
		CurrentDrawTarget->DrawCompleted(_destrect, bmp, _cliprect, type, opacity);
		return;
	}

	if(bmp != UpdateBitmapForChild)
	{
		if(MainImage == NULL)
		{
			// special optimization for MainImage == NULL
			// (all the layer face is treated as transparent)
			tTVPComplexRect nr; // new region
			nr.Or(destrect);
			nr.Sub(DrawnRegion);
			tTVPComplexRect opr; // operation region
			// now nr is a client region which is not overlapped by children
			// at this time
			if(DisplayType == type && opacity == 255)
			{
				// DisplayType == type and full opacity
				// just copy the target bitmap
				tTVPComplexRect::tIterator it = nr.GetIterator();
				while(it.Step())
				{
					tTVPRect r(*it);
					tTVPRect sr;
					sr.left = cliprect.left + (r.left - destrect.left);
					sr.top  = cliprect.top  + (r.top  - destrect.top );
					sr.right = sr.left + r.get_width();
					sr.bottom = sr.top + r.get_height();

					UpdateBitmapForChild->CopyRect(
						r.left - UpdateOfsX, r.top - UpdateOfsY,
						bmp, sr);
				}
				// calculate operation region
				opr.Or(destrect);
				opr.Sub(nr);
			}
			else
			{
				// set operation region
				tTVPComplexRect::tIterator it = nr.GetIterator();
				while(it.Step())
				{
					tTVPRect r(*it);
					r.add_offsets(-UpdateOfsX, -UpdateOfsY);
					// fill r with transparent color
					CopySelf(UpdateBitmapForChild,
							r.left,
							r.top, r);
							// CopySelf of MainImage == NULL actually
							// fills target rectangle with full transparency
				}
				opr.Or(destrect);
			}

			// operate r
			tTVPComplexRect::tIterator it = opr.GetIterator();
			while(it.Step())
			{
				tTVPRect r(*it);
				tTVPRect sr;
				sr.left = cliprect.left + (r.left - destrect.left);
				sr.top  = cliprect.top  + (r.top  - destrect.top );
				sr.right = sr.left + r.get_width();
				sr.bottom = sr.top + r.get_height();

				BltImage(UpdateBitmapForChild, DisplayType,
					r.left - UpdateOfsX,
					r.top - UpdateOfsY,
					bmp, sr, type, opacity);
			}

			// update DrawnRegion
			DrawnRegion.Or(destrect);
		}
		else
		{
			BltImage(UpdateBitmapForChild, DisplayType,
				destrect.left - UpdateOfsX,
				destrect.top - UpdateOfsY,
				bmp, cliprect, type, opacity);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalComplete2(tTVPComplexRect & updateregion,
	tTVPDrawable *drawable)
{
//--- querying phase

	// search ltOpaque, not to draw region behind them.
	if(Manager) Manager->QueryUpdateExcludeRect();

//--- drawing phase

	// split the region to some stripes to utilize the CPU's memory
	// caching.

	//tjs_int i;
	tTVPComplexRect::tIterator it = updateregion.GetIterator();
	while(it.Step())
	{
		tTVPRect r(*it);

		// Add layer offset because Draw() accepts the position in
		// *parent* 's coordinates.
		r.add_offsets(Rect.left, Rect.top);

		if(TVPGraphicSplitOperationType != gsotNone)
		{
			// compute optimum height of the stripe
			tjs_int oh;

			if(GetVisibleChildrenCount() || InTransition)
			{
				// split
				tjs_int rw = r.get_width();
				if(rw < 40) oh = 256;
				else if(rw < 80) oh = 128;
				else if(rw < 160) oh = 64;
				else if(rw < 320) oh = 32;
				else oh = 16; // 2 lines per core in modern 8 cores cpu
			}
			else
			{
				// no need to split
				oh = r.get_height();
			}

			// split to some stripes
			tjs_int y;
			tTVPRect opr;
			opr.left = r.left;
			opr.right = r.right;
			if(TVPGraphicSplitOperationType == gsotInterlace)
			{
				// interlaced split
				for(y = r.top; y < r.bottom; y+= oh*2)
				{
					opr.top = y;
					opr.bottom = (y+oh < r.bottom) ? y+oh: r.bottom;

					// call "Draw" to draw to the window
					Draw(drawable, opr, false);
				}
				for(y = r.top + oh; y < r.bottom; y+= oh*2)
				{
					opr.top = y;
					opr.bottom = (y+oh < r.bottom) ? y+oh: r.bottom;

					// call "Draw" to draw to the window
					Draw(drawable, opr, false);
				}
			}
			else if(TVPGraphicSplitOperationType == gsotSimple)
			{
				// non-interlaced
				for(y = r.top; y < r.bottom; y+=oh)
				{
					opr.top = y;
					opr.bottom = (y+oh < r.bottom) ? y+oh: r.bottom;

					// call "Draw" to draw to the window
					Draw(drawable, opr, false);
				}
			}
			else if(TVPGraphicSplitOperationType == gsotBiDirection)
			{
				// bidirection
				static int direction = 0;
				if(direction & 1)
				{
					for(y = r.top; y < r.bottom; y+=oh)
					{
						opr.top = y;
						opr.bottom = (y+oh < r.bottom) ? y+oh: r.bottom;

						// call "Draw" to draw to the window
						Draw(drawable, opr, false);
					}
				}
				else
				{
					y = r.bottom - oh;
					if(y < r.top) y = r.top;
					while(1)
					{
						opr.top = (y < r.top ? r.top : y);
						opr.bottom = (y+oh < r.bottom) ? y+oh: r.bottom;

						if(opr.bottom <= r.top) break;

						// call "Draw" to draw to the window
						Draw(drawable, opr, false);

						y-=oh;
					}
				}
				direction++;
			}
		}
		else
		{
			// don't split scanlines
			Draw(drawable, r, false);
		}
	}

	updateregion.Clear();
}

//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalComplete2_GPU(tTVPRect updateregion, tTVPDrawable *drawable)
{
	if (Manager) Manager->QueryUpdateExcludeRect();
	updateregion.add_offsets(Rect.left, Rect.top);
	Draw_GPU(drawable, 0, 0, updateregion, false);
}

//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalComplete(tTVPComplexRect & updateregion,
	tTVPDrawable *drawable)
{
	BeforeCompletion();

	// at this point, final update region (in this completion) is determined
	InCompletion = true;

	if (IsGPU()) {
		InternalComplete2_GPU(updateregion.GetBound(), drawable);
	} else {
		InternalComplete2(updateregion, drawable);
	}

	InCompletion = false;
	AfterCompletion();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::CompleteForWindow(tTVPDrawable *drawable)
{
	BeforeCompletion();

	if(Manager) Manager->NotifyUpdateRegionFixed();

	InCompletion = true;

	if(Manager) Manager->GetLayerTreeOwner()->StartBitmapCompletion(Manager);
	try
	{
		if (IsGPU()) {
			InternalComplete2_GPU(Rect, drawable);
		} else {
			InternalComplete2(Manager->GetUpdateRegionForCompletion(), drawable);
		}
	}
	catch(...)
	{
		if(Manager) Manager->GetLayerTreeOwner()->EndBitmapCompletion(Manager);
		throw;
	}
	if(Manager) Manager->GetLayerTreeOwner()->EndBitmapCompletion(Manager);

	InCompletion = false;
	AfterCompletion();

}
//---------------------------------------------------------------------------
tTVPBaseTexture * tTJSNI_BaseLayer::Complete(const tTVPRect & rect)
{
	class tCompleteDrawable : public tTVPDrawable
	{
	protected:
		tTVPBaseTexture * Bitmap;
		tTVPRect BitmapRect;
		tTVPLayerType LayerType;
	public:
		tCompleteDrawable(tTVPBaseTexture *bmp, tTVPLayerType layertype)
			: Bitmap(bmp), LayerType(layertype) {};

		tTVPBaseTexture * GetDrawTargetBitmap(const tTVPRect &rect,
			tTVPRect &cliprect)
			{ cliprect = rect; return Bitmap; }
		tTVPLayerType GetTargetLayerType() { return LayerType; }
		virtual void DrawCompleted(const tTVPRect &destrect,
			tTVPBaseTexture *bmp, const tTVPRect &cliprect,
			tTVPLayerType type, tjs_int opacity) override
		{
			if(bmp != Bitmap)
			{
				Bitmap->CopyRect(destrect.left, destrect.top,
					bmp, cliprect);
			}
		}
	};

	class tCompleteDrawable_GPU : public tCompleteDrawable {
	public:
		tCompleteDrawable_GPU(tTVPBaseTexture *bmp, tTVPLayerType layertype)
			: tCompleteDrawable(bmp, layertype) {
			bmp->Fill(tTVPRect(0, 0, bmp->GetWidth(), bmp->GetHeight()), layertype == ltOpaque ? 0xFF000000 : 0);
		};

		virtual void DrawCompleted(const tTVPRect &destrect,
			tTVPBaseTexture *bmp, const tTVPRect &cliprect,
			tTVPLayerType type, tjs_int opacity) override {
			if (bmp != Bitmap) {
				BltImage(Bitmap, LayerType, destrect.left, destrect.top, bmp, cliprect, type, opacity, LayerType == ltOpaque);
			}
		}
	};

	// complete given rectangle of cache.

	if(!GetCacheEnabled()) return NULL;
		// caller must ensure that the caching is enabled

	if(GetVisibleChildrenCount() == 0 && ImageLeft == 0 && ImageTop == 0 &&
		MainImage->GetWidth() == GetWidth() && MainImage->GetHeight() == GetHeight())
	{
		// the layer has no visible children
		// and entire of the bitmap is the visible area.
		// simply returns main image
		return MainImage;
	}

	if(CacheRecalcRegion.GetCount() == 0)
	{
		// the layer has not region to reconstruct
		return CacheBitmap;
	}

	tTVPComplexRect ur;
	ur.Or(rect);
	if (IsGPU()) {
		tCompleteDrawable_GPU drawable(CacheBitmap, DisplayType);
		InternalComplete(ur, &drawable); // complete cache
	} else {
		// create drawable object
		tCompleteDrawable drawable(CacheBitmap, DisplayType);

		// complete
		InternalComplete(ur, &drawable); // complete cache
	}
	return CacheBitmap;
}
//---------------------------------------------------------------------------
tTVPBaseTexture * tTJSNI_BaseLayer::Complete()
{
	// complete entire area of the layer
	tTVPRect r;
	r.left = 0;
	r.top = 0;
	r.right = Rect.get_width();
	r.bottom = Rect.get_height();

	return Complete(r);
}
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// transition management
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StartTransition(const ttstr &name, bool withchildren,
	tTJSNI_BaseLayer * transsource, tTJSVariantClosure options)
{
	// start transition

	// is current transition processing?
	if(InTransition)
	{
		TVPThrowExceptionMessage(TVPCurrentTransitionMustBeStopping);
	}

	if(transsource && transsource->TransSrc == this)
	{
		TVPThrowExceptionMessage(TVPTransitionMutualSource);
	}

	// pointers which must be released at last...
	iTVPTransHandlerProvider *pro = NULL;
	tTVPSimpleOptionProvider *sop = NULL;
	iTVPBaseTransHandler * handler = NULL;

	try
	{
		// find transition handler
		pro = TVPFindTransHandlerProvider(name);
			// this may raise an exception

		// check selfupdate member of 'options'
		tTJSVariant var;
		TransSelfUpdate = false;
		static ttstr selfupdate_name(TJS_W("selfupdate"));
		if(TJS_SUCCEEDED(options.PropGet(0, selfupdate_name.c_str(),
			selfupdate_name.GetHint(), &var, NULL)))
		{
			if(var.Type() != tvtVoid)
				TransSelfUpdate = 0!=(tjs_int)var;
					// selfupdate member found
		}

		// check callback member of 'options'
		TransTickCallback = tTJSVariantClosure(NULL, NULL);
		static ttstr callback_name(TJS_W("callback"));
		UseTransTickCallback = false;
		if(TJS_SUCCEEDED(options.PropGet(0, callback_name.c_str(),
			callback_name.GetHint(), &var, NULL)))
		{
			// selfupdate member found
			if(var.Type() != tvtVoid)
			{
				TransTickCallback = var.AsObjectClosure(); // AddRef() is performed here
				UseTransTickCallback = true;
			}
		}


		// create option provider
		sop = new tTVPSimpleOptionProvider(options);

		// notify starting of the transition to the provider
		tjs_error er = pro->StartTransition(sop, &TVPSimpleImageProvider,
			DisplayType,
			withchildren ? GetWidth()  : MainImage->GetWidth(),
			withchildren ? GetHeight() : MainImage->GetHeight(),
			transsource?
				(withchildren?transsource->GetWidth() :
					transsource->MainImage->GetWidth())  :0,
			transsource?
				(withchildren?transsource->GetHeight() :
					transsource->MainImage->GetHeight()) :0,
			&TransType, &TransUpdateType, &handler);

		if(TJS_FAILED(er))
			TVPThrowExceptionMessage(TVPTransHandlerError,
				TJS_W("iTVPTransHandlerProvider::StartTransition failed"));

		if(TransUpdateType != tutDivisibleFade && TransUpdateType != tutDivisible
			&& TransUpdateType != tutGiveUpdate)
			TVPThrowExceptionMessage(TVPTransHandlerError, (const tjs_char*)TVPUnknownUpdateType );

		if(TransType != ttSimple && TransType != ttExchange)
			TVPThrowExceptionMessage(TVPTransHandlerError, (const tjs_char*)TVPUnknownTransitionType );

		// check update type
		if(TransUpdateType == tutGiveUpdate)
			TVPThrowExceptionMessage(TVPTransHandlerError, (const tjs_char*)TVPUnsupportedUpdateTypeTutGiveUpdate );
					// sorry for inconvinience
		if(TransType == ttExchange && !transsource)
			TVPThrowExceptionMessage(TVPSpecifyTransitionSource);

		// check wether the source and destination both have image
		if(!withchildren)
		{
			if(!MainImage)
				TVPThrowExceptionMessage(TVPTransitionSourceAndDestinationMustHaveImage);
			if(transsource && !transsource->MainImage)
				TVPThrowExceptionMessage(TVPTransitionSourceAndDestinationMustHaveImage);
		}

		// set to cache
		TransWithChildren = withchildren;
		if(TransWithChildren)
		{
			IncCacheEnabledCount();
			if(transsource) transsource->IncCacheEnabledCount();
		}

		// set to interrupt into updating/completion pipe line
		TransSrc = transsource;
		if(transsource) transsource->TransDest = this;

		// set transition handler
		if(TransUpdateType == tutDivisibleFade || TransUpdateType == tutDivisible)
			DivisibleTransHandler =
			static_cast<iTVPDivisibleTransHandler *>(handler);
		if(TransUpdateType == tutGiveUpdate) GiveUpdateTransHandler =
			static_cast<iTVPGiveUpdateTransHandler *>(handler);

		// hold destination and source objects
		TransDestObj = Owner;
		if(TransDestObj) TransDestObj->AddRef();
		if(transsource) TransSrcObj = transsource->Owner; else TransSrcObj = NULL;
		if(TransSrcObj) TransSrcObj->AddRef();

		// register to idle event handler
		TransIdleCallback.Owner = this;
		if(!TransSelfUpdate) TVPAddContinuousEventHook(&TransIdleCallback);

		// initial tick count
		TVPStartTickCount();
		if(UseTransTickCallback)
		{
			TransTick = 0;
			// initially 0
			// dummy calling StartProcess/EndProcess to notify initial tick count;
			// for first call with TransTick = 0, these method should not
			// return any error status here.
			if(DivisibleTransHandler)
			{
				DivisibleTransHandler->StartProcess(TransTick);
				DivisibleTransHandler->EndProcess();
			}
			else if(GiveUpdateTransHandler)
			{
				;// not yet implemented
			}
		}
		else
		{
			TransTick = GetTransTick();
		}

		// set flag
		InTransition = true;
		TransCompEventPrevented = false;

		// update
		Update(true);
	}
	catch(...)
	{
		if(pro) pro->Release();
		if(sop) sop->Release();
		if(handler) handler->Release();
		throw;
	}
	if(pro) pro->Release();
	if(sop) sop->Release();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InternalStopTransition()
{
	// stop transition
	if(InTransition)
	{
		InTransition = false;
		TransCompEventPrevented = false;

		// unregister idle event handler
		if(!TransSelfUpdate) TVPRemoveContinuousEventHook(&TransIdleCallback);

		// disable cache
		if(TransWithChildren)
		{
			DecCacheEnabledCount();
			if(TransSrc) TransSrc->DecCacheEnabledCount();
		}

		//
		// exchange the layer
		if(TransType == ttExchange)
		{
			tjs_int tl = this->Rect.left;
			tjs_int tt = this->Rect.top;
			tjs_int sl = TransSrc->Rect.left;
			tjs_int st = TransSrc->Rect.top;
			bool tv = this->GetVisible();
			bool sv = TransSrc->GetVisible();
			if(TransWithChildren)
			{
				// exchange with tree structure
				Exchange(TransSrc);
			}
			else
			{
				// exchange the layer and the target only
				Swap(TransSrc);
			}
			this->SetPosition(sl, st);
			TransSrc->SetPosition(tl, tt);
			this->SetVisible(sv);
			TransSrc->SetVisible(tv);
		}

		bool transsrcalive = false;
		if(TransSrc && !TransSrc->Shutdown) transsrcalive = true;

		if(TransSrc) TransSrc->TransDest = NULL;
		TransSrc = NULL;

		// release transition handler object
		if(DivisibleTransHandler)
			DivisibleTransHandler->Release(), DivisibleTransHandler = NULL;
		if(GiveUpdateTransHandler)
			GiveUpdateTransHandler->Release(), GiveUpdateTransHandler = NULL;


		// fire event
		if(Owner && !Shutdown && transsrcalive)
		{
			static ttstr eventname(TJS_W("onTransitionCompleted"));

			// fire SYNCHRONOUS event of "onTransitionCompleted"
			tTJSVariant param[2];
			param[0] = tTJSVariant(TransDestObj, TransDestObj);
			if(TransDestObj) TransDestObj->Release(), TransDestObj = NULL;
			param[1] = tTJSVariant(TransSrcObj, TransSrcObj);
			if(TransSrcObj) TransSrcObj->Release(), TransSrcObj = NULL;

			TVPPostEvent(Owner, Owner, eventname, 0,
				TVP_EPT_IMMEDIATE, 2, param);
		}

		// release destination and source objects
		if(TransDestObj) TransDestObj->Release(), TransDestObj = NULL;
		if(TransSrcObj) TransSrcObj->Release(), TransSrcObj = NULL;

		// release TransTickCallback
		TransTickCallback.Release(), TransTickCallback = tTJSVariantClosure(NULL, NULL);
	}

}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StopTransition()
{
	// stop the transition by manual
	InternalStopTransition();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::StopTransitionByHandler()
{
	// stopping of the transition caused by the handler
	if(!TVPEventDisabled)
	{
		// event dispatching is enabled
		InternalStopTransition();
	}
	else
	{
		// event dispatching is not enabled
		TransCompEventPrevented = true;
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::InvokeTransition(tjs_uint64 tick)
{
	if(!TransCompEventPrevented)
	{
		if(UseTransTickCallback)
			TransTick = GetTransTick();
		else
			TransTick = tick;
		if(!GetNodeVisible())
		{
			StopTransitionByHandler();
		}
		else
		{
			if(!MainImage && TransWithChildren &&
				TransUpdateType == tutDivisibleFade && DisplayType != ltOpaque)
			{
				// update only for child region
				UpdateAllChildren(true);
				if(TransSrc) TransSrc->UpdateAllChildren(true);
			}
			else
			{
				Update(true); // update without re-computing piled images
			}
		}
	}
	else
	{
		// transition complete event is prevented
		if(!TVPEventDisabled) // if event dispatching is enabled
			InternalStopTransition(); // stop the transition
	}
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::DoDivisibleTransition(iTVPBaseBitmap *dest,
	tjs_int dx, tjs_int dy, const tTVPRect &srcrect)
{
	// apply transition ( with no children ) over given target bitmap
	if(!InTransition || !DivisibleTransHandler) return;

	tTVPDivisibleData data;
	data.Left = srcrect.left;
	data.Top = srcrect.top;
	data.Width = srcrect.get_width();
	data.Height = srcrect.get_height();

	// src1
	if(!SrcSLP)
		SrcSLP = new tTVPScanLineProviderForBaseBitmap(MainImage);
	else
		SrcSLP->Attach(MainImage);
	data.Src1 = SrcSLP;
	data.Src1Left = srcrect.left;
	data.Src1Top = srcrect.top;
	ImageModified = true;

	// src2
	if(TransSrc)
	{
		// source available
		if(!TransSrc->SrcSLP)
			TransSrc->SrcSLP =
				new tTVPScanLineProviderForBaseBitmap(TransSrc->MainImage);
		else
			TransSrc->SrcSLP->Attach(TransSrc->MainImage);

		data.Src2 = TransSrc->SrcSLP;
		data.Src2Left = srcrect.left;
		data.Src2Top = srcrect.top;
	}

	// dest
	if(!DestSLP)
		DestSLP = new tTVPScanLineProviderForBaseBitmap(dest);
	else
		DestSLP->Attach(dest);
	data.Dest = DestSLP;
	data.DestLeft = dx;
	data.DestTop = dy;

	// process
	DivisibleTransHandler->Process(&data);

	if(data.Dest == data.Src1)
	{
		// returned destination differs from given destination
		// (returned destination is data.Src1)
		dest->CopyRect(dx, dy, MainImage, srcrect);
	}
	else if(data.Dest == data.Src2)
	{
		// (returned destination is data.Src2)
		dest->CopyRect(dx, dy, TransSrc->MainImage, srcrect);
	}
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
tTVPBaseTexture * tTJSNI_BaseLayer::tTransDrawable::
	GetDrawTargetBitmap(const tTVPRect &rect, tTVPRect &cliprect)
{
	// save target bitmap pointer
	Target = OrgDrawable->GetDrawTargetBitmap(rect, cliprect);
	TargetRect = cliprect;
	return Target;
}
//---------------------------------------------------------------------------
tTVPLayerType tTJSNI_BaseLayer::tTransDrawable::GetTargetLayerType()
{
	return OrgDrawable->GetTargetLayerType();
}
//---------------------------------------------------------------------------
void tTJSNI_BaseLayer::tTransDrawable::DrawCompleted(const tTVPRect &destrect,
	tTVPBaseTexture *bmp, const tTVPRect &cliprect, tTVPLayerType type,
		tjs_int opacity)
{
	// do divisible transition
	if(!Owner->InTransition || !Owner->DivisibleTransHandler) return;

	tTVPDivisibleData data;
	data.Left = destrect.left - Owner->Rect.left;
	data.Top = destrect.top - Owner->Rect.top;
	data.Width = cliprect.get_width();
	data.Height = cliprect.get_height();

    iTVPBaseBitmap * src1bmp;
	if(Owner->TransUpdateType == tutDivisible)
		src1bmp = Src1Bmp;
	else
		src1bmp = bmp;
	Owner->ImageModified = true;

	if(!Owner->SrcSLP)
		Owner->SrcSLP = new tTVPScanLineProviderForBaseBitmap(src1bmp);
	else
		Owner->SrcSLP->Attach(src1bmp);

	data.Src1 = Owner->SrcSLP;
	data.Src1Left = cliprect.left;
	data.Src1Top = cliprect.top;

	tTVPBaseTexture *src = NULL;
	if(Owner->TransSrc)
	{
		// source available
		// prepare source 2 from CacheBitmap
		if(Owner->TransUpdateType == tutDivisible)
			src = Src2Bmp;
		else
			src = Owner->TransSrc->Complete(destrect);
		if(!Owner->TransSrc->SrcSLP)
			Owner->TransSrc->SrcSLP =
				new tTVPScanLineProviderForBaseBitmap(src);
		else
			Owner->TransSrc->SrcSLP->Attach(src);

		data.Src2 = Owner->TransSrc->SrcSLP;
		data.Src2Left = data.Left; // destrect.left;
		data.Src2Top = data.Top; //destrect.top;
	}
	else
	{
		data.Src2 = NULL;
	}

	tTVPBaseTexture *dest;
	bool tempalloc = false;
	if(bmp == Target || Target == NULL)
	{
		// source bitmap is the same as the Original Target;
		// allocatte temporary bitmap
		dest = tTVPTempBitmapHolder::GetTemp(
							cliprect.get_width(),
							cliprect.get_height(), true);  // fit = true

					// TODO: check whether "fit" can affect the performance

		tempalloc = true;
		if(!Owner->DestSLP)
			Owner->DestSLP = new tTVPScanLineProviderForBaseBitmap(dest);
		else
			Owner->DestSLP->Attach(dest);
		data.Dest = Owner->DestSLP;
		data.DestLeft = 0;
		data.DestTop = 0;
	}
	else
	{
		if(!Owner->DestSLP)
			Owner->DestSLP = new tTVPScanLineProviderForBaseBitmap(Target);
		else
			Owner->DestSLP->Attach(Target);
		dest = Target;
		data.Dest = Owner->DestSLP;
		data.DestLeft = TargetRect.left;
		data.DestTop = TargetRect.top;
	}

	try
	{
		Owner->DivisibleTransHandler->Process(&data);
		tTVPRect cr = cliprect;

		if(data.Dest == Owner->DestSLP)
		{
			cr.set_offsets(data.DestLeft, data.DestTop);
			OrgDrawable->DrawCompleted(destrect, dest, cr, type, opacity);
		}
		else if(data.Dest == data.Src1)
		{
			cr.set_offsets(data.DestLeft, data.DestTop);
			OrgDrawable->DrawCompleted(destrect, bmp, cr, type, opacity);
		}
		else if(data.Dest == data.Src2 && Owner->TransSrc)
		{
			cr.set_offsets(data.DestLeft, data.DestTop);
			OrgDrawable->DrawCompleted(destrect, src, cr, type, opacity);
		}
	}
	catch(...)
	{
		if(tempalloc) tTVPTempBitmapHolder::FreeTemp();
		throw;
	}

	if(tempalloc) tTVPTempBitmapHolder::FreeTemp();
}
//---------------------------------------------------------------------------
tjs_uint64 tTJSNI_BaseLayer::GetTransTick()
{
	if(!UseTransTickCallback)
	{
		// just use TVPGetTickCount() as a source
		return TVPGetTickCount();
	}
	else
	{
		// call TransTickCallback to receive result
		tTJSVariant res;
		TransTickCallback.FuncCall(0, NULL, NULL, &res, 0, NULL, NULL);
		return (tjs_uint64)(tjs_int64)res;
	}
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
// tTJSNC_Layer : TJS Layer class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Layer::ClassID = -1;
tTJSNC_Layer::tTJSNC_Layer() : tTJSNativeClass(TJS_W("Layer"))
{
	// registration of native members

	TJS_BEGIN_NATIVE_MEMBERS(Layer) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_Layer,
	/*TJS class name*/Layer)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Layer)
//----------------------------------------------------------------------

//-- methods

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/moveBefore)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	_this->MoveBefore(src);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/moveBefore)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/moveBehind)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	_this->MoveBehind(src);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/moveBehind)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/bringToBack)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->BringToBack();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/bringToBack)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/bringToFront)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->BringToFront();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/bringToFront)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/saveLayerImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	ttstr name(*param[0]);
	ttstr type(TJS_W("bmp"));
	if(numparams >=2 && param[1]->Type() != tvtVoid)
		type = *param[1];
	_this->SaveLayerImage(name, type);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/saveLayerImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/loadImages)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	ttstr name(*param[0]);
	tjs_uint32 key = clNone; // TODO Intfなのに固有値が
	if(numparams >=2 && param[1]->Type() != tvtVoid)
		key = (tjs_uint32)param[1]->AsInteger();
	iTJSDispatch2 * metainfo = _this->LoadImages(name, key);
	try
	{
		if(result) *result = metainfo;
	}
	catch(...)
	{
		if(metainfo) metainfo->Release();
		throw;
	}
	if(metainfo) metainfo->Release();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/loadImages)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/loadProvinceImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	ttstr name(*param[0]);
	_this->LoadProvinceImage(name);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/loadProvinceImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getMainPixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	if(result) *result = (tjs_int64)_this->GetMainPixel(*param[0], *param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getMainPixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMainPixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	_this->SetMainPixel(*param[0], *param[1], (tjs_int)*param[2]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMainPixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getMaskPixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	if(result) *result = _this->GetMaskPixel(*param[0], *param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getMaskPixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMaskPixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	_this->SetMaskPixel(*param[0], *param[1], *param[2]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMaskPixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getProvincePixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	if(result) *result = _this->GetProvincePixel(*param[0], *param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getProvincePixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setProvincePixel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	_this->SetProvincePixel(*param[0], *param[1], *param[2]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setProvincePixel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getLayerAt)// not GetMostFrontChildAt
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	bool exclude_self = false;
	bool get_disabled = false;
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		exclude_self = param[2]->operator bool();

	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		get_disabled = param[3]->operator bool();

	tTJSNI_BaseLayer *lay =
		_this->GetMostFrontChildAt(*param[0], *param[1], exclude_self, get_disabled);

	if(result)
	{
		if(lay && lay->GetOwnerNoAddRef())
			*result = tTJSVariant(lay->GetOwnerNoAddRef(), lay->GetOwnerNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getLayerAt)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setPos) // not setPosition
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	if(numparams == 4 && param[2]->Type() != tvtVoid && param[3]->Type() != tvtVoid)
	{
		// set bounds
		tTVPRect r;
		r.left = *param[0];
		r.top = *param[1];
		r.right = (tjs_int)*param[2] + r.left;
		r.bottom = (tjs_int)*param[3] + r.top;
		_this->SetBounds(r);
	}
	else
	{
		// set position only
		_this->SetPosition(*param[0], *param[1]);
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetSize((tjs_int)*param[0], (tjs_int)*param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setSizeToImageSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	_this->SetSize(_this->GetImageWidth(), _this->GetImageHeight());
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setSizeToImageSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setImagePos)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetImagePosition((tjs_int)*param[0], (tjs_int)*param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setImagePos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setImageSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->SetImageSize((tjs_int)*param[0], (tjs_int)*param[1]);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setImageSize)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/independMainImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	bool copy = true;
	if(numparams >= 1 && param[0]->Type() != tvtVoid)
		copy = param[0]->operator bool();
	_this->IndependMainImage(copy);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/independMainImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/independProvinceImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	bool copy = true;
	if(numparams >= 1 && param[0]->Type() != tvtVoid)
		copy = param[0]->operator bool();
	_this->IndependProvinceImage(copy);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/independProvinceImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setClip) // not setClipRect
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams == 0)
	{
		// reset clip rectangle
		_this->ResetClip();

	}
	else
	{

		if(numparams < 4) return TJS_E_BADPARAMCOUNT;

		_this->SetClip(
			(tjs_int)*param[0],
			(tjs_int)*param[1],
			(tjs_int)*param[2],
			(tjs_int)*param[3]);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setClip)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/fillRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	tjs_int x, y;
	x = *param[0];
	y = *param[1];
	_this->FillRect(tTVPRect(x, y, x+(tjs_int)*param[2], y+(tjs_int)*param[3]),
		static_cast<tjs_uint32>((tjs_int64)*param[4]));
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/fillRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/colorRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 5) return TJS_E_BADPARAMCOUNT;
	tjs_int x, y;
	x = *param[0];
	y = *param[1];
	_this->ColorRect(tTVPRect(x, y, x+(tjs_int)*param[2], y+(tjs_int)*param[3]),
		static_cast<tjs_uint32>((tjs_int64)*param[4]),
		(numparams>=6 && param[5]->Type() != tvtVoid)? (tjs_int) *param[5] : 255);
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/colorRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/drawText)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	_this->DrawText(
		*param[0],
		*param[1],
		*param[2],
		static_cast<tjs_uint32>((tjs_int64)*param[3]),
		(numparams >= 5 && param[4]->Type() != tvtVoid)?(tjs_int)*param[4] : (tjs_int)255,
		(numparams >= 6 && param[5]->Type() != tvtVoid)? param[5]->operator bool() : true,
		(numparams >= 7 && param[6]->Type() != tvtVoid)? (tjs_int)*param[6] : 0,
		(numparams >= 8 && param[7]->Type() != tvtVoid)? static_cast<tjs_uint32>((tjs_int64)*param[7]) : 0,
		(numparams >= 9 && param[8]->Type() != tvtVoid)? (tjs_int)*param[8] : 0,
		(numparams >=10 && param[9]->Type() != tvtVoid)? (tjs_int)*param[9] : 0,
		(numparams >=11 && param[10]->Type() != tvtVoid)? (tjs_int)*param[10] : 0
		);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/drawText)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/drawGlyph)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 4) return TJS_E_BADPARAMCOUNT;
	iTJSDispatch2* glyph = param[2]->AsObjectNoAddRef();
	_this->DrawGlyph(
		*param[0],
		*param[1],
		glyph,
		static_cast<tjs_uint32>((tjs_int64)*param[3]),
		(numparams >= 5 && param[4]->Type() != tvtVoid)?(tjs_int)*param[4] : (tjs_int)255,
		(numparams >= 6 && param[5]->Type() != tvtVoid)? param[5]->operator bool() : true,
		(numparams >= 7 && param[6]->Type() != tvtVoid)? (tjs_int)*param[6] : 0,
		(numparams >= 8 && param[7]->Type() != tvtVoid)? static_cast<tjs_uint32>((tjs_int64)*param[7]) : 0,
		(numparams >= 9 && param[8]->Type() != tvtVoid)? (tjs_int)*param[8] : 0,
		(numparams >=10 && param[9]->Type() != tvtVoid)? (tjs_int)*param[9] : 0,
		(numparams >=11 && param[10]->Type() != tvtVoid)? (tjs_int)*param[10] : 0
		);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/drawGlyph)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/piledCopy)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 7) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect rect(*param[3], *param[4], *param[5], *param[6]);
	rect.right += rect.left;
	rect.bottom += rect.top;

	_this->PiledCopy(*param[0], *param[1], src, rect);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/piledCopy)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/copyRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 7) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	iTVPBaseBitmap* provinceSrc = NULL;
	tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = provinceSrc = NULL;
		else
		{
			src = srclayer->GetMainImage();
			provinceSrc = srclayer->GetProvinceImage();
		}

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = provinceSrc = NULL;
			else
				src = provinceSrc = srcbmp->GetBitmap();
		}
	}
	if(!src && !provinceSrc) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect rect(*param[3], *param[4], *param[5], *param[6]);
	rect.right += rect.left;
	rect.bottom += rect.top;

	_this->CopyRect(*param[0], *param[1], src, provinceSrc, rect);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/copyRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/pileRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 7) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect rect(*param[3], *param[4], *param[5], *param[6]);
	rect.right += rect.left;
	rect.bottom += rect.top;

	if(numparams >= 9 && param[8]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.pileRect"), TJS_W("9")));
	}

	_this->PileRect(*param[0], *param[1], src, rect,
		(numparams>=8 && param[7]->Type() != tvtVoid)?(tjs_int)*param[7]:255);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/pileRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/blendRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 7) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect rect(*param[3], *param[4], *param[5], *param[6]);
	rect.right += rect.left;
	rect.bottom += rect.top;

	if(numparams >= 9 && param[8]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.blendRect"), TJS_W("9")));
	}

	_this->BlendRect(*param[0], *param[1], src, rect,
		(numparams>=8 && param[7]->Type() != tvtVoid)?(tjs_int)*param[7]:255);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/blendRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/copy9Patch)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
		{
			src = srclayer->GetMainImage();
		}

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect margin;
	bool updated = _this->Copy9Patch( src, margin );
	if( result ) {
		if( updated ) {
			iTJSDispatch2 *ret = TVPCreateRectObject( margin.left, margin.top, margin.right, margin.bottom );
			*result = tTJSVariant(ret, ret);
			ret->Release();
		} else {
			result->Clear();
		}
	}
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/copy9Patch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 7) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
	tTVPBlendOperationMode automode = omAlpha;
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
			src = srclayer->GetMainImage(), automode = srclayer->GetOperationModeFromType();

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect rect(*param[3], *param[4], *param[5], *param[6]);
	rect.right += rect.left;
	rect.bottom += rect.top;

	tTVPBlendOperationMode mode;
	if(numparams >= 8 && param[7]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[7]);
	else
		mode = omAuto;

	if(numparams >= 10 && param[9]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.operateRect"), TJS_W("10")));
	}

	// get correct blend mode if the mode is omAuto
	if(mode == omAuto) mode = automode;

	_this->OperateRect(*param[0], *param[1], src, rect, mode,
		(numparams>=9 && param[8]->Type() != tvtVoid)?(tjs_int)*param[8]:255);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/operateRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stretchCopy)
{
	// dx, dy, dw, dh, src, sx, sy, sw, sh, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 9) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[4]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
			src = srclayer->GetMainImage();

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
	destrect.right += destrect.left;
	destrect.bottom += destrect.top;

	tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tTVPBBStretchType type = stNearest;
	if(numparams >= 10)
		type = (tTVPBBStretchType)(tjs_int)*param[9];

	tjs_real typeopt = 0.0;
	if(numparams >= 11)
		typeopt = (tjs_real)*param[10];
	else if( type == stFastCubic || type == stCubic )
		typeopt = -1.0;

	_this->StretchCopy(destrect, src, srcrect, type, typeopt);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stretchCopy)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stretchPile)
{
	// dx, dy, dw, dh, src, sx, sy, sw, sh, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 9) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[4]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
	destrect.right += destrect.left;
	destrect.bottom += destrect.top;

	tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tjs_int opa = 255;

	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		opa = *param[9];

	tTVPBBStretchType type = stNearest;
	if(numparams >= 11 && param[10]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[10];

	if(numparams >= 12 && param[11]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.stretchPile"), TJS_W("12")));
	}

	_this->StretchPile(destrect, src, srcrect, opa, type);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stretchPile)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stretchBlend)
{
	// dx, dy, dw, dh, src, sx, sy, sw, sh, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 9) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[4]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
	destrect.right += destrect.left;
	destrect.bottom += destrect.top;

	tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tjs_int opa = 255;

	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		opa = *param[9];

	tTVPBBStretchType type = stNearest;
	if(numparams >= 11 && param[10]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[10];
	if(numparams >= 12 && param[11]->Type() != tvtVoid)
	{
		static bool IsWarned = false;
		if (!IsWarned) {
			IsWarned = true;
			TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
				TJS_W("Layer.stretchBlend"), TJS_W("12")));
		}
	}

	_this->StretchBlend(destrect, src, srcrect, opa, type);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stretchBlend)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateStretch)
{
	// dx, dy, dw, dh, src, sx, sy, sw, sh, mode=omAuto, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 9) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[4]->AsObjectClosureNoAddRef();
	tTVPBlendOperationMode automode = omAlpha;
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
			src = srclayer->GetMainImage(), automode = srclayer->GetOperationModeFromType();

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect destrect(*param[0], *param[1], *param[2], *param[3]);
	destrect.right += destrect.left;
	destrect.bottom += destrect.top;

	tTVPRect srcrect(*param[5], *param[6], *param[7], *param[8]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tTVPBlendOperationMode mode;
	if(numparams >= 10 && param[9]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[9]);
	else
		mode = omAuto;

	tjs_int opa = 255;

	if(numparams >= 11 && param[10]->Type() != tvtVoid)
		opa = *param[10];

	tTVPBBStretchType type = stNearest;
	if(numparams >= 12 && param[11]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[11];

	tjs_real typeopt = 0.0;
	if(numparams >= 13)
		typeopt = (tjs_real)*param[12];
	else if( type == stFastCubic || type == stCubic )
		typeopt = -1.0;

	// get correct blend mode if the mode is omAuto
	if(mode == omAuto) mode = automode;

	_this->OperateStretch(destrect, src, srcrect, mode, opa, type, typeopt);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/operateStretch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/affineCopy)
{
	// src, sx, sy, sw, sh, affine, x0/a, y0/b, x1/c, y1/d, x2/tx, y2/ty, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 12) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
			src = srclayer->GetMainImage();

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tTVPBBStretchType type = stNearest;

	if(numparams >= 13 && param[12]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[12];

	bool clear = false;

	if(numparams >= 14 && param[13]->Type() != tvtVoid)
		clear = 0!=(tjs_int)*param[13];

	if(param[5]->operator bool())
	{
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		_this->AffineCopy(mat, src, srcrect, type, clear);
	}
	else
	{
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];
		_this->AffineCopy(points, src, srcrect, type, clear);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/affineCopy)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/affinePile)
{
	// src, sx, sy, sw, sh, affine, x0/a, y0/b, x1/c, y1/d, x2/tx, y2/ty, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 12) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tjs_int opa = 255;
	tTVPBBStretchType type = stNearest;

	if(numparams >= 13 && param[12]->Type() != tvtVoid)
		opa = (tjs_int)*param[12];
	if(numparams >= 14 && param[13]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[13];
	if(numparams >= 15 && param[14]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.affinePile"), TJS_W("15")));
	}

	if(param[5]->operator bool())
	{
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		_this->AffinePile(mat, src, srcrect, opa, type);
	}
	else
	{
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];
		_this->AffinePile(points, src, srcrect, opa, type);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/affinePile)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/affineBlend)
{
	// src, sx, sy, sw, sh, affine, x0/a, y0/b, x1/c, y1/d, x2/tx, y2/ty, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 12) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer * src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tjs_int opa = 255;
	tTVPBBStretchType type = stNearest;

	if(numparams >= 13 && param[12]->Type() != tvtVoid)
		opa = (tjs_int)*param[12];
	if(numparams >= 14 && param[13]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[13];
	if(numparams >= 15 && param[14]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.affineBlend"), TJS_W("15")));
	}

	if(param[5]->operator bool())
	{
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		_this->AffineBlend(mat, src, srcrect, opa, type);
	}
	else
	{
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];
		_this->AffineBlend(points, src, srcrect, opa, type);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/affineBlend)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/operateAffine)
{
	// src, sx, sy, sw, sh, affine, x0/a, y0/b, x1/c, y1/d, x2/tx, y2/ty,
	// mode=omAuto, opa=255, type=0
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	if(numparams < 12) return TJS_E_BADPARAMCOUNT;

	iTVPBaseBitmap* src = NULL;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTVPBlendOperationMode automode = omAlpha;
	if(clo.Object)
	{
		tTJSNI_BaseLayer * srclayer = NULL;
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&srclayer)))
			src = NULL;
		else
			src = srclayer->GetMainImage(), automode = srclayer->GetOperationModeFromType();

		if( src == NULL )
		{	// try to get bitmap interface
			tTJSNI_Bitmap * srcbmp = NULL;
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
				src = NULL;
			else
				src = srcbmp->GetBitmap();
		}
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayerOrBitmap);

	tTVPRect srcrect(*param[1], *param[2], *param[3], *param[4]);
	srcrect.right += srcrect.left;
	srcrect.bottom += srcrect.top;

	tjs_int opa = 255;
	tTVPBBStretchType type = stNearest;

	if(numparams >= 14 && param[13]->Type() != tvtVoid)
		opa = (tjs_int)*param[13];
	if(numparams >= 15 && param[14]->Type() != tvtVoid)
		type = (tTVPBBStretchType)(tjs_int)*param[14];
	if(numparams >= 16 && param[15]->Type() != tvtVoid)
	{
		TVPAddLog(TVPFormatMessage(TVPHoldDestinationAlphaParameterIsNowDeprecated,
			TJS_W("Layer.operateAffine"), TJS_W("16")));
	}

	tTVPBlendOperationMode mode;
	if(numparams >= 13 && param[12]->Type() != tvtVoid)
		mode = (tTVPBlendOperationMode)(tjs_int)(*param[12]);
	else
		mode = omAuto;

	// get correct blend mode if the mode is omAuto
	if(mode == omAuto) mode = automode;

	if(param[5]->operator bool())
	{
		// affine matrix mode
		t2DAffineMatrix mat;
		mat.a = *param[6];
		mat.b = *param[7];
		mat.c = *param[8];
		mat.d = *param[9];
		mat.tx = *param[10];
		mat.ty = *param[11];
		_this->OperateAffine(mat, src, srcrect, mode, opa, type);
	}
	else
	{
		// points mode
		tTVPPointD points[3];
		points[0].x = *param[6];
		points[0].y = *param[7];
		points[1].x = *param[8];
		points[1].y = *param[9];
		points[2].x = *param[10];
		points[2].y = *param[11];
		_this->OperateAffine(points, src, srcrect, mode, opa, type);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/operateAffine)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/doBoxBlur)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tjs_int xblur = 1;
	tjs_int yblur = 1;

	if(numparams >= 1 && param[0]->Type() != tvtVoid)
		xblur = (tjs_int)*param[0];
	
	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		yblur = (tjs_int)*param[1];
	
	_this->DoBoxBlur(xblur, yblur);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/doBoxBlur)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/adjustGamma)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams == 0) return TJS_S_OK;

	tTVPGLGammaAdjustData data;
	memcpy(&data, &TVPIntactGammaAdjustData, sizeof(data));

	if(numparams >= 1 && param[0]->Type() != tvtVoid)
		data.RGamma = param[0]->AsReal();
	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		data.RFloor = *param[1];
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
		data.RCeil  = *param[2];
	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		data.GGamma = param[3]->AsReal();
	if(numparams >= 5 && param[4]->Type() != tvtVoid)
		data.GFloor = *param[4];
	if(numparams >= 6 && param[5]->Type() != tvtVoid)
		data.GCeil  = *param[5];
	if(numparams >= 7 && param[6]->Type() != tvtVoid)
		data.BGamma = param[6]->AsReal();
	if(numparams >= 8 && param[7]->Type() != tvtVoid)
		data.BFloor = *param[7];
	if(numparams >= 9 && param[8]->Type() != tvtVoid)
		data.BCeil  = *param[8];

	_this->AdjustGamma(data);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/adjustGamma)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/doGrayScale)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->DoGrayScale();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/doGrayScale)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/flipLR) // not LRFlip
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->LRFlip();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/flipLR)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/flipUD) // not UDFlip
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->UDFlip();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/flipUD)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/convertType)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTVPDrawFace fromtype = (tTVPDrawFace)(tjs_int)*param[0];

	_this->ConvertLayerType(fromtype);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/convertType)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/update)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	// this event sets CallOnPaint flag of tTJSNI_Layer, to
	// invoke onPaint event.

	if(numparams < 1)
	{
		_this->UpdateByScript();
			// update entire area of the layer
	}
	else
	{
		if(numparams < 4) return TJS_E_BADPARAMCOUNT;
		tjs_int l, t, w, h;
		l = (tjs_int)*param[0];
		t = (tjs_int)*param[1];
		w = (tjs_int)*param[2];
		h = (tjs_int)*param[3];
		_this->UpdateByScript(tTVPRect(l, t, l+w, t+h));
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/update)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setCursorPos)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	_this->SetCursorPos(*param[0], *param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setCursorPos)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/releaseCapture)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->ReleaseCapture();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/releaseCapture)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/releaseTouchCapture)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	
	if(numparams >= 1)
	{
		tjs_uint32 id = static_cast<tjs_uint32>(param[0]->AsInteger());
		_this->ReleaseTouchCapture(id);
	}
	else
	{
		_this->ReleaseTouchCapture(0,true);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/releaseTouchCapture)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/focus) // not setFocus
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	bool direction = true;
	if(numparams >= 1) direction = param[0]->operator bool();

	bool succeeded = _this->SetFocus(direction);

	if(result) *result = (tjs_int)succeeded;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/focus)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/focusPrev)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSNI_BaseLayer *lay = _this->FocusPrev();

	if(result)
	{
		if(lay && lay->GetOwnerNoAddRef())
			*result = tTJSVariant(lay->GetOwnerNoAddRef(), lay->GetOwnerNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/focusPrev)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/focusNext)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSNI_BaseLayer *lay = _this->FocusNext();

	if(result)
	{
		if(lay && lay->GetOwnerNoAddRef())
			*result = tTJSVariant(lay->GetOwnerNoAddRef(), lay->GetOwnerNoAddRef());
		else
			*result = tTJSVariant((iTJSDispatch2*)NULL);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/focusNext)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setMode)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->SetMode();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setMode)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/removeMode)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->RemoveMode();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/removeMode)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/setAttentionPos) // not setAttentionPoint
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 2) return TJS_E_BADPARAMCOUNT;

	_this->SetAttentionPoint(*param[0], *param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/setAttentionPos)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/beginTransition)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	// parameters are : <name>, [<withchildren>=true], [<transwith>=null],
	//                  [<options>]
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;
	ttstr name = *param[0];
	bool withchildren = true;
	if(numparams >= 2 && param[1]->Type() != tvtVoid)
		withchildren = param[1]->operator bool();
	tTJSNI_BaseLayer * transsrc = NULL;
	if(numparams >= 3 && param[2]->Type() != tvtVoid)
	{
		tTJSVariantClosure clo = param[2]->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&transsrc)))
				TVPThrowExceptionMessage(TVPSpecifyLayer);
		}
	}
	if(!transsrc) TVPThrowExceptionMessage(TVPSpecifyLayer);

	tTJSVariantClosure options(NULL, NULL);
	if(numparams >= 4 && param[3]->Type() != tvtVoid)
		options = param[3]->AsObjectClosureNoAddRef();

	_this->StartTransition(name, withchildren, transsrc, options);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/beginTransition)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/stopTransition)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	_this->StopTransition();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/stopTransition)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/assignImages)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSNI_BaseLayer *src;
	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	if(clo.Object)
	{
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);
	}
	if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);

	_this->AssignImages(src);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/assignImages)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/dump)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
	_this->DumpStructure();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/dump)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/copyToBitmapFromMainImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Bitmap* dstbmp = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&dstbmp)))
			return TJS_E_INVALIDPARAM;
		if( dstbmp ) _this->CopyFromMainImage( dstbmp );
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/copyToBitmapFromMainImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/copyFromBitmapToMainImage)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
	tTJSNI_Bitmap* srcbmp = NULL;
	if( clo.Object ) {
		if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Bitmap::ClassID, (iTJSNativeInstance**)&srcbmp)))
			return TJS_E_INVALIDPARAM;
		if( srcbmp ) _this->AssignMainImageWithUpdate( srcbmp->GetBitmap() );
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/copyFromBitmapToMainImage)
//----------------------------------------------------------------------

//-- events

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onHitTest)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	/*
		this event does not call "action" method
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(3, "onHitTest", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("hit");
		TVP_ACTION_INVOKE_END(obj);
	}
	*/
	if(numparams < 3) return TJS_E_BADPARAMCOUNT;
	bool b = param[2]->operator bool();

	_this->SetHitTestWork(b);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onHitTest)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(2, "onClick", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onClick)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onDoubleClick)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(2, "onDoubleClick", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onDoubleClick)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(4, "onMouseDown", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("button");
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(4, "onMouseUp", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("button");
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseMove)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(3, "onMouseMove", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseMove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseEnter)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onMouseEnter", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseEnter)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseLeave)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onMouseLeave", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseLeave)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTouchDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(5, "onTouchDown", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("cx");
		TVP_ACTION_INVOKE_MEMBER("cy");
		TVP_ACTION_INVOKE_MEMBER("id");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTouchDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTouchUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(5, "onTouchUp", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("cx");
		TVP_ACTION_INVOKE_MEMBER("cy");
		TVP_ACTION_INVOKE_MEMBER("id");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTouchUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTouchMove)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(5, "onTouchMove", objthis);
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_MEMBER("cx");
		TVP_ACTION_INVOKE_MEMBER("cy");
		TVP_ACTION_INVOKE_MEMBER("id");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTouchMove)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTouchScaling)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(5, "onTouchScaling", objthis);
		TVP_ACTION_INVOKE_MEMBER("startdistance");
		TVP_ACTION_INVOKE_MEMBER("currentdistance");
		TVP_ACTION_INVOKE_MEMBER("cx");
		TVP_ACTION_INVOKE_MEMBER("cy");
		TVP_ACTION_INVOKE_MEMBER("flag");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTouchScaling)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTouchRotate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(6, "onTouchRotate", objthis);
		TVP_ACTION_INVOKE_MEMBER("startangle");
		TVP_ACTION_INVOKE_MEMBER("currentangle");
		TVP_ACTION_INVOKE_MEMBER("distance");
		TVP_ACTION_INVOKE_MEMBER("cx");
		TVP_ACTION_INVOKE_MEMBER("cy");
		TVP_ACTION_INVOKE_MEMBER("flag");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTouchRotate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMultiTouch)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onMultiTouch", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMultiTouch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onBlur)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onBlur", objthis);
		TVP_ACTION_INVOKE_MEMBER("focused");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onBlur)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onFocus)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onFocus", objthis);
		TVP_ACTION_INVOKE_MEMBER("blurred");
		TVP_ACTION_INVOKE_MEMBER("direction");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onFocus)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onNodeEnabled)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onNodeEnabled", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onNodeEnabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onNodeDisabled)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onNodeDisabled", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onNodeDisabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onKeyDown)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(3, "onKeyDown", objthis);
		TVP_ACTION_INVOKE_MEMBER("key");
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_MEMBER("process");
		TVP_ACTION_INVOKE_END(obj);
	}

	// call default key down behavior handler
	if(numparams < 3 || param[2]->operator bool())
		_this->DefaultKeyDown((tjs_int)*param[0], (tjs_int)*param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onKeyDown)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onKeyUp)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(3, "onKeyUp", objthis);
		TVP_ACTION_INVOKE_MEMBER("key");
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_MEMBER("process");
		TVP_ACTION_INVOKE_END(obj);
	}

	// call default key up behavior handler
	if(numparams < 3 || param[2]->operator bool())
		_this->DefaultKeyUp((tjs_int)*param[0], (tjs_int)*param[1]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onKeyUp)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onKeyPress)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(2, "onKeyPress", objthis);
		TVP_ACTION_INVOKE_MEMBER("key");
		TVP_ACTION_INVOKE_MEMBER("process");
		TVP_ACTION_INVOKE_END(obj);
	}

	// call default key down behavior handler
	if(numparams < 2 || param[1]->operator bool())
	{
		ttstr p = *param[0];
		tjs_char code;
		if(p.IsEmpty()) code = 0; else code = (tjs_char)*p.c_str();
		_this->DefaultKeyPress(code);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onKeyPress)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onMouseWheel)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(4, "onMouseWheel", objthis);
		TVP_ACTION_INVOKE_MEMBER("shift");
		TVP_ACTION_INVOKE_MEMBER("delta");
		TVP_ACTION_INVOKE_MEMBER("x");
		TVP_ACTION_INVOKE_MEMBER("y");
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onMouseWheel)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSearchPrevFocusable)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onSearchPrevFocusable", objthis);
		TVP_ACTION_INVOKE_MEMBER("layer");
		TVP_ACTION_INVOKE_END(obj);
	}

	// set focusable layer back
	if(param[0]->Type() != tvtVoid)
	{
		tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			tTJSNI_BaseLayer *src;
			if(clo.Object)
			{
				if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
					TVPThrowExceptionMessage(TVPSpecifyLayer);
			}
			if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
			_this->SetFocusWork(src);
		}
		else
		{
			_this->SetFocusWork(NULL);
		}
	}
	else
	{
		_this->SetFocusWork(NULL);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSearchPrevFocusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onSearchNextFocusable)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(1, "onSearchNextFocusable", objthis);
		TVP_ACTION_INVOKE_MEMBER("layer");
		TVP_ACTION_INVOKE_END(obj);
	}

	// set focusable layer back
	if(param[0]->Type() != tvtVoid)
	{
		tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			tTJSNI_BaseLayer *src;
			if(clo.Object)
			{
				if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
					TVPThrowExceptionMessage(TVPSpecifyLayer);
			}
			if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
			_this->SetFocusWork(src);
		}
		else
		{
			_this->SetFocusWork(NULL);
		}
	}
	else
	{
		_this->SetFocusWork(NULL);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onSearchNextFocusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onBeforeFocus)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(3, "onBeforeFocus", objthis);
		TVP_ACTION_INVOKE_MEMBER("layer");
		TVP_ACTION_INVOKE_MEMBER("blurred");
		TVP_ACTION_INVOKE_MEMBER("direction");
		TVP_ACTION_INVOKE_END(obj);
	}

	// set focusable layer back
	if(param[0]->Type() != tvtVoid)
	{
		tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			tTJSNI_BaseLayer *src;
			if(clo.Object)
			{
				if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
					tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&src)))
					TVPThrowExceptionMessage(TVPSpecifyLayer);
			}
			if(!src) TVPThrowExceptionMessage(TVPSpecifyLayer);
			_this->SetFocusWork(src);
		}
		else
		{
			_this->SetFocusWork(NULL);
		}
	}
	else
	{
		_this->SetFocusWork(NULL);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onBeforeFocus)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onPaint)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(0, "onPaint", objthis);
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onPaint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/onTransitionCompleted)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

	tTJSVariantClosure obj = _this->GetActionOwnerNoAddRef();
	if(obj.Object)
	{
		TVP_ACTION_INVOKE_BEGIN(2, "onTransitionCompleted", objthis);
		TVP_ACTION_INVOKE_MEMBER("dest"); // destination (before exchanging)
		TVP_ACTION_INVOKE_MEMBER("src"); // source (also before exchanging;can be a null)
		TVP_ACTION_INVOKE_END(obj);
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/onTransitionCompleted)
//----------------------------------------------------------------------


//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(parent)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		tTJSNI_BaseLayer *parent = _this->GetParent();
		if(parent)
		{
			iTJSDispatch2 *dsp = parent->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2 *) NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);

		tTJSNI_BaseLayer *parent;
		tTJSVariantClosure clo = param->AsObjectClosureNoAddRef();
		if(clo.Object)
		{
			if(TJS_FAILED(clo.Object->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
				tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&parent)))
				TVPThrowExceptionMessage(TVPSpecifyLayer);
		}

		_this->SetParent(parent);

		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(parent)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(children)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		iTJSDispatch2 *dsp = _this->GetChildrenArrayObjectNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(children)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(order) // not orderIndex
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetOrderIndex();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetOrderIndex(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(order)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(absolute) // not absoluteOrderIndex
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetAbsoluteOrderIndex();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetAbsoluteOrderIndex(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(absolute)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(absoluteOrderMode) // not absoluteOrderIndexMode
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetAbsoluteOrderMode();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetAbsoluteOrderMode(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(absoluteOrderMode)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(visible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetVisible();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetVisible(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(visible)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(cached)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetCached();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetCached(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(cached)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(nodeVisible)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetNodeVisible();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(nodeVisible)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(opacity)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetOpacity();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetOpacity(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(opacity)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(window)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		iTVPLayerTreeOwner* lto = _this->GetLayerTreeOwner();
		if(!lto)
		{
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		}
		else
		{
			iTJSDispatch2 *dsp = lto->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(window)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(isPrimary)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->IsPrimary();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(isPrimary)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(left)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetLeft();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetLeft(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(left)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(top)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetTop();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetTop(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(top)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(width)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetWidth(static_cast<tjs_uint>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(width)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHeight(static_cast<tjs_uint>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imageLeft)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetImageLeft();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImageLeft(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imageLeft)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imageTop)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetImageTop();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImageTop(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imageTop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imageWidth)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetImageWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImageWidth(static_cast<tjs_uint>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imageWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imageHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetImageHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImageHeight(static_cast<tjs_uint>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imageHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(type)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetType();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetType((tTVPLayerType)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(type)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(face)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetFace();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetFace((tTVPDrawFace)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(face)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(holdAlpha)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetHoldAlpha();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHoldAlpha(0!=(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(holdAlpha)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(clipLeft)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetClipLeft();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetClipLeft(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(clipLeft)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(clipTop)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetClipTop();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetClipTop(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(clipTop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(clipWidth)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetClipWidth();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetClipWidth(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(clipWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(clipHeight)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetClipHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetClipHeight(static_cast<tjs_int>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(clipHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imageModified)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetImageModified();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImageModified(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imageModified)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hitType)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetHitType();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHitType((tTVPHitType)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hitType)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hitThreshold)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetHitThreshold();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHitThreshold((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hitThreshold)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(cursor)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetCursor();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		if(param->Type() == tvtString)
			_this->SetCursorByStorage(*param);
		else
			_this->SetCursorByNumber(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(cursor)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(cursorX)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetCursorX();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetCursorX(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(cursorX)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(cursorY)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetCursorY();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetCursorY(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(cursorY)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hint)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetHint();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHint(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(showParentHint)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetShowParentHint();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetShowParentHint(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(showParentHint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(ignoreHintSensing)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetIgnoreHintSensing();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetIgnoreHintSensing(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(ignoreHintSensing)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(focusable)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetFocusable();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetFocusable(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(focusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(prevFocusable)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		tTJSNI_BaseLayer *lay = _this->GetPrevFocusable();
		if(lay)
		{
			iTJSDispatch2 *dsp = lay->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(prevFocusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(nextFocusable)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		tTJSNI_BaseLayer *lay = _this->GetNextFocusable();
		if(lay)
		{
			iTJSDispatch2 *dsp = lay->GetOwnerNoAddRef();
			*result = tTJSVariant(dsp, dsp);
		}
		else
		{
			*result = tTJSVariant((iTJSDispatch2*)NULL);
		}
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(nextFocusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(joinFocusChain)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetJoinFocusChain();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetJoinFocusChain(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(joinFocusChain)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(nodeFocusable)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetNodeFocusable();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(nodeFocusable)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(focused)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetFocused();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(focused)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enabled)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetEnabled();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetEnabled(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(enabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(nodeEnabled)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetNodeEnabled();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(nodeEnabled)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(attentionLeft)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetAttentionLeft();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetAttentionLeft((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(attentionLeft)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(attentionTop)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetAttentionTop();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetAttentionTop((tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(attentionTop)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(useAttention)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetUseAttention();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetUseAttention(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(useAttention)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(imeMode)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)_this->GetImeMode();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetImeMode((tTVPImeMode) (tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(imeMode)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(callOnPaint)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetCallOnPaint();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetCallOnPaint(param->operator bool());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(callOnPaint)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(font)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		iTJSDispatch2 *dsp = _this->GetFontObjectNoAddRef();
		*result = tTJSVariant(dsp, dsp);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(font)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(name)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = _this->GetName();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetName(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(name)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(neutralColor)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int64)_this->GetNeutralColor();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetNeutralColor(static_cast<tjs_uint32>((tjs_int64)*param));
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(neutralColor)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hasImage)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		*result = (tjs_int)(bool)_this->GetHasImage();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);
		_this->SetHasImage((bool)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(hasImage)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mainImageBuffer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = (tTVInteger)reinterpret_cast<tjs_intptr_t>(_this->GetMainImagePixelBuffer());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mainImageBuffer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mainImageBufferForWrite)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = (tTVInteger)reinterpret_cast<tjs_intptr_t>(_this->GetMainImagePixelBufferForWrite());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mainImageBufferForWrite)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(mainImageBufferPitch)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = _this->GetMainImagePixelBufferPitch();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(mainImageBufferPitch)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(provinceImageBuffer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = (tTVInteger)reinterpret_cast<tjs_intptr_t>(_this->GetProvinceImagePixelBuffer());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(provinceImageBuffer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(provinceImageBufferForWrite)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = (tTVInteger)reinterpret_cast<tjs_intptr_t>(_this->GetProvinceImagePixelBufferForWrite());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(provinceImageBufferForWrite)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(provinceImageBufferPitch)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Layer);;
		*result = _this->GetProvinceImagePixelBufferPitch();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(provinceImageBufferPitch)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS


}
//---------------------------------------------------------------------------



extern FontSystem* TVPFontSystem;
//---------------------------------------------------------------------------
// tTJSNI_Font : Font Native Object
//---------------------------------------------------------------------------
tTJSNI_Font::tTJSNI_Font()
{
	Layer = NULL;
}
//---------------------------------------------------------------------------
tTJSNI_Font::~tTJSNI_Font()
{
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD tTJSNI_Font::Construct(tjs_int numparams,
	tTJSVariant **param, iTJSDispatch2 *tjs_obj)
{
	if( numparams >= 1 )
	{
		iTJSDispatch2 *dsp = param[0]->AsObjectNoAddRef();

		tTJSNI_Layer *lay = NULL;
		if(TJS_FAILED(dsp->NativeInstanceSupport(TJS_NIS_GETINSTANCE,
			tTJSNC_Layer::ClassID, (iTJSNativeInstance**)&lay)))
			TVPThrowExceptionMessage(TVPSpecifyLayer);

		Layer = lay;
	}
	else
	{
		Layer = NULL;
		Font = TVPFontSystem->GetDefaultFont();
	}

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_Font::Invalidate()
{
	Layer = NULL;

	inherited::Invalidate();
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontFace(const ttstr & face)
{
	if( Layer ) Layer->SetFontFace(face);
	else
	{
		if(Font.Face != face)
		{
			Font.Face = face;
		}
	}
}
//---------------------------------------------------------------------------
ttstr tTJSNI_Font::GetFontFace() const
{
	if( Layer ) return Layer->GetFontFace();
	else return Font.Face;
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontHeight(tjs_int height)
{
	if( Layer ) Layer->SetFontHeight(height);
	else
	{
		if(height < 0) height = -height; // TVP2 does not support negative value of height
		if(Font.Height != height)
		{
			Font.Height = height;
		}
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Font::GetFontHeight() const
{
	if( Layer ) return Layer->GetFontHeight();
	else return Font.Height;
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontAngle(tjs_int angle)
{
	if( Layer ) Layer->SetFontAngle( angle );
	else
	{
		if(Font.Angle != angle)
		{
			angle = angle % 3600;
			if(angle < 0) angle += 3600;
			Font.Angle = angle;
		}
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Font::GetFontAngle() const
{
	if( Layer ) return Layer->GetFontAngle();
	else return Font.Angle;
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontBold(bool b)
{
	if( Layer ) Layer->SetFontBold(b);
	else
	{
		if( (0!=(Font.Flags & TVP_TF_BOLD)) != b)
		{
			Font.Flags &= ~TVP_TF_BOLD;
			if(b) Font.Flags |= TVP_TF_BOLD;
		}
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_Font::GetFontBold() const
{
	if( Layer ) return Layer->GetFontBold();
	else return 0!=(Font.Flags & TVP_TF_BOLD);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontItalic(bool b)
{
	if( Layer ) Layer->SetFontItalic(b);
	else
	{
		if( (0!=(Font.Flags & TVP_TF_ITALIC)) != b)
		{
			Font.Flags &= ~TVP_TF_ITALIC;
			if(b) Font.Flags |= TVP_TF_ITALIC;
		} 
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_Font::GetFontItalic() const
{
	if( Layer ) return Layer->GetFontItalic();
	else return 0!=(Font.Flags & TVP_TF_ITALIC);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontStrikeout(bool b)
{
	if( Layer ) Layer->SetFontStrikeout(b);
	else
	{
		if( (0!=(Font.Flags & TVP_TF_STRIKEOUT)) != b)
		{
			Font.Flags &= ~TVP_TF_STRIKEOUT;
			if(b) Font.Flags |= TVP_TF_STRIKEOUT;
		}
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_Font::GetFontStrikeout() const
{
	if( Layer ) return Layer->GetFontStrikeout();
	else return 0!=(Font.Flags & TVP_TF_STRIKEOUT);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontUnderline(bool b)
{
	if( Layer ) Layer->SetFontUnderline(b);
	else
	{
		if( (0!=(Font.Flags & TVP_TF_UNDERLINE)) != b)
		{
			Font.Flags &= ~TVP_TF_UNDERLINE;
			if(b) Font.Flags |= TVP_TF_UNDERLINE;
		}
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_Font::GetFontUnderline() const
{
	if( Layer ) return Layer->GetFontUnderline();
	else return 0!=(Font.Flags & TVP_TF_UNDERLINE);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::SetFontFaceIsFileName(bool b)
{
	if( Layer ) Layer->SetFontFaceIsFileName(b);
	else
	{
		if( (0!=(Font.Flags & TVP_TF_FONTFILE)) != b)
		{
			Font.Flags &= ~TVP_TF_FONTFILE;
			if(b) Font.Flags |= TVP_TF_FONTFILE;
		}
	}
}
//---------------------------------------------------------------------------
bool tTJSNI_Font::GetFontFaceIsFileName() const
{
	if( Layer ) return Layer->GetFontFaceIsFileName();
	else return 0!=(Font.Flags & TVP_TF_FONTFILE);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Font::GetTextWidthDirect(const ttstr & text)
{
	GetCurrentRasterizer()->ApplyFont( Font );
	tjs_uint width = 0;
	const tjs_char *buf = text.c_str();
	while(*buf)
	{
		tjs_int w, h;
		GetCurrentRasterizer()->GetTextExtent( *buf, w, h );
		width += w;
		buf++;
	}
	return width;
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Font::GetTextWidth(const ttstr & text)
{
	if( Layer ) return Layer->GetTextWidth(text);
	else return GetTextWidthDirect(text);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Font::GetTextHeight(const ttstr & text)
{
	if( Layer ) return Layer->GetTextHeight(text);
	else return std::abs(Font.Height);
}
//---------------------------------------------------------------------------
double tTJSNI_Font::GetEscWidthX(const ttstr & text)
{
	if( Layer ) return Layer->GetEscWidthX(text);
	else return std::cos(Font.Angle * (M_PI/1800)) * GetTextWidthDirect(text);
}
//---------------------------------------------------------------------------
double tTJSNI_Font::GetEscWidthY(const ttstr & text)
{
	if( Layer ) return Layer->GetEscWidthY(text);
	else return std::sin(Font.Angle * (M_PI/1800)) * (-GetTextWidthDirect(text));
}
//---------------------------------------------------------------------------
double tTJSNI_Font::GetEscHeightX(const ttstr & text)
{
	if( Layer ) return Layer->GetEscHeightX(text);
	else return std::sin(Font.Angle * (M_PI/1800)) * std::abs(Font.Height);
}
//---------------------------------------------------------------------------
double tTJSNI_Font::GetEscHeightY(const ttstr & text)
{
	if( Layer ) return Layer->GetEscHeightY(text);
	else return std::cos(Font.Angle * (M_PI/1800)) * std::abs(Font.Height);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::GetFontGlyphDrawRect( const ttstr & text, tTVPRect& area )
{
	if( Layer )
	{
		Layer->GetFontGlyphDrawRect(text,area);
	}
	else
	{
		GetCurrentRasterizer()->ApplyFont( Font );
		GetCurrentRasterizer()->GetGlyphDrawRect( text, area );
	}
}
//---------------------------------------------------------------------------
extern void TVPGetAllFontList(std::vector<ttstr>& list);
void tTJSNI_Font::GetFontList(tjs_uint32 flags, std::vector<ttstr> & list)
{
	if( Layer ) Layer->GetFontList(flags,list);
	else
	{
		std::vector<ttstr> ansilist;
		TVPGetAllFontList(ansilist);
		for(std::vector<ttstr>::iterator i = ansilist.begin(); i != ansilist.end(); i++)
			list.push_back(i->c_str());
	}
}
//---------------------------------------------------------------------------
void tTJSNI_Font::MapPrerenderedFont(const ttstr & storage)
{
	if( Layer ) Layer->MapPrerenderedFont(storage);
	else TVPMapPrerenderedFont(Font, storage);
}
//---------------------------------------------------------------------------
void tTJSNI_Font::UnmapPrerenderedFont()
{
	if( Layer ) Layer->UnmapPrerenderedFont();
	else TVPUnmapPrerenderedFont(Font);
}
//---------------------------------------------------------------------------
const tTVPFont& tTJSNI_Font::GetFont() const
{
	if( Layer ) return Layer->GetFont();
	else return Font;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// tTJSNC_Font : TJS Font class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_Font::ClassID = -1;
tTJSNC_Font::tTJSNC_Font() : tTJSNativeClass(TJS_W("Font"))
{
	TJS_BEGIN_NATIVE_MEMBERS(Font) // constructor
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_Font,
	/*TJS class name*/Font)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/Font)
//----------------------------------------------------------------------

//-- methods

//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTextWidth)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetTextWidth(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getTextWidth)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getTextHeight)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetTextHeight(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getTextHeight)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getEscWidthX)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetEscWidthX(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getEscWidthX)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getEscWidthY)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetEscWidthY(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getEscWidthY)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getEscHeightX)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetEscHeightX(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getEscHeightX)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getEscHeightY)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) *result = _this->GetEscHeightY(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getEscHeightY)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getGlyphDrawRect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	if(result) {
		tTVPRect rt;
		_this->GetFontGlyphDrawRect( *param[0], rt );
		iTJSDispatch2 *disp = TVPCreateRectObject( rt.left, rt.top, rt.right, rt.bottom );
		*result = tTJSVariant(disp, disp);
		disp->Release();
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getGlyphDrawRect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/doUserSelect)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);

	if(numparams < 4) return TJS_E_BADPARAMCOUNT;

	tjs_uint32 flags = (tjs_int64)*param[0];
	ttstr caption = *param[1];
	ttstr prompt = *param[2];
	ttstr samplestring = *param[3];

	tjs_int ret = // TODO: implement it ?
#if 0
		(tjs_int)_this->GetLayer()->DoUserFontSelect(flags, caption,
		prompt, samplestring);
#else
		0;
#endif

	if(result) *result = ret;

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/doUserSelect)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/getList)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);

	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	tjs_uint32 flags = static_cast<tjs_uint32>((tjs_int64)*param[0]);

	std::vector<ttstr> list;
	_this->GetFontList(flags, list);

	if(result)
	{
		iTJSDispatch2 *dsp;
		dsp = TJSCreateArrayObject();
		tTJSVariant tmp(dsp, dsp);
		*result = tmp;
		dsp->Release();

		for(tjs_uint i = 0; i < list.size(); i++)
		{
			tmp = list[i];
			dsp->PropSetByNum(TJS_MEMBERENSURE, i, &tmp, dsp);
		}
	}

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/getList)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/mapPrerenderedFont)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 1) return TJS_E_BADPARAMCOUNT;

	_this->MapPrerenderedFont(*param[0]);

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/mapPrerenderedFont)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/unmapPrerenderedFont)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
	if(numparams < 0) return TJS_E_BADPARAMCOUNT;

	_this->UnmapPrerenderedFont();

	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/unmapPrerenderedFont)
//----------------------------------------------------------------------

//-- properties

//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(face)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontFace();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontFace(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(face)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(height)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontHeight();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontHeight(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(height)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(bold)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontBold();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontBold(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(bold)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(italic)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontItalic();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontItalic(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(italic)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(strikeout)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontStrikeout();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontStrikeout(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(strikeout)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(underline)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontUnderline();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontUnderline(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(underline)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(angle)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontAngle();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontAngle(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(angle)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(faceIsFileName)
{
	// Face名をファイル名として開く、FreeTypeでのみ有効。ただし、そのレイヤーでIMEを有効した場合動作は不定
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		*result = _this->GetFontFaceIsFileName();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_Font);
		_this->SetFontFaceIsFileName(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(faceIsFileName)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(rasterizer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetFontRasterizer();
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TVPSetFontRasterizer(*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(rasterizer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(defaultFaceName)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		*result = TVPGetDefaultFontName();
		// *result = ttstr(TVPFontSystem->GetDefaultFontName());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		ttstr name( *param );
		// don't override, specified by preference
		// TVPFontSystem->SetDefaultFontName( name.c_str() );
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_STATIC_PROP_DECL(defaultFaceName)
//----------------------------------------------------------------------

	TJS_END_NATIVE_MEMBERS
}

//---------------------------------------------------------------------------
// TVPCreateNativeClass_Font
//---------------------------------------------------------------------------
struct tFontClassHolder {
	tTJSNativeClass * Obj;
	tFontClassHolder() : Obj(NULL) {}
	void Set( tTJSNativeClass* obj ) {
		if( Obj ) {
			Obj->Release();
			Obj = NULL;
		}
		Obj = obj;
		Obj->AddRef();
	}
	~tFontClassHolder() { if( Obj ) Obj->Release(), Obj = NULL; }
} static fontclassholder;
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_Font()
{
	if( fontclassholder.Obj ) {
		tTJSNativeClass* fontclass = fontclassholder.Obj;
		fontclass->AddRef();
		return fontclass;
	}
	tTJSNativeClass* fontclass = new tTJSNC_Font();
	fontclassholder.Set( fontclass );
	return fontclass;
}
//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateFontObject(iTJSDispatch2 * layer)
{
	if( fontclassholder.Obj == NULL ) {
		TVPThrowInternalError;
	}
	iTJSDispatch2 *out;
	tTJSVariant param(layer);
	tTJSVariant *pparam = &param;
	if(TJS_FAILED(fontclassholder.Obj->CreateNew(0, NULL, NULL, &out, 1, &pparam, fontclassholder.Obj)))
		TVPThrowInternalError;

	return out;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
iTJSDispatch2 * TVPGetObjectFrom_NI_BaseLayer(tTJSNI_BaseLayer * layer)
{
	iTJSDispatch2 * disp = layer->Owner;
	if(disp) disp->AddRef();
	return disp;
}
//---------------------------------------------------------------------------

