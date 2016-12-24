//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
//!@file "PassThrough" 描画デバイス管理
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
#include "DrawDevice.h"
#include "PassThroughDrawDevice.h"
#include "LayerIntf.h"
#include "MsgIntf.h"
#include "SysInitIntf.h"
#include "WindowIntf.h"
#include "DebugIntf.h"
#include "ThreadIntf.h"

#include "TickCount.h"
#include <assert.h>
#include <math.h>
/*
	PassThroughDrawDevice クラスには、Window.PassThroughDrawDevice として
	アクセスできる。通常、Window クラスを生成すると、その drawDevice プロパ
	ティには自動的にこのクラスのインスタンスが設定されるので、(ほかのDrawDevice
	を使わない限りは) 特に意識する必要はない。

	PassThroughDrawDevice は以下のメソッドとプロパティを持つ。

	recreate()
		Drawer (内部で使用している描画方式) を切り替える。preferredDrawer プロパティ
		が dtNone 以外であればそれに従うが、必ず指定された drawer が使用される保証はない。

	preferredDrawer
		使用したい drawer を表すプロパティ。以下のいずれかの値をとる。
		値を設定することも可能。new 直後の値は コマンドラインオプションの dbstyle で
		設定した値になる。
		drawerがこの値になる保証はない (たとえば dtDBD3D を指定していても何らかの
		原因で Direct3D の初期化に失敗した場合は DirectDraw が使用される可能性がある)。
		ウィンドウ作成直後、最初にプライマリレイヤを作成するよりも前にこのプロパティを
		設定する事により、recreate() をわざわざ実行しなくても指定の drawer を使用
		させることができる。
		Window.PassThroughDrawDevice.dtNone			指定しない
		Window.PassThroughDrawDevice.dtDrawDib		拡大縮小が必要な場合はGDI、
													そうでなければDBなし
		Window.PassThroughDrawDevice.dtDBGDI		GDIによるDB
		Window.PassThroughDrawDevice.dtDBDD			DirectDrawによるDB
		Window.PassThroughDrawDevice.dtDBD3D		Direct3DによるDB

	drawer
		現在使用されている drawer を表すプロパティ。以下のいずれかの値をとる。
		読み取り専用。
		Window.PassThroughDrawDevice.dtNone			普通はこれはない
		Window.PassThroughDrawDevice.dtDrawDib		ダブルバッファリング(DB)なし
		Window.PassThroughDrawDevice.dtDBGDI		GDIによるDB
		Window.PassThroughDrawDevice.dtDBDD			DirectDrawによるDB
		Window.PassThroughDrawDevice.dtDBD3D		Direct3DによるDB
*/


//---------------------------------------------------------------------------
// オプション
//---------------------------------------------------------------------------
static tjs_int TVPPassThroughOptionsGeneration = 0;
static bool TVPZoomInterpolation = true;
static tTVPPassThroughDrawDevice::tDrawerType TVPPreferredDrawType = tTVPPassThroughDrawDevice::dtNone;
//---------------------------------------------------------------------------
static void TVPInitPassThroughOptions()
{
	if(TVPPassThroughOptionsGeneration == TVPGetCommandLineArgumentGeneration()) return;
	TVPPassThroughOptionsGeneration = TVPGetCommandLineArgumentGeneration();

	TVPZoomInterpolation = true;
}
//---------------------------------------------------------------------------







//---------------------------------------------------------------------------
//! @brief	PassThrough で用いる描画方法用インターフェース
//---------------------------------------------------------------------------
class tTVPDrawer
{
protected:
	tTVPPassThroughDrawDevice * Device;
	tjs_int DestLeft;
	tjs_int DestTop;
	tjs_int DestWidth;
	tjs_int DestHeight;
	tjs_int SrcWidth;
	tjs_int SrcHeight;
	//SDL_Window* TargetWindow;
	bool DrawUpdateRectangle;
public:
	tTVPDrawer(tTVPPassThroughDrawDevice * device)
	{
		Device = device;
		SrcWidth = 0;
		SrcHeight = 0;
		DestLeft = DestTop = DestWidth = DestHeight = 0;
		//TargetWindow = NULL;
		DrawUpdateRectangle = false;
	} 
	virtual ~tTVPDrawer()
	{
	}
	virtual ttstr GetName() = 0;

