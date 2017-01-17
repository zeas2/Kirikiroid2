//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Layer Management
//---------------------------------------------------------------------------
#ifndef LayerIntfH
#define LayerIntfH


#include "tjsNative.h"
#include "tvpfontstruc.h"
#include "ComplexRect.h"
#include "drawable.h"
#include "tvpinputdefs.h"
#include "TransIntf.h"
#include "EventIntf.h"
#include "ObjectList.h"




//---------------------------------------------------------------------------
// global flags
//---------------------------------------------------------------------------
extern bool TVPFreeUnusedLayerCache;

//---------------------------------------------------------------------------
// initial bitmap holder ( since tTVPBaseBitmap cannot create empty bitmap )
//---------------------------------------------------------------------------
const tTVPBaseTexture & TVPGetInitialBitmap();
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// bitmap holder refcont control ( for Bitmap class )
//---------------------------------------------------------------------------
void TVPTempBitmapHolderAddRef();
void TVPTempBitmapHolderRelease();
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// global options
//---------------------------------------------------------------------------
enum tTVPGraphicSplitOperationType
{ gsotNone, gsotSimple, gsotInterlace, gsotBiDirection };
extern tTVPGraphicSplitOperationType TVPGraphicSplitOperationType;
extern bool TVPDefaultHoldAlpha;
//---------------------------------------------------------------------------


/*[*/
//---------------------------------------------------------------------------
// drawn face types
//---------------------------------------------------------------------------
enum tTVPDrawFace
{
	dfBoth  = 0,
	dfAlpha = 0,
	dfAddAlpha = 4,
	dfMain = 1,
	dfOpaque = 1,
	dfMask = 2,
	dfProvince = 3,
	dfAuto = 128 // face is chosen automatically from the layer type
};
//---------------------------------------------------------------------------
/*]*/


/*[*/
//---------------------------------------------------------------------------
// alias to blending types
//---------------------------------------------------------------------------
enum tTVPBlendOperationMode
{
	omPsNormal = ltPsNormal,
	omPsAdditive = ltPsAdditive,
	omPsSubtractive = ltPsSubtractive,
	omPsMultiplicative = ltPsMultiplicative,
	omPsScreen = ltPsScreen,
	omPsOverlay = ltPsOverlay,
	omPsHardLight = ltPsHardLight,
	omPsSoftLight = ltPsSoftLight,
	omPsColorDodge = ltPsColorDodge,
	omPsColorDodge5 = ltPsColorDodge5,
	omPsColorBurn = ltPsColorBurn,
	omPsLighten = ltPsLighten,
	omPsDarken = ltPsDarken,
	omPsDifference = ltPsDifference,
	omPsDifference5 = ltPsDifference5,
	omPsExclusion = ltPsExclusion,
	omAdditive = ltAdditive,
	omSubtractive = ltSubtractive,
	omMultiplicative = ltMultiplicative,
	omDodge = ltDodge,
	omDarken = ltDarken,
	omLighten = ltLighten,
	omScreen = ltScreen,
	omAlpha = ltTransparent,
	omAddAlpha = ltAddAlpha,
	omOpaque = ltCoverRect,

	omAuto = 128   // operation mode is guessed from the source layer type
};
//---------------------------------------------------------------------------
/*]*/




/*[*/
//---------------------------------------------------------------------------
// layer hit test type
//---------------------------------------------------------------------------
enum tTVPHitType {htMask, htProvince};
//---------------------------------------------------------------------------
/*]*/



/*[*/
//---------------------------------------------------------------------------
// color key types
//---------------------------------------------------------------------------
#define TVP_clAdapt				((tjs_uint32)(0x01ffffff))
#define TVP_clNone				((tjs_uint32)(0x1fffffff))
#define TVP_clPalIdx			((tjs_uint32)(0x03000000))
#define TVP_clAlphaMat			((tjs_uint32)(0x04000000))
#define TVP_Is_clPalIdx(n)		((tjs_uint32)(((n)&0xff000000) == TVP_clPalIdx))
#define TVP_get_clPalIdx(n)		((tjs_uint32)((n)&0xff))
#define TVP_Is_clAlphaMat(n)	((tjs_uint32)(((n)&0xff000000) == TVP_clAlphaMat))
#define TVP_get_clAlphaMat(n)	((tjs_uint32)((n)&0xffffff))
//---------------------------------------------------------------------------
/*]*/


//---------------------------------------------------------------------------
// update optimization options
//---------------------------------------------------------------------------
#define TVP_UPDATE_UNITE_LIMIT 300
#define TVP_CACHE_UNITE_LIMIT 60
#define TVP_EXPOSED_UNITE_LIMIT 30
#define TVP_DIRECT_UNITE_LIMIT 10
//---------------------------------------------------------------------------





//---------------------------------------------------------------------------
// TVPToActualColor :  convert color identifier to actual color
//---------------------------------------------------------------------------
TJS_EXP_FUNC_DEF(tjs_uint32, TVPToActualColor, (tjs_uint32 col));
	// implement in each platform
