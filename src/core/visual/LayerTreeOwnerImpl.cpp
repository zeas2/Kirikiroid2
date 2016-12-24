//---------------------------------------------------------------------------
/**
 * 
 */
//---------------------------------------------------------------------------
//!@file レイヤーツリーオーナー
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "ComplexRect.h"
#include "LayerTreeOwnerImpl.h"

#include <algorithm>
#include "DrawDevice.h"
#include "MsgIntf.h"
#include "LayerIntf.h"
#include "LayerManager.h"
#include "DebugIntf.h"

tTVPLayerTreeOwner::tTVPLayerTreeOwner() : PrimaryLayerManagerIndex(0) {
	DestRect.clear();
}

iTVPLayerManager* tTVPLayerTreeOwner::GetLayerManagerAt(size_t index) {
	if(Managers.size() <= index) return NULL;
	return Managers[index];
}
const iTVPLayerManager* tTVPLayerTreeOwner::GetLayerManagerAt(size_t index) const {
	if(Managers.size() <= index) return NULL;
	return Managers[index];
}
bool tTVPLayerTreeOwner::TransformToPrimaryLayerManager(tjs_int &x, tjs_int &y) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return false;
	// プライマリレイヤマネージャのプライマリレイヤのサイズを得る
	tjs_int pl_w, pl_h;
	if(!manager->GetPrimaryLayerSize(pl_w, pl_h)) return false;
	// x , y は DestRect の 0, 0 を原点とした座標として渡されてきている
	tjs_int w = DestRect.get_width();
	tjs_int h = DestRect.get_height();
	x = w ? (x * pl_w / w) : 0;
	y = h ? (y * pl_h / h) : 0;
	return true;
}
bool tTVPLayerTreeOwner::TransformToPrimaryLayerManager(tjs_real &x, tjs_real &y) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return false;
	// プライマリレイヤマネージャのプライマリレイヤのサイズを得る
	tjs_int pl_w, pl_h;
	if(!manager->GetPrimaryLayerSize(pl_w, pl_h)) return false;
	// x , y は DestRect の 0, 0 を原点とした座標として渡されてきている
	x = pl_w ? (x * DestRect.get_width()  / pl_w) : 0.0;
	y = pl_h ? (y * DestRect.get_height() / pl_h) : 0.0;
	return true;
}
bool tTVPLayerTreeOwner::TransformFromPrimaryLayerManager(tjs_int &x, tjs_int &y) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return false;
	// プライマリレイヤマネージャのプライマリレイヤのサイズを得る
	tjs_int pl_w, pl_h;
	if(!manager->GetPrimaryLayerSize(pl_w, pl_h)) return false;
	// x , y は DestRect の 0, 0 を原点とした座標として渡されてきている
	x = pl_w ? (x * DestRect.get_width()  / pl_w) : 0;
	y = pl_h ? (y * DestRect.get_height() / pl_h) : 0;
	return true;
}
void tTVPLayerTreeOwner::GetPrimaryLayerSize( tjs_int &w, tjs_int &h ) const {
	w = h = 0;
	const iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	if(!manager->GetPrimaryLayerSize(w, h)) {
		w = h = 0;
	}
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::RegisterLayerManager( class iTVPLayerManager* manager ) {
	// Managers に manager を push する。AddRefするのを忘れないこと。
	Managers.push_back(manager);
	manager->AddRef();
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::UnregisterLayerManager( class iTVPLayerManager* manager ) {
	// Managers から manager を削除する。Releaseする。
	std::vector<iTVPLayerManager *>::iterator i = std::find(Managers.begin(), Managers.end(), manager);
	if(i == Managers.end())
		TVPThrowInternalError;
	(*i)->Release();
	Managers.erase(i);
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::SetMouseCursor(class iTVPLayerManager* manager, tjs_int cursor) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnSetMouseCursor( cursor );
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::GetCursorPos(class iTVPLayerManager* manager, tjs_int &x, tjs_int &y) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	OnGetCursorPos(x, y);
	if(primary_manager != manager || !TransformToPrimaryLayerManager(x, y)) {
		// プライマリレイヤマネージャ以外には座標 0,0 で渡しておく
		 x = y = 0;
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::SetCursorPos(class iTVPLayerManager* manager, tjs_int x, tjs_int y) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		if(TransformFromPrimaryLayerManager(x, y))
			OnSetCursorPos(x, y);
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::ReleaseMouseCapture(class iTVPLayerManager* manager) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnReleaseMouseCapture();
	}
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::SetHint(class iTVPLayerManager* manager, iTJSDispatch2* sender, const ttstr &hint) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnSetHintText(sender,hint);
	}
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::NotifyLayerResize(class iTVPLayerManager* manager) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(primary_manager == manager) {
		tjs_int w, h;
		GetPrimaryLayerSize( w, h );
		OnResizeLayer( w, h );
		DestRect.set_size( w, h );
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::NotifyLayerImageChange(class iTVPLayerManager* manager) {
	// change layer image
	for(std::vector<iTVPLayerManager *>::iterator i = Managers.begin(); i != Managers.end(); i++) {
		(*i)->UpdateToDrawDevice();
	}

	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(primary_manager == manager)
		OnChangeLayerImage();
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::SetAttentionPoint(class iTVPLayerManager* manager, tTJSNI_BaseLayer *layer, tjs_int x, tjs_int y) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		if(TransformFromPrimaryLayerManager(x, y))
			OnSetAttentionPoint( layer, x, y);
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::DisableAttentionPoint(class iTVPLayerManager* manager) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnDisableAttentionPoint();
	}
}

void TJS_INTF_METHOD tTVPLayerTreeOwner::SetImeMode( class iTVPLayerManager* manager, tjs_int mode ) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnSetImeMode(mode);
	}
}
void TJS_INTF_METHOD tTVPLayerTreeOwner::ResetImeMode( class iTVPLayerManager* manager ) {
	iTVPLayerManager * primary_manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!primary_manager) return;
	if(primary_manager == manager) {
		OnResetImeMode();
	}
}