	virtual bool SetDestPos(tjs_int left, tjs_int top)
		{ DestLeft = left; DestTop = top; return true; }
	virtual bool SetDestSize(tjs_int width, tjs_int height)
		{ DestWidth = width; DestHeight = height; return true; }
	void GetDestSize(tjs_int &width, tjs_int &height) const
		{ width = DestWidth; height = DestHeight; }
	virtual bool NotifyLayerResize(tjs_int w, tjs_int h)
		{ SrcWidth = w; SrcHeight = h; return true; }
	void GetSrcSize(tjs_int &w, tjs_int &h) const
		{ w = SrcWidth; h = SrcHeight; }
	virtual bool SetDestSizeAndNotifyLayerResize(tjs_int width, tjs_int height, tjs_int w, tjs_int h)
	{
		if(!SetDestSize(width, height)) return false;
		if(!NotifyLayerResize(w, h)) return false;
		return true;
	}
	virtual void SetTargetWindow(int wnd)
	{
		//TargetWindow = wnd;
	}
	virtual void StartBitmapCompletion() = 0;
// 	virtual void NotifyBitmapCompleted(tjs_int x, tjs_int y,
// 		tTVPBaseTexture *bmp,
// 		const tTVPRect &cliprect) = 0;
	virtual void EndBitmapCompletion(iTVPBaseBitmap *bmp) = 0;
	virtual void Show() {;}
	virtual void SetShowUpdateRect(bool b)  { DrawUpdateRectangle = b; }
	virtual int GetInterpolationCapability() { return 3; }
		// bit 0 for point-on-point, bit 1 for bilinear interpolation

	virtual void InitTimings() {;} // notifies begining of benchmark
	virtual void ReportTimings() {;} // notifies end of benchmark

    virtual void Clear() = 0;
};


class tTVPDrawer_Software : public tTVPDrawer
{
public:
	tTVPDrawer_Software(tTVPPassThroughDrawDevice * device) : tTVPDrawer(device)
    {
    }

	virtual ttstr GetName() { return TJS_W("Software rendering"); }

	virtual void StartBitmapCompletion()
	{
		Clear();
// 		SDL_Renderer *renderer = TVPGetPrimaryRender();
// 		SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
// 		SDL_RenderFillRect(renderer, NULL);
	}

    virtual void EndBitmapCompletion(iTVPBaseBitmap *_bmp)
    {
        if(!_bmp) return;

		tTVPRect rcdst(DestLeft, DestTop, DestLeft + DestWidth, DestTop + DestHeight);
		tTVPRect rcsrc(0, 0, SrcWidth, SrcHeight);
		//TVPGetRenderManager()->DrawScreen(rcdst, _bmp->GetTexture(), rcsrc);
//        SDL_DestroyTexture(texture);
//        SDL_FreeSurface(surface);
    }
	virtual void Show()
	{
		/* Update the screen! */
		//TVPGetRenderManager()->PresentScreen();
	}
    virtual void Clear()
    {
		//TVPGetRenderManager()->ClearScreen();
    }
};