TJS_EXP_FUNC_DEF(tjs_uint32, TVPFromActualColor, (tjs_uint32 col));
	// implement in each platform
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// Plugin service functions
//---------------------------------------------------------------------------
class tTJSNI_BaseLayer;
TJS_EXP_FUNC_DEF(iTJSDispatch2 *, TVPGetObjectFrom_NI_BaseLayer,   (tTJSNI_BaseLayer * layer));
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_BaseLayer implementation
//---------------------------------------------------------------------------
class tTJSNI_BaseWindow;
class tTVPBaseBitmap;
class tTVPLayerManager;
class tTJSNI_BaseLayer :
	public tTJSNativeInstance, public tTVPDrawable,
	public tTVPCompactEventCallbackIntf
{
	friend class tTVPLayerManager;
	friend iTJSDispatch2 * TVPGetObjectFrom_NI_BaseLayer(tTJSNI_BaseLayer * layer);
protected:
	iTJSDispatch2 *Owner;
	tTJSVariantClosure ActionOwner;

	//---------------------------------------------- object lifetime stuff --
public:
	tTJSNI_BaseLayer(void);
	~tTJSNI_BaseLayer();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

	iTJSDispatch2 * GetOwnerNoAddRef() const { return Owner; }

	tTJSVariantClosure GetActionOwnerNoAddRef() const { return ActionOwner; }


private:
	bool Shutdown; // true when shutting down
	bool CompactEventHookInit;
	void RegisterCompactEventHook();
	void TJS_INTF_METHOD OnCompact(tjs_int level); // method from tTVPCompactEventCallbackIntf

	//----------------------------------------------- interface to manager --
private:
	tTVPLayerManager *Manager;
public:
	tTVPLayerManager *GetManager() const { return Manager; }
	class iTVPLayerTreeOwner * GetLayerTreeOwner() const;

	//---------------------------------------------------- tree management --
private:
	tTJSNI_BaseLayer *Parent;
	tObjectList<tTJSNI_BaseLayer> Children;
	ttstr Name;
	bool Visible;
	tjs_int Opacity;
	tjs_uint OrderIndex;
		// index in parent's 'Children' array.
		// do not access this variable directly; use GetOrderIndex() instead.
	tjs_uint OverallOrderIndex;
		// index in overall tree.
		// use GetOverallOrderIndex instead
	bool AbsoluteOrderMode; // manage Z order in absolute Z position
	tjs_int AbsoluteOrderIndex;
		// do not access this variable directly; use GetAbsoluteOrderIndex() instead. 

	bool ChildrenOrderIndexValid;

	tjs_int VisibleChildrenCount;
	iTJSDispatch2 *ChildrenArray; // Array object which holds children array ...
	iTJSDispatch2 *ArrayClearMethod; // holds Array.clear method
	bool ChildrenArrayValid;

	void Join(tTJSNI_BaseLayer *parent); // join to the parent
	void Part(); // part from the parent

	void AddChild(tTJSNI_BaseLayer *child);
	void SeverChild(tTJSNI_BaseLayer *child);

public:
	iTJSDispatch2 * GetChildrenArrayObjectNoAddRef();

private:

	tTJSNI_BaseLayer* GetAncestorChild(tTJSNI_BaseLayer *ancestor);
		// retrieve "ancestor"'s child that is ancestor of this ( can be thisself )
	bool IsAncestor(tTJSNI_BaseLayer *ancestor);
		// is "ancestor" is ancestor of this layer ? (cannot be itself)
	bool IsAncestorOrSelf(tTJSNI_BaseLayer *ancestor)
		{ return ancestor == this || IsAncestor(ancestor); }
			// same as IsAncestor (but can be itself)

	void RecreateOverallOrderIndex(tjs_uint& index,
		std::vector<tTJSNI_BaseLayer*>& nodes);

	void Exchange(tTJSNI_BaseLayer *target, bool keepchild = false);
		// exchange this for the other layer
	void Swap(tTJSNI_BaseLayer *target) // swap this for the other layer
		{ Exchange(target, true); }

	tjs_int GetVisibleChildrenCount()
	{ if(VisibleChildrenCount == -1) CheckChildrenVisibleState();
		return VisibleChildrenCount; }

	void CheckChildrenVisibleState();
	void NotifyChildrenVisualStateChanged();

	void RecreateOrderIndex();

public:
	tjs_uint GetOrderIndex()
	{
		if(!Parent) return 0;
		if(Parent->ChildrenOrderIndexValid) return OrderIndex;
		Parent->RecreateOrderIndex();
		return OrderIndex;
	}

	// SetOrderIndex are below

	tjs_uint GetOverallOrderIndex();

	tTJSNI_BaseLayer * GetParent() const { return Parent; }
	void SetParent(tTJSNI_BaseLayer *parent) { Join(parent); }

	tjs_uint GetCount() { return Children.GetActualCount(); }

	tTJSNI_BaseLayer * GetChildren(tjs_int idx)
		{ Children.Compact(); return Children[idx]; }

	const ttstr & GetName() const { return Name; }
	void SetName(const ttstr &name) { Name = name; }
	bool GetVisible() const { return Visible; }
	void SetVisible(bool st);
	tjs_int GetOpacity() const { return Opacity; }
	void SetOpacity(tjs_int opa);
	bool IsSeen() const { return Visible && Opacity != 0; }
	bool IsPrimary() const;

	bool GetParentVisible() const; // is parent visible? this does not check opacity
	bool GetNodeVisible() { return GetParentVisible() && Visible; } // this does not check opacity

	tTJSNI_BaseLayer * GetNeighborAbove(bool loop = false);
	tTJSNI_BaseLayer * GetNeighborBelow(bool loop = false);

private:
	void CheckZOrderMoveRule(tTJSNI_BaseLayer *lay);
	void ChildChangeOrder(tjs_int from, tjs_int to);
	void ChildChangeAbsoluteOrder(tjs_int from, tjs_int abs_to);


public:
	void MoveBefore(tTJSNI_BaseLayer *lay); // move before sibling : lay
	void MoveBehind(tTJSNI_BaseLayer *lay); // move behind sibling : lay

	void SetOrderIndex(tjs_int index);

	void BringToBack(); // to most back position
	void BringToFront(); // to most front position

	tjs_int GetAbsoluteOrderIndex(); // retrieve order index in absolute position
	void SetAbsoluteOrderIndex(tjs_int index);

	bool GetAbsoluteOrderMode() const { return AbsoluteOrderMode; }
	void SetAbsoluteOrderMode(bool b);

public:
	void DumpStructure(int level = 0);  // dump layer structure to dms



	//--------------------------------------------- layer type management --
protected:
	tTVPLayerType Type; // user set Type
	tTVPLayerType DisplayType; // actual Type
		// note that Type and DisplayType are different when Type = {ltEffect|ltFilter}

	void NotifyLayerTypeChange();

	void UpdateDrawFace(); // set DrawFace from Face and Type
public:
	tTVPBlendOperationMode GetOperationModeFromType() const;
		// returns corresponding blend operation mode from layer type

public:
	tTVPLayerType GetType() const { return Type; }
	void SetType(tTVPLayerType type);

	const tjs_char * GetTypeNameString();

	void ConvertLayerType(tTVPDrawFace fromtype);

	//-------------------------------------------- geographical management --
protected:
	tTVPRect Rect;
	bool ExposedRegionValid;
	tTVPComplexRect ExposedRegion; // exposed region (no-children-overlapped-region)
	tTVPComplexRect OverlappedRegion; // overlapped region (overlapped by children)
		// above two shuld not be accessed directly

	void SetToCreateExposedRegion() { ExposedRegionValid = false; }
	void CreateExposedRegion(); // create exposed/overlapped region information
	const tTVPComplexRect & GetExposedRegion()
		{ if(!ExposedRegionValid) CreateExposedRegion();
		  return ExposedRegion; }
	const tTVPComplexRect & GetOverlappedRegion()
		{ if(!ExposedRegionValid) CreateExposedRegion();
		  return OverlappedRegion; }

	void InternalSetSize(tjs_uint width, tjs_uint height);
	void InternalSetBounds(const tTVPRect &rect);

	void ConvertToParentRects(tTVPComplexRect &dest, const tTVPComplexRect &src);

private:

public:
	const tTVPRect & GetRect() const { return Rect; }
	tjs_int GetLeft() const { return Rect.left; }
	tjs_int GetTop() const { return Rect.top; }
	tjs_uint GetWidth() const { return (tjs_uint)Rect.get_width(); }
	tjs_uint GetHeight() const { return (tjs_uint)Rect.get_height(); }


	void SetLeft(tjs_int left);
	void SetTop(tjs_int top);
	void SetPosition(tjs_int left, tjs_int top);
	void SetWidth(tjs_uint width);
	void SetHeight(tjs_uint height);
	void SetSize(tjs_uint width, tjs_uint height);
	void SetBounds(const tTVPRect &rect);

	void ParentRectToChildRect(tTVPRect &rect);

	void ToPrimaryCoordinates(tjs_int &x, tjs_int &y) const;
	void FromPrimaryCoordinates(tjs_int &x, tjs_int &y) const;
	void FromPrimaryCoordinates(tjs_real &x, tjs_real &y) const;

	//-------------------------------------------- image buffer management --
	tTVPBaseTexture *MainImage;
protected:
	bool CanHaveImage; // whether the layer can have image
	tTVPBaseBitmap *ProvinceImage;
	tjs_uint32 NeutralColor; // Neutral Color (which can be set by the user)
	tjs_uint32 TransparentColor; // transparent color (which cannot be set by the user, decided by layer type)

	void ChangeImageSize(tjs_uint width, tjs_uint height);

	tjs_int ImageLeft; // image offset left
	tjs_int ImageTop; // image offset top

	void AllocateImage();
	void DeallocateImage();
	void AllocateProvinceImage();
	void DeallocateProvinceImage();
	void AllocateDefaultImage();

public:
	void AssignImages(tTJSNI_BaseLayer *src); // assign image content

    void AssignMainImage(iTVPBaseBitmap *bmp);
		// assign single main bitmap image. the image size assigned must be
		// identical to the destination layer bitmap.

	void AssignMainImageWithUpdate(iTVPBaseBitmap *bmp);
	void CopyFromMainImage( class tTJSNI_Bitmap* bmp );
#ifndef TVP_REVRGB
#define TVP_REVRGB(v) ((v & 0xFF00FF00) | ((v >> 16) & 0xFF) | ((v & 0xFF) << 16))
#endif
	void SetNeutralColor(tjs_uint32 color) { NeutralColor = TVP_REVRGB(color); }
	tjs_uint32 GetNeutralColor() const { return TVP_REVRGB(NeutralColor); }

	void SetHasImage(bool b);
	bool GetHasImage() const;

public:
	void SetImageLeft(tjs_int left);
	tjs_int GetImageLeft() const;
	void SetImageTop(tjs_int top);
	tjs_int GetImageTop() const;
	void SetImagePosition(tjs_int left, tjs_int top);
	void SetImageWidth(tjs_uint width);
	tjs_uint GetImageWidth() const;
	void SetImageHeight(tjs_uint height);
	tjs_uint GetImageHeight() const;

private:
	void InternalSetImageSize(tjs_uint width, tjs_uint height);
public:
	void SetImageSize(tjs_uint width, tjs_uint height);
private:
	void ImageLayerSizeChanged(); // called from geographical management
public:
	tTVPBaseTexture * GetMainImage() { ApplyFont(); return MainImage; }
	tTVPBaseBitmap * GetProvinceImage() { ApplyFont(); return ProvinceImage; }
		// exporting of these two members is a bit dangerous
		// in the manner of protecting
		// ( user can resize the image with the returned object !? )

	void IndependMainImage(bool copy = true);
	void IndependProvinceImage(bool copy = true);

	void SaveLayerImage(const ttstr &name, const ttstr &type);

	void AssignTexture(class iTVPTexture2D *tex);
	iTJSDispatch2 * LoadImages(const ttstr &name, tjs_uint32 colorkey);

	void LoadProvinceImage(const ttstr &name);

	tjs_uint32 GetMainPixel(tjs_int x, tjs_int y) const;
	void SetMainPixel(tjs_int x, tjs_int y, tjs_uint32 color);
	tjs_int GetMaskPixel(tjs_int x, tjs_int y) const;
	void SetMaskPixel(tjs_int x, tjs_int y, tjs_int mask);
	tjs_int GetProvincePixel(tjs_int x, tjs_int y) const;
	void SetProvincePixel(tjs_int x, tjs_int y, tjs_int n);

	const void * GetMainImagePixelBuffer() const;
	void * GetMainImagePixelBufferForWrite();
	tjs_int GetMainImagePixelBufferPitch() const;
	const void * GetProvinceImagePixelBuffer() const;
	void * GetProvinceImagePixelBufferForWrite();
	tjs_int GetProvinceImagePixelBufferPitch() const;

	//---------------------------------- input event / hit test management --
private:
	tTVPHitType HitType;
	tjs_int HitThreshold;
	bool OnHitTest_Work;
	bool Enabled; // is layer enabled for input?
	bool EnabledWork; // work are for delivering onNodeEnabled or onNodeDisabled
	bool Focusable; // is layer focusable ?
	bool JoinFocusChain; // does layer join the focus chain ?
	tTJSNI_BaseLayer *FocusWork;

	tjs_int AttentionLeft; // attention position
	tjs_int AttentionTop;
	tjs_int UseAttention;

	tTVPImeMode ImeMode; // ime mode

	tjs_int Cursor; // mouse cursor
	tjs_int CursorX_Work; // holds x-coordinate in SetCursorX and SetCursorY

public:
	void SetCursorByStorage(const ttstr &storage);
	void SetCursorByNumber(tjs_int num);


private:
	tjs_int GetLayerActiveCursor(); // return layer's actual (active) mouse cursor
	void SetCurrentCursorToWindow(); // set current layer cusor to the window

private:
	bool _HitTestNoVisibleCheck(tjs_int x, tjs_int y);

public:
	bool HitTestNoVisibleCheck(tjs_int x, tjs_int y);
	void SetHitTestWork(bool b) { OnHitTest_Work = b; }

	tjs_int GetCursor() const { return Cursor; }

private:
	ttstr Hint; // layer hint text
	bool ShowParentHint; // show parent's hint ?
	bool IgnoreHintSensing;

	void SetCurrentHintToWindow(); // set current hint to the window

public:
	const ttstr & GetHint() const { return Hint; }
	void SetHint(const ttstr & hint);
	bool GetShowParentHint() const { return ShowParentHint; }
	void SetShowParentHint(bool b) { ShowParentHint = b; }
	bool GetIgnoreHintSensing() const { return IgnoreHintSensing; }
	void SetIgnoreHintSensing(bool b) { IgnoreHintSensing = b; }

public:
	tjs_int GetAttentionLeft() const { return AttentionLeft; }
	void SetAttentionLeft(tjs_int l);
	tjs_int GetAttentionTop() const { return AttentionTop; }
	void SetAttentionTop(tjs_int t);
	void SetAttentionPoint(tjs_int l, tjs_int t);
	bool GetUseAttention() const { return 0!=UseAttention; }
	void SetUseAttention(bool b);

private:

public:
	void SetImeMode(tTVPImeMode mode);
	tTVPImeMode GetImeMode() const { return ImeMode; }

public:
	// these mouse cursor position functions are based on layer's coordinates,
	// not layer image's coordinates.
	tjs_int GetCursorX();
	void SetCursorX(tjs_int x);
	tjs_int GetCursorY();
	void SetCursorY(tjs_int y);
	void SetCursorPos(tjs_int x, tjs_int y);

private:
	bool GetMostFrontChildAt(tjs_int x, tjs_int y, tTJSNI_BaseLayer **lay,
		const tTJSNI_BaseLayer *except = NULL, bool get_disabled = false);

public:
	tTJSNI_BaseLayer * GetMostFrontChildAt(tjs_int x, tjs_int y, bool exclude_self = false,
		bool get_disabled = false);

	tTVPHitType GetHitType() const { return HitType; }
	void SetHitType(tTVPHitType type) { HitType = type; }

	tjs_int GetHitThreshold() const { return HitThreshold; }
	void SetHitThreshold(tjs_int t) { HitThreshold = t; }

private:
	void FireClick(tjs_int x, tjs_int y);
	void FireDoubleClick(tjs_int x, tjs_int y);
	void FireMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	void FireMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags);
	void FireMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags);
	void FireMouseEnter();
	void FireMouseLeave();

	void FireTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id );
	void FireTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag );
	void FireTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag );
	void FireMultiTouch();