void tTVPLayerTreeOwner::FireClick(tjs_int x, tjs_int y) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyClick(x, y);
}
void tTVPLayerTreeOwner::FireDoubleClick(tjs_int x, tjs_int y) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyDoubleClick(x, y);
}
void tTVPLayerTreeOwner::FireMouseDown(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMouseDown(x, y, mb, flags);
}
void tTVPLayerTreeOwner::FireMouseUp(tjs_int x, tjs_int y, tTVPMouseButton mb, tjs_uint32 flags) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMouseUp(x, y, mb, flags);
}
void tTVPLayerTreeOwner::FireMouseMove(tjs_int x, tjs_int y, tjs_uint32 flags) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMouseMove(x, y, flags);
}
void tTVPLayerTreeOwner::FireMouseWheel(tjs_uint32 shift, tjs_int delta, tjs_int x, tjs_int y) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMouseWheel(shift, delta, x, y);
}
void tTVPLayerTreeOwner::FireReleaseCapture() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->ReleaseCapture();
}
void tTVPLayerTreeOwner::FireMouseOutOfWindow() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMouseOutOfWindow();
}

void tTVPLayerTreeOwner::FireTouchDown( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyTouchDown(x, y, cx, cy, id);
}
void tTVPLayerTreeOwner::FireTouchUp( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyTouchUp(x, y, cx, cy, id);
}
void tTVPLayerTreeOwner::FireTouchMove( tjs_real x, tjs_real y, tjs_real cx, tjs_real cy, tjs_uint32 id ) {
	if(!TransformToPrimaryLayerManager(x, y)) return;
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyTouchMove(x, y, cx, cy, id);
}
void tTVPLayerTreeOwner::FireTouchScaling( tjs_real startdist, tjs_real curdist, tjs_real cx, tjs_real cy, tjs_int flag ) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyTouchScaling(startdist, curdist, cx, cy, flag);
}
void tTVPLayerTreeOwner::FireTouchRotate( tjs_real startangle, tjs_real curangle, tjs_real dist, tjs_real cx, tjs_real cy, tjs_int flag ) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyTouchRotate(startangle, curangle, dist, cx, cy, flag);
}
void tTVPLayerTreeOwner::FireMultiTouch() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyMultiTouch();
}

void tTVPLayerTreeOwner::FireKeyDown(tjs_uint key, tjs_uint32 shift) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyKeyDown(key, shift);
}
void tTVPLayerTreeOwner::FireKeyUp(tjs_uint key, tjs_uint32 shift) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyKeyUp(key, shift);
}
void tTVPLayerTreeOwner::FireKeyPress(tjs_char key) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->NotifyKeyPress(key);
}

void tTVPLayerTreeOwner::FireDisplayRotate( tjs_int orientation, tjs_int rotate, tjs_int bpp, tjs_int hresolution, tjs_int vresolution ) {
	// 何もしない
}

void tTVPLayerTreeOwner::FireRecheckInputState() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->RecheckInputState();
}
tTJSNI_BaseLayer* tTVPLayerTreeOwner::GetPrimaryLayer() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return NULL;
	return manager->GetPrimaryLayer();
}
tTJSNI_BaseLayer* tTVPLayerTreeOwner::GetFocusedLayer() {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return NULL;
	return manager->GetFocusedLayer();
}
void tTVPLayerTreeOwner::SetFocusedLayer(tTJSNI_BaseLayer * layer) {
	iTVPLayerManager * manager = GetLayerManagerAt(PrimaryLayerManagerIndex);
	if(!manager) return;
	manager->SetFocusedLayer(layer);
}