//---------------------------------------------------------------------------
tTVPPassThroughDrawDevice::tTVPPassThroughDrawDevice()
{
	TVPInitPassThroughOptions(); // read and initialize options
	PreferredDrawerType = TVPPreferredDrawType;
	//TargetWindow = NULL;
	Drawer = NULL;
	DrawerType = dtNone;
	DestSizeChanged = false;
	SrcSizeChanged = false;
	LockedWidth = LockedHeight = -1;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
tTVPPassThroughDrawDevice::~tTVPPassThroughDrawDevice()
{
	DestroyDrawer();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPPassThroughDrawDevice::DestroyDrawer()
{
	if(Drawer) delete Drawer, Drawer = NULL;
	DrawerType = dtNone;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPPassThroughDrawDevice::CreateDrawer(tDrawerType type)
{
	if(Drawer) delete Drawer, Drawer = NULL;

	switch(type)
	{
	case dtNone:
		break;
    case dtDBD3D:
	case dtDrawDib:
    case dtDBGDI:
    case dtDBDD:
		break;
	}
        Drawer = new tTVPDrawer_Software(this);

	try
	{

// 		if(Drawer)
// 			Drawer->SetTargetWindow(TargetWindow);

		if(Drawer)
		{
			if(!Drawer->SetDestPos(DestRect.left, DestRect.top))
			{
				TVPAddImportantLog(
					TJS_W("Passthrough: Failed to set destination position to draw device drawer"));
				delete Drawer, Drawer = NULL;
			}
		}

		if(Drawer)
		{
			GetSrcSize(SrcWidth, SrcHeight);
			if(!Drawer->SetDestSizeAndNotifyLayerResize(DestRect.get_width(), DestRect.get_height(), SrcWidth, SrcHeight))
			{
				TVPAddImportantLog(
					TJS_W("Passthrough: Failed to set destination size and source layer size to draw device drawer"));
				delete Drawer, Drawer = NULL;
			}
		}

		if(Drawer) DrawerType = type; else DrawerType = dtNone;

		RequestInvalidation(tTVPRect(0, 0, DestRect.get_width(), DestRect.get_height()));
	}
	catch(const eTJS & e)
	{
		TVPAddImportantLog(TJS_W("Passthrough: Failed to create drawer: ") + e.GetMessage());
		DestroyDrawer();
	}
	catch(...)
	{
		TVPAddImportantLog(TJS_W("Passthrough: Failed to create drawer: unknown reason"));
		DestroyDrawer();
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPPassThroughDrawDevice::CreateDrawer(bool zoom_required, bool should_benchmark)
{
    // only create once under android
    CreateDrawer(dtDrawDib);
    return;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void tTVPPassThroughDrawDevice::EnsureDrawer()
{
	// このメソッドでは、以下の条件の際に drawer を作る(作り直す)。
	// 1. Drawer が NULL の場合
	// 2. 現在の Drawer のタイプが適切でなくなったとき
	// 3. 元のレイヤのサイズが変更されたとき
	TVPInitPassThroughOptions();

	//if(TargetWindow)
	{
		// ズームは必要だったか？
		bool zoom_was_required = false;
        bool need_recreate = false;
        tjs_int srcw, srch;
        GetSrcSize(srcw, srch);
        if (Drawer)
		{
            tjs_int drawsrcw, drawsrch;
            Drawer->GetSrcSize(drawsrcw, drawsrch);
			tjs_int destw, desth;
			Drawer->GetDestSize(destw, desth);
			if(destw != srcw || desth != srch)
				zoom_was_required = true;
            if (drawsrcw != srcw || drawsrch != srch)
                need_recreate = true;
            Drawer->SetDestPos(DestRect.left, DestRect.top);
		}

		// ズームは(今回は)必要か？
		bool zoom_is_required = false;
		if(DestRect.get_width() != srcw || DestRect.get_height() != srch)
			zoom_is_required = true;


		bool should_benchmark = false;
		if(!Drawer) need_recreate = true;
		if(zoom_was_required != zoom_is_required) need_recreate = true;
//		if(need_recreate) should_benchmark = true;
//		if(SrcSizeChanged) { SrcSizeChanged = false; need_recreate = true; }
			// SrcSizeChanged という理由だけでは should_benchmark は真には
			// 設定しない

		if(need_recreate)
		{
			// Drawer の再作成が必要
			CreateDrawer(zoom_is_required, should_benchmark);
		}
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::AddLayerManager(iTVPLayerManager * manager)
{
	if(inherited::Managers.size() > 0)
	{
		// "Pass Through" デバイスでは２つ以上のLayer Managerを登録できない
		TVPThrowExceptionMessage(TJS_W("\"passthrough\" device does not support layer manager more than 1"));
			// TODO: i18n
	}
	inherited::AddLayerManager(manager);

	manager->SetDesiredLayerType(ltOpaque); // ltOpaque な出力を受け取りたい
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::SetTargetWindow(int wnd, bool is_main)
{
	TVPInitPassThroughOptions();
	DestroyDrawer();
	//TargetWindow = wnd;
	//IsMainWindow = is_main;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::SetDestRectangle(const tTVPRect & rect)
{
	// 位置だけの変更の場合かどうかをチェックする
	if(rect.get_width() == DestRect.get_width() && rect.get_height() == DestRect.get_height())
	{
		// 位置だけの変更だ
		if(Drawer) Drawer->SetDestPos(rect.left, rect.top);
		inherited::SetDestRectangle(rect);
	}
	else
	{
		// サイズも違う
		DestSizeChanged = true;
		inherited::SetDestRectangle(rect);
		EnsureDrawer();
		if(Drawer)
		{
			if(!Drawer->SetDestSize(rect.get_width(), rect.get_height()))
				DestroyDrawer(); // エラーが起こったのでその drawer を破棄する
		}
	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::NotifyLayerResize(iTVPLayerManager * manager)
{
	inherited::NotifyLayerResize(manager);
	SrcSizeChanged = true;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::Show()
{
	if(Drawer) {
        //TVPDrawCursor();
        Drawer->Show();
    }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::StartBitmapCompletion(iTVPLayerManager * manager)
{
	EnsureDrawer();

	// この中で DestroyDrawer が呼ばれる可能性に注意すること
	//if(Drawer) Drawer->StartBitmapCompletion();

	if(!Drawer)
	{
		// リトライする
		EnsureDrawer();
		//if(Drawer) Drawer->StartBitmapCompletion();
	}

	if (Drawer) Drawer->StartBitmapCompletion();

//     if(TVPForceUpdateScreen) {
//         iTVPBaseBitmap *buffer = manager->GetDrawBuffer();
//         tTVPRect rc;
//         if(buffer) {
//             rc.bottom = buffer->GetHeight();
//             rc.right = buffer->GetWidth();
//             buffer->Fill(rc, 0xFF000000);
//         }
//         ((tTVPLayerManager*)manager)->AddUpdateRegion(rc);
//     }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::NotifyBitmapCompleted(iTVPLayerManager * manager,
	tjs_int x, tjs_int y, tTVPBaseTexture *bmp,
	const tTVPRect &cliprect, tTVPLayerType type, tjs_int opacity)
{
	// bits, bitmapinfo で表されるビットマップの cliprect の領域を、x, y に描画
	// する。
	// opacity と type は無視するしかないので無視する
// 	if(Drawer)
// 	{
// 		TVPInitPassThroughOptions();
// 		Drawer->NotifyBitmapCompleted(x, y, bmp, cliprect);
// 	}
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::EndBitmapCompletion(iTVPLayerManager * manager)
{
//     TVPCopyTexture(
//         NULL, texRect(DestRect.left, DestRect.bottom, DestRect.get_width(), -DestRect.get_height()),
//         manager->GetDrawBuffer()->GetTexture(), texRect(0, 0, SrcWidth, SrcHeight)
//         );
    if (Drawer) {
//         tTVPBaseTexture *bmp = manager->GetDrawBuffer();
//         tTVPRect rc(0, 0, bmp->GetWidth(), bmp->GetHeight());
//         Drawer->NotifyBitmapCompleted(0, 0, bmp, rc);
        Drawer->EndBitmapCompletion(manager->GetDrawBuffer());
    }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTVPPassThroughDrawDevice::SetShowUpdateRect(bool b)
{
	if(Drawer) Drawer->SetShowUpdateRect(b);
}

void tTVPPassThroughDrawDevice::Clear() {
    if(Drawer) Drawer->Clear();
}

//---------------------------------------------------------------------------








//---------------------------------------------------------------------------
// tTJSNC_PassThroughDrawDevice : PassThroughDrawDevice TJS native class
//---------------------------------------------------------------------------
tjs_uint32 tTJSNC_PassThroughDrawDevice::ClassID = (tjs_uint32)-1;
tTJSNC_PassThroughDrawDevice::tTJSNC_PassThroughDrawDevice() :
	tTJSNativeClass(TJS_W("PassThroughDrawDevice"))
{
	// register native methods/properties

	TJS_BEGIN_NATIVE_MEMBERS(PassThroughDrawDevice)
	TJS_DECL_EMPTY_FINALIZE_METHOD
//----------------------------------------------------------------------
// constructor/methods
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_CONSTRUCTOR_DECL(/*var.name*/_this, /*var.type*/tTJSNI_PassThroughDrawDevice,
	/*TJS class name*/PassThroughDrawDevice)
{
	return TJS_S_OK;
}
TJS_END_NATIVE_CONSTRUCTOR_DECL(/*TJS class name*/PassThroughDrawDevice)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/recreate)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
	_this->GetDevice()->SetToRecreateDrawer();
	_this->GetDevice()->EnsureDrawer();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/recreate)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/lockTouchSize)
{
	TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
	if (numparams < 2) return TJS_E_BADPARAMCOUNT;
	_this->GetDevice()->SetLockedSize(param[0]->operator tjs_int(), param[1]->operator tjs_int());
	if (result) result->Clear();
	return TJS_S_OK;
}
TJS_END_NATIVE_METHOD_DECL(/*func. name*/lockTouchSize)


//---------------------------------------------------------------------------
//----------------------------------------------------------------------
// properties
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(interface)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
		*result = reinterpret_cast<tjs_int64>(_this->GetDevice());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(interface)
//----------------------------------------------------------------------
#define TVP_REGISTER_PTDD_ENUM(name) \
	TJS_BEGIN_NATIVE_PROP_DECL(name) \
	{ \
		TJS_BEGIN_NATIVE_PROP_GETTER \
		{ \
			*result = (tjs_int64)tTVPPassThroughDrawDevice::name; \
			return TJS_S_OK; \
		} \
		TJS_END_NATIVE_PROP_GETTER \
		TJS_DENY_NATIVE_PROP_SETTER \
	} \
	TJS_END_NATIVE_PROP_DECL(name)

TVP_REGISTER_PTDD_ENUM(dtNone)
TVP_REGISTER_PTDD_ENUM(dtDrawDib)
TVP_REGISTER_PTDD_ENUM(dtDBGDI)
TVP_REGISTER_PTDD_ENUM(dtDBDD)
TVP_REGISTER_PTDD_ENUM(dtDBD3D)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(preferredDrawer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
		*result = (tjs_int64)(_this->GetDevice()->GetPreferredDrawerType());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_BEGIN_NATIVE_PROP_SETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
		_this->GetDevice()->SetPreferredDrawerType((tTVPPassThroughDrawDevice::tDrawerType)(tjs_int)*param);
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(preferredDrawer)
//----------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(drawer)
{
	TJS_BEGIN_NATIVE_PROP_GETTER
	{
		TJS_GET_NATIVE_INSTANCE(/*var. name*/_this, /*var. type*/tTJSNI_PassThroughDrawDevice);
		*result = (tjs_int64)(_this->GetDevice()->GetDrawerType());
		return TJS_S_OK;
	}
	TJS_END_NATIVE_PROP_GETTER

	TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL(drawer)
//----------------------------------------------------------------------
	TJS_END_NATIVE_MEMBERS
}
//---------------------------------------------------------------------------
iTJSNativeInstance *tTJSNC_PassThroughDrawDevice::CreateNativeInstance()
{
	return new tTJSNI_PassThroughDrawDevice();
}
//---------------------------------------------------------------------------










//---------------------------------------------------------------------------
tTJSNI_PassThroughDrawDevice::tTJSNI_PassThroughDrawDevice()
{
	Device = new tTVPPassThroughDrawDevice();
}
//---------------------------------------------------------------------------
tTJSNI_PassThroughDrawDevice::~tTJSNI_PassThroughDrawDevice()
{
	if(Device) Device->Destruct(), Device = NULL;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
	tTJSNI_PassThroughDrawDevice::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_PassThroughDrawDevice::Invalidate()
{
	if(Device) Device->Destruct(), Device = NULL;
}
//---------------------------------------------------------------------------