public:
	void ReleaseCapture();
	void ReleaseTouchCapture( tjs_uint32 id, bool all = false );

private:
	tTJSNI_BaseLayer *SearchFirstFocusable(bool ignore_chain_focusable = true);

	tTJSNI_BaseLayer *_GetPrevFocusable(); // search next focusable layer backward
	tTJSNI_BaseLayer *_GetNextFocusable(); // search next focusable layer forward

public:
	tTJSNI_BaseLayer *GetPrevFocusable();
	tTJSNI_BaseLayer *GetNextFocusable();
	void SetFocusWork(tTJSNI_BaseLayer *lay)
		{ FocusWork = lay; }

public:

	tTJSNI_BaseLayer * FocusPrev();
	tTJSNI_BaseLayer * FocusNext();

	void SetFocusable(bool b);
	bool GetFocusable() const { return Focusable; }

	void SetJoinFocusChain(bool b) { JoinFocusChain = b; }
	bool GetJoinFocusChain() const { return JoinFocusChain; }

	bool CheckFocusable() const { return Focusable && Visible && Enabled; }
	bool ParentFocusable() const;
	bool GetNodeFocusable()
		{ return CheckFocusable() && ParentFocusable() && !IsDisabledByMode(); }

	bool SetFocus(bool direction = true);

	bool GetFocused();

private:
	void FireBlur(tTJSNI_BaseLayer *prevfocused);
	void FireFocus(tTJSNI_BaseLayer *prevfocused, bool direction);
	tTJSNI_BaseLayer * FireBeforeFocus(tTJSNI_BaseLayer *prevfocused, bool direction);

public:
	void SetMode();
	void RemoveMode();

	bool IsDisabledByMode();  // is "this" layer disable by modal layer?

private:

public:
	void NotifyNodeEnabledState();
	void SaveEnabledWork();
	void SetEnabled(bool b);
	bool ParentEnabled();
	bool GetEnabled() const { return Enabled; }

	bool GetNodeEnabled()
		{ return GetEnabled() && ParentEnabled() && !IsDisabledByMode(); }

private:
	void FireKeyDown(tjs_uint key, tjs_uint32 shift);
	void FireKeyUp(tjs_uint key, tjs_uint32 shift);
	void FireKeyPress(tjs_char key);
	void FireMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y);

public:
	void DefaultKeyDown(tjs_uint key, tjs_uint32 shift); // default keyboard behavior
	void DefaultKeyUp(tjs_uint key, tjs_uint32 shift); // default keyboard behavior
	void DefaultKeyPress(tjs_char key); // default keyboard behavior

	//--------------------------------------------------- cache management --
private:
	tTVPBaseTexture *CacheBitmap;
	//tTVPComplexRect CachedRegion;

	void AllocateCache();
	void ResizeCache();
	void DeallocateCache();
	void DispSizeChanged(); // is called from geographical management
	void CompactCache(); // free cache image if the cache is not needed

	tjs_uint CacheEnabledCount;

	bool Cached;  // script-controlled cached state

public:
	bool GetCacheEnabled() const { return CacheEnabledCount!=0; }


	tjs_uint IncCacheEnabledCount();
	tjs_uint DecCacheEnabledCount();

	void SetCached(bool b);
	bool GetCached() const { return Cached; }

	//--------------------------------------------- drawing function stuff --
protected:
	tTVPDrawFace DrawFace; // (actual) current drawing layer face
	tTVPDrawFace Face; // (outward) current drawing layer face

	bool HoldAlpha; // whether the layer alpha channel is to be kept
		// when the layer type is ltOpaque

public:
	void SetFace(tTVPDrawFace f) { Face = f; UpdateDrawFace(); }
	tTVPDrawFace GetFace() const { return Face; }

	void SetHoldAlpha(bool b)  { HoldAlpha = b; }
	bool GetHoldAlpha() const  { return HoldAlpha; }

protected:
	bool ImageModified; // flag to know modification of layer image
	tTVPRect ClipRect; // clipping rectangle
public:
	void ResetClip();
	void SetClip(tjs_int left, tjs_int top, tjs_int width, tjs_int height);
	tjs_int GetClipLeft() const { return ClipRect.left; }
	void SetClipLeft(tjs_int left);
	tjs_int GetClipTop() const { return ClipRect.top; }
	void SetClipTop(tjs_int top);
	tjs_int GetClipWidth() const { return ClipRect.right - ClipRect.left; }
	void SetClipWidth(tjs_int width);
	tjs_int GetClipHeight() const { return ClipRect.bottom - ClipRect.top; }
	void SetClipHeight(tjs_int height);
    const tTVPRect& GetClip() const { return ClipRect; }

private:
	bool ClipDestPointAndSrcRect(tjs_int &dx, tjs_int &dy,
		tTVPRect &srcrectout, const tTVPRect &srcrect) const;


private:
	bool GetBltMethodFromOperationModeAndDrawFace(
		tTVPBBBltMethod & result,
		tTVPBlendOperationMode mode);	

public:
	void FillRect(const tTVPRect &rect, tjs_uint32 color);
	void ColorRect(const tTVPRect &rect, tjs_uint32 color, tjs_int opa);

	void DrawText(tjs_int x, tjs_int y, const ttstr &text, tjs_uint32 color,
		tjs_int opa, bool aa, tjs_int shadowlevel, tjs_uint32 shadowcolor,
			tjs_int shadowwidth,
		tjs_int shadowofsx, tjs_int shadowofsy);

	void DrawGlyph(tjs_int x, tjs_int y, iTJSDispatch2* glyph, tjs_uint32 color,
		tjs_int opa, bool aa, tjs_int shadowlevel, tjs_uint32 shadowcolor,
		tjs_int shadowwidth, tjs_int shadowofsx, tjs_int shadowofsy);

	void PiledCopy(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
		const tTVPRect &rect);

	void CopyRect(tjs_int dx, tjs_int dy, iTVPBaseBitmap *src, iTVPBaseBitmap *provincesrc,
		const tTVPRect &rect);
	
	bool Copy9Patch( const iTVPBaseBitmap *src, tTVPRect &margin );

	void StretchCopy(const tTVPRect &destrect, iTVPBaseBitmap *src,
		const tTVPRect &rect, tTVPBBStretchType mode = stNearest, tjs_real typeopt = 0.0);

	void AffineCopy(const t2DAffineMatrix &matrix, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBBStretchType mode = stNearest, bool clear = false);

	void AffineCopy(const tTVPPointD *points, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBBStretchType mode = stNearest, bool clear = false);

	void PileRect(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
		const tTVPRect &rect, tjs_int opacity = 255);

	void BlendRect(tjs_int dx, tjs_int dy, tTJSNI_BaseLayer *src,
		const tTVPRect &rect, tjs_int opacity = 255);

	void OperateRect(tjs_int dx, tjs_int dy, iTVPBaseBitmap *src,
		const tTVPRect &rect, tTVPBlendOperationMode mode = omAuto,
			tjs_int opacity = 255);

	void StretchPile(const tTVPRect &destrect, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
			tTVPBBStretchType type = stNearest);

	void StretchBlend(const tTVPRect &destrect, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
			tTVPBBStretchType type = stNearest);

	void OperateStretch(const tTVPRect &destrect, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode = omAuto, tjs_int opacity = 255,
			tTVPBBStretchType type = stNearest, tjs_real typeopt = 0.0);

	void AffinePile(const t2DAffineMatrix &matrix, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void AffinePile(const tTVPPointD *points, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void AffineBlend(const t2DAffineMatrix &matrix, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void AffineBlend(const tTVPPointD *points, tTJSNI_BaseLayer *src,
		const tTVPRect &srcrect, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void OperateAffine(const t2DAffineMatrix &matrix, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode = omAuto, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void OperateAffine(const tTVPPointD *points, iTVPBaseBitmap *src,
		const tTVPRect &srcrect, tTVPBlendOperationMode mode = omAuto, tjs_int opacity = 255,
		tTVPBBStretchType type = stNearest);

	void DoBoxBlur(tjs_int xblur = 1, tjs_int yblur = 1);

	void AdjustGamma(const tTVPGLGammaAdjustData & data);
	void DoGrayScale();
	void LRFlip();
	void UDFlip();

	bool GetImageModified() const { return ImageModified; }
	void SetImageModified(bool b) { ImageModified = b; }


	//------------------------------------------- interface to font object --

private:
	tTVPFont Font;
	bool FontChanged;
	iTJSDispatch2 *FontObject;

	void ApplyFont();

public:
	void SetFontFace(const ttstr & face);
	ttstr GetFontFace() const;
	void SetFontHeight(tjs_int height);
	tjs_int GetFontHeight() const;
	void SetFontAngle(tjs_int angle);
	tjs_int GetFontAngle() const;
	void SetFontBold(bool b);
	bool GetFontBold() const;
	void SetFontItalic(bool b);
	bool GetFontItalic() const;
	void SetFontStrikeout(bool b);
	bool GetFontStrikeout() const;
	void SetFontUnderline(bool b);
	bool GetFontUnderline() const;
	void SetFontFaceIsFileName(bool b);
	bool GetFontFaceIsFileName() const;


	tjs_int GetTextWidth(const ttstr & text);
	tjs_int GetTextHeight(const ttstr & text);
	double GetEscWidthX(const ttstr & text);
	double GetEscWidthY(const ttstr & text);
	double GetEscHeightX(const ttstr & text);
	double GetEscHeightY(const ttstr & text);
	void GetFontGlyphDrawRect( const ttstr & text, tTVPRect& area );
// 	bool DoUserFontSelect(tjs_uint32 flags, const ttstr &caption,
// 		const ttstr &prompt, const ttstr &samplestring);

	void GetFontList(tjs_uint32 flags, std::vector<ttstr> & list);

	void MapPrerenderedFont(const ttstr & storage);
	void UnmapPrerenderedFont();

	const tTVPFont& GetFont() const;

	iTJSDispatch2 * GetFontObjectNoAddRef();


	//------------------------------------------------ updating management --
protected:
	tjs_int UpdateOfsX, UpdateOfsY;
	tTVPRect UpdateRectForChild; // to be used in tTVPDrawable::GetDrawTargetBitmap
	tjs_int UpdateRectForChildOfsX;
	tjs_int UpdateRectForChildOfsY;
	tTVPDrawable * CurrentDrawTarget; // set by Draw
	tTVPBaseTexture *UpdateBitmapForChild; // to be used in tTVPDrawable::GetDrawTargetBitmap
	tTVPRect UpdateExcludeRect; // rectangle whose update is not be needed

	tTVPComplexRect CacheRecalcRegion; // region that must be reconstructed for cache
	tTVPComplexRect DrawnRegion; // region that is already marked as "blitted"
	bool DirectTransferToParent; // child image should be directly transfered into parent

	bool CallOnPaint; // call onPaint event when flaged

	void UpdateTransDestinationOnSelfUpdate(const tTVPComplexRect &region);
	void UpdateTransDestinationOnSelfUpdate(const tTVPRect &rect);

	void UpdateChildRegion(tTJSNI_BaseLayer *child, const tTVPComplexRect &region,
		bool tempupdate, bool targnodevisible, bool addtoprimary);

	void InternalUpdate(const tTVPRect &rect, bool tempupdate = false);

public:
	void Update(tTVPComplexRect &rects, bool tempupdate = false);
	void Update(const tTVPRect &rect, bool tempupdate = false);
	void Update(bool tempupdate = false);

	void UpdateAllChildren(bool tempupdate = false);

	void UpdateByScript(const tTVPRect &rect) { CallOnPaint = true; Update(rect); }
	void UpdateByScript() { CallOnPaint = true; Update(); }

	void SetCallOnPaint(bool b) { CallOnPaint = b; }
	bool GetCallOnPaint() const { return CallOnPaint; }

    //void InternalDrawNoCache_GPU(tTVPDrawable *target, const tTVPRect &rect);
    void InternalDrawNoCache_CPU(tTVPDrawable *target, const tTVPRect &rect);
	virtual void Draw_GPU(tTVPDrawable *target, int x, int y, const tTVPRect &r, bool visiblecheck = true);

private:
	void ParentUpdate();  // called when layer moves


	bool InCompletion; // update/completion pipe line is processing
	void BeforeCompletion(); // called before the drawing is processed
	void AfterCompletion(); // called after the drawing is processed

	void QueryUpdateExcludeRect(tTVPRect &rect, bool parentvisible);
		// query update exclude rect ( checks completely opaque area )

    static void BltImage(iTVPBaseBitmap *dest, tTVPLayerType targettype, tjs_int destx,
        tjs_int desty, iTVPBaseBitmap *src, const tTVPRect &srcrect,
		tTVPLayerType drawtype, tjs_int opacity, bool hda = false);

	void DrawSelf(tTVPDrawable *target, tTVPRect &pr, 
		tTVPRect &cr);
	void CopySelfForRect(iTVPBaseBitmap *dest, tjs_int destx, tjs_int desty,
		const tTVPRect &srcrect);
	void CopySelf(iTVPBaseBitmap *dest, tjs_int destx, tjs_int desty,
		const tTVPRect &r);
	void EffectImage(iTVPBaseBitmap *dest, const tTVPRect & destrect);

	void Draw(tTVPDrawable *target, const tTVPRect &r, bool visiblecheck = true);

	// these 3 below are methods from tTVPDrawable
	tTVPBaseTexture * GetDrawTargetBitmap(const tTVPRect &rect,
		tTVPRect &cliprect);
	tTVPLayerType GetTargetLayerType();
	void DrawCompleted(const tTVPRect&destrect, tTVPBaseTexture *bmp,
		const tTVPRect &cliprect,
		tTVPLayerType type, tjs_int opacity) override;

	void InternalComplete2(tTVPComplexRect & updateregion, tTVPDrawable *drawable);
	void InternalComplete2_GPU(tTVPRect updateregion, tTVPDrawable *drawable);
	void InternalComplete(tTVPComplexRect & updateregion, tTVPDrawable *drawable);
	void CompleteForWindow(tTVPDrawable *drawable);
public:
private:
	tTVPBaseTexture * Complete(const tTVPRect & rect);
	tTVPBaseTexture * Complete(); // complete entire area of the layer


	//---------------------------------------------- transition management --
private:
	iTVPDivisibleTransHandler * DivisibleTransHandler;
	iTVPGiveUpdateTransHandler * GiveUpdateTransHandler;

	tTJSNI_BaseLayer * TransDest; // transition destination
	iTJSDispatch2 *TransDestObj;
	tTJSNI_BaseLayer * TransSrc; // transition source
	iTJSDispatch2 *TransSrcObj;

	bool InTransition; // is transition processing?
	bool TransWithChildren; // is transition with children?
	bool TransSelfUpdate; // is transition update performed by user code ?

	tjs_uint64 TransTick; // current tick count
	bool UseTransTickCallback; // whether to use tick source callback function
	tTJSVariantClosure TransTickCallback; // tick callback function

	tTVPTransType TransType; // current transition type
	tTVPTransUpdateType TransUpdateType; // current transition update type

	tTVPScanLineProviderForBaseBitmap * DestSLP; // destination scan line provider
	tTVPScanLineProviderForBaseBitmap * SrcSLP; // source scan line provider

	bool TransCompEventPrevented; // whether "onTransitionCompleted" event is prevented

public:
	void StartTransition(const ttstr &name, bool withchildren,
		tTJSNI_BaseLayer *transsource, tTJSVariantClosure options);

private:
	void InternalStopTransition();

public:
	void StopTransition();

private:
	void StopTransitionByHandler();

	void InvokeTransition(tjs_uint64 tick); // called frequanctly

    void DoDivisibleTransition(iTVPBaseBitmap *dest, tjs_int dx, tjs_int dy,
		const tTVPRect &srcrect);

	struct tTransDrawable : public tTVPDrawable
	{
		// tTVPDrawable class for Transition pipe line rearrangement
		tTJSNI_BaseLayer * Owner;
		tTVPBaseTexture * Target;
		tTVPRect TargetRect;
		tTVPDrawable *OrgDrawable;

		tTVPBaseTexture *Src1Bmp; // tutDivisible
		tTVPBaseTexture *Src2Bmp; // tutDivisible

		void Init(tTJSNI_BaseLayer *owner, tTVPDrawable *org)
		{
			Owner = owner;
			OrgDrawable = org;
			Target = NULL;
		}

		tTVPBaseTexture * GetDrawTargetBitmap(const tTVPRect &rect,
			tTVPRect &cliprect) override;
		tTVPLayerType GetTargetLayerType();
		void DrawCompleted(const tTVPRect&destrect, tTVPBaseTexture *bmp,
			const tTVPRect &cliprect,
			tTVPLayerType type, tjs_int opacity) override;
	} TransDrawable;
	friend struct tTransDrawable;

	struct tTransIdleCallback : public tTVPContinuousEventCallbackIntf
	{
		tTJSNI_BaseLayer * Owner;
		void TJS_INTF_METHOD OnContinuousCallback(tjs_uint64 tick)
			{ Owner->InvokeTransition(tick); }
		// from tTVPIdleEventCallbackIntf
	} TransIdleCallback;
	friend struct tTransIdleCallback;

	tjs_uint64 GetTransTick();

};
//---------------------------------------------------------------------------



#include "LayerImpl.h"


//---------------------------------------------------------------------------
// tTJSNC_Layer : TJS Layer class
//---------------------------------------------------------------------------
class tTJSNC_Layer : public tTJSNativeClass
{
public:
	tTJSNC_Layer();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
	/*
		implement this in each platform.
		this must return a proper instance of tTJSNI_Layer.
	*/
};
//---------------------------------------------------------------------------
extern tTJSNativeClass * TVPCreateNativeClass_Layer();
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_Font : Font Native Object
//---------------------------------------------------------------------------
class tTJSNI_Font : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

	tTVPFont Font;
	tTJSNI_BaseLayer * Layer;
	
	tjs_int GetTextWidthDirect(const ttstr & text);

public:
	tTJSNI_Font();
	~tTJSNI_Font();
	tjs_error TJS_INTF_METHOD Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

	tTJSNI_BaseLayer * GetLayer() const { return Layer; }

	void SetFontFace(const ttstr & face);
	ttstr GetFontFace() const;
	void SetFontHeight(tjs_int height);
	tjs_int GetFontHeight() const;
	void SetFontAngle(tjs_int angle);
	tjs_int GetFontAngle() const;
	void SetFontBold(bool b);
	bool GetFontBold() const;
	void SetFontItalic(bool b);
	bool GetFontItalic() const;
	void SetFontStrikeout(bool b);
	bool GetFontStrikeout() const;
	void SetFontUnderline(bool b);
	bool GetFontUnderline() const;
	void SetFontFaceIsFileName(bool b);
	bool GetFontFaceIsFileName() const;

	tjs_int GetTextWidth(const ttstr & text);
	tjs_int GetTextHeight(const ttstr & text);
	double GetEscWidthX(const ttstr & text);
	double GetEscWidthY(const ttstr & text);
	double GetEscHeightX(const ttstr & text);
	double GetEscHeightY(const ttstr & text);
	void GetFontGlyphDrawRect( const ttstr & text, tTVPRect& area );

	void GetFontList(tjs_uint32 flags, std::vector<ttstr> & list);
	
	void MapPrerenderedFont(const ttstr & storage);
	void UnmapPrerenderedFont();

	const tTVPFont& GetFont() const;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_Font : TJS Font class
//---------------------------------------------------------------------------
class tTJSNC_Font : public tTJSNativeClass
{
public:
	tTJSNC_Font();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance() { return new tTJSNI_Font(); }
};
//---------------------------------------------------------------------------
iTJSDispatch2 * TVPCreateFontObject(iTJSDispatch2 * layer);
extern tTJSNativeClass * TVPCreateNativeClass_Font();
//---------------------------------------------------------------------------


#endif
