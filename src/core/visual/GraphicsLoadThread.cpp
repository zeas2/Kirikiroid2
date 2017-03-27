
#include "tjsCommHead.h"

#include "BitmapIntf.h"
#include "GraphicsLoadThread.h"
#include "ThreadIntf.h"
#include "NativeEventQueue.h"
#include "UserEvent.h"
#include "EventIntf.h"
#include "StorageIntf.h"
#include "LayerBitmapIntf.h"
#include "MsgIntf.h"
#include "UtilStreams.h"
#include "BitmapBitsAlloc.h"
#include "LayerIntf.h"

tTVPTmpBitmapImage::tTVPTmpBitmapImage()
	: MetaInfo(NULL)
{}
tTVPTmpBitmapImage::~tTVPTmpBitmapImage() {
	if (bmp) {
		delete bmp;
		bmp = NULL;
	}
	if( MetaInfo ) {
		delete MetaInfo;
		MetaInfo = NULL;
	}
}
tTVPImageLoadCommand::tTVPImageLoadCommand() : owner_(NULL), bmp_(NULL), dest_(NULL) {}
tTVPImageLoadCommand::~tTVPImageLoadCommand() {
	if( owner_ ) {
		owner_->Release();
		owner_ = NULL;
	}
	if( dest_ ) {
		delete dest_;
		dest_ = NULL;
	}
	bmp_ = NULL;
}

static int TVPLoadGraphicAsync_SizeCallback(void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)
{
	tTVPTmpBitmapImage* img = (tTVPTmpBitmapImage*)callbackdata;
	if (!img->bmp) {
		img->bmp = new tTVPBitmap(w, h, 32);
	} else if (img->bmp->GetWidth() != w || img->bmp->GetHeight() != h) {
		img->bmp->Release();
		img->bmp = new tTVPBitmap(w, h, 32);
	}
	switch (fmt) {
	case gpfLuminance:
	case gpfRGB:
		img->bmp->IsOpaque = true; break;
	case gpfPalette:
	case gpfRGBA:
		img->bmp->IsOpaque = false; break;
	}
	return img->bmp->GetPitch();
}
//---------------------------------------------------------------------------
static void* TVPLoadGraphicAsync_ScanLineCallback(void *callbackdata, tjs_int y)
{
	tTVPTmpBitmapImage* img = (tTVPTmpBitmapImage*)callbackdata;
	if( y >= 0 ) {
		if( y < (tjs_int)img->bmp->GetHeight() ) {
			return img->bmp->GetScanLine(y);
		} else {
			return NULL;
		}
	}
	return NULL; // -1 の時のフラッシュ処理は何もしない
}
//---------------------------------------------------------------------------
static void TVPLoadGraphicAsync_MetaInfoPushCallback(void *callbackdata, const ttstr & name, const ttstr & value)
{
	tTVPTmpBitmapImage * img = (tTVPTmpBitmapImage *)callbackdata;

	if(!img->MetaInfo) img->MetaInfo = new std::vector<tTVPGraphicMetaInfoPair>();
	img->MetaInfo->push_back(tTVPGraphicMetaInfoPair(name, value));
}
//---------------------------------------------------------------------------

tTVPAsyncImageLoader::tTVPAsyncImageLoader()
: EventQueue(this,&tTVPAsyncImageLoader::Proc), tTVPThread(true)
{
	EventQueue.Allocate();
}
tTVPAsyncImageLoader::~tTVPAsyncImageLoader() {
	ExitRequest();
	WaitFor();
	EventQueue.Deallocate();
	while( CommandQueue.size() > 0 ) {
		tTVPImageLoadCommand* cmd = CommandQueue.front();
		CommandQueue.pop();
		delete cmd;
	}
	while( LoadedQueue.size() > 0 ) {
		tTVPImageLoadCommand* cmd = LoadedQueue.front();
		LoadedQueue.pop();
		delete cmd;
	}
}
void tTVPAsyncImageLoader::ExitRequest() {
	Terminate();
	PushCommandQueueEvent.Set();
}
void tTVPAsyncImageLoader::Execute() {
	// プライオリティは最低にする
	SetPriority(ttpIdle);
	LoadingThread();
}
void tTVPAsyncImageLoader::SendToLoadFinish() {
	NativeEvent ev(TVP_EV_IMAGE_LOAD_THREAD);
	EventQueue.PostEvent(ev);
}
void tTVPAsyncImageLoader::Proc( NativeEvent& ev )
{
	if(ev.Message != TVP_EV_IMAGE_LOAD_THREAD) {
		EventQueue.HandlerDefault(ev);
		return;
	}
	HandleLoadedImage();
}
void tTVPAsyncImageLoader::HandleLoadedImage() {
	bool loading;
	do {
		loading = false;
		tTVPImageLoadCommand* cmd = NULL;
		{
			tTJSCriticalSectionHolder cs(ImageQueueCS);
			if( LoadedQueue.size() > 0 ) {
				cmd = LoadedQueue.front();
				LoadedQueue.pop();
				loading = true;
			}
		}
		if( cmd != NULL ) {
			cmd->bmp_->SetLoading( false );
			if( cmd->result_.length() > 0 ) {
				// error
				tTJSVariant param[4];
				param[0] = tTJSVariant((iTJSDispatch2*)NULL,(iTJSDispatch2*)NULL);
				param[1] = 1; // true async
				param[2] = 1; // true error
				param[3] = cmd->result_; // error_mes
				static ttstr eventname(TJS_W("onLoaded"));
				if (cmd->owner_ && cmd->owner_->IsValid(0, NULL, NULL, cmd->owner_) == TJS_S_TRUE) {
					TVPPostEvent(cmd->owner_, cmd->owner_, eventname, 0, TVP_EPT_IMMEDIATE, 4, param);
				}

				if( cmd->dest_->MetaInfo ) {
					delete cmd->dest_->MetaInfo;
					cmd->dest_->MetaInfo = NULL;
				}
			} else {
				iTJSDispatch2* metainfo = TVPMetaInfoPairsToDictionary(cmd->dest_->MetaInfo);

				cmd->bmp_->SetSizeAndImageBuffer(cmd->dest_->bmp);
				// 読込み完了時にもキャッシュチェック(非同期なので完了前に読み込まれている可能性あり)
				if( TVPHasImageCache( cmd->path_, glmNormal, 0, 0, TVP_clNone ) == false ) {
					TVPPushGraphicCache( cmd->path_, cmd->dest_->bmp, cmd->dest_->MetaInfo );
					cmd->dest_->MetaInfo = NULL;
				} else {
					delete cmd->dest_->MetaInfo;
					cmd->dest_->MetaInfo = NULL;
				}
				cmd->dest_->bmp->Release();
				cmd->dest_->bmp = NULL;

				tTJSVariant param[4];
				param[0] = tTJSVariant(metainfo,metainfo);
				if( metainfo ) metainfo->Release();
				param[1] = 1; // true async
				param[2] = 0; // false error
				param[3] = TJS_W(""); // error_mes
				static ttstr eventname(TJS_W("onLoaded"));
				if (cmd->owner_ && cmd->owner_->IsValid(0, NULL, NULL, cmd->owner_) == TJS_S_TRUE) {
					TVPPostEvent(cmd->owner_, cmd->owner_, eventname, 0, TVP_EPT_IMMEDIATE, 4, param);
				}
			}
			delete cmd;
		}
	} while(loading);
}
//---------------------------------------------------------------------------

// onLoaded( dic, is_async, is_error, error_mes ); エラーは
// sync ( main thead )
void tTVPAsyncImageLoader::LoadRequest( iTJSDispatch2 *owner, tTJSNI_Bitmap* bmp, const ttstr &name ) {
	//tTVPBaseBitmap* dest = new tTVPBaseBitmap( 32, 32, 32 );
	tTVPBaseBitmap dest( TVPGetInitialBitmap() );
	iTJSDispatch2* metainfo = NULL;
	ttstr nname = TVPNormalizeStorageName(name);
	if( TVPCheckImageCache(nname,&dest,glmNormal,0,0,TVP_clNone,&metainfo) ) {
		// キャッシュ内に発見、即座に読込みを完了する
		if (bmp) {
			bmp->CopyFrom(&dest);
			bmp->SetLoading(false);
		}
		if (!owner) return;
		tTJSVariant param[4];
		param[0] = tTJSVariant(metainfo,metainfo);
		if( metainfo ) metainfo->Release();
		param[1] = 0; // false
		param[2] = 0; // false
		param[3] = TJS_W(""); // error_mes
		static ttstr eventname(TJS_W("onLoaded"));
		TVPPostEvent(owner, owner, eventname, 0, TVP_EPT_IMMEDIATE, 4, param);
		return;
	}
	if( TVPIsExistentStorage(name) == false ) {
		TVPThrowExceptionMessage(TVPCannotFindStorage, name);
	}
	ttstr ext = TVPExtractStorageExt(name);
	if(ext == TJS_W("")) {
		TVPThrowExceptionMessage(TJS_W("Filename extension not found/%1"), name);
	}

	PushLoadQueue( owner, bmp, nname );
}

// tTJSCriticalSectionHolder cs_holder(TVPCreateStreamCS);
//	tTJSBinaryStream* stream = TVPCreateStream(nname, TJS_BS_READ);
// TVPCreateStream はロックされているので、非同期で実行可能

void tTVPAsyncImageLoader::PushLoadQueue( iTJSDispatch2 *owner, tTJSNI_Bitmap *bmp, const ttstr &nname ) {
	tTVPImageLoadCommand* cmd = new tTVPImageLoadCommand();
	cmd->owner_ = owner;
	if (owner) owner->AddRef();
	cmd->bmp_ = bmp;
	cmd->path_ = nname;
	cmd->dest_ = new tTVPTmpBitmapImage();
	cmd->result_.Clear();
	{
		// キューをロックしてプッシュ
		tTJSCriticalSectionHolder cs(CommandQueueCS);
		CommandQueue.push(cmd);
	}
	// 追加したことをイベントで通知
	PushCommandQueueEvent.Set();
}
void tTVPAsyncImageLoader::LoadingThread() {
	while( !GetTerminated() ) {
		// キュー追加イベント待ち
		PushCommandQueueEvent.WaitFor(0);
		if( GetTerminated() ) break;
		bool loading;
		do {
			loading = false;
			tTVPImageLoadCommand* cmd = NULL;

			{ // Lock
				tTJSCriticalSectionHolder cs(CommandQueueCS);
				if( CommandQueue.size() ) {
					cmd = CommandQueue.front();
					CommandQueue.pop();
				}
			}
			if( cmd ) {
				loading = true;
				LoadImageFromCommand(cmd);
				{	// Lock
					tTJSCriticalSectionHolder cs(ImageQueueCS);
					LoadedQueue.push(cmd);
				}
				// Send to message
				SendToLoadFinish();
			}
		} while( loading && !GetTerminated() );
	}
}
tTVPGraphicHandlerType* TVPGuessGraphicLoadHandler(ttstr& name);
void tTVPAsyncImageLoader::LoadImageFromCommand( tTVPImageLoadCommand* cmd ) {
	ttstr ext = TVPExtractStorageExt(cmd->path_);
	tTVPGraphicHandlerType* handler = NULL;
	ttstr name(cmd->path_);
	if (ext.IsEmpty()) {
		// missing extension
		handler = TVPGuessGraphicLoadHandler(name);
//		cmd->result_ = TJS_W("Filename extension not found");
	} else {
		handler = TVPGetGraphicLoadHandler(ext);
	}
	if( handler ) {
		try {
			tTVPStreamHolder holder(name);
			handler->Load(handler->FormatData, (void*)cmd->dest_, TVPLoadGraphicAsync_SizeCallback,
				TVPLoadGraphicAsync_ScanLineCallback, TVPLoadGraphicAsync_MetaInfoPushCallback,
				holder.Get(), -1, glmNormal );
		} catch(...) {
			// 例外は全てキャッチ
			cmd->result_ = TVPFormatMessage(TVPImageLoadError, cmd->path_);
		}
	} else {
		// error
		cmd->result_ = TVPFormatMessage(TVPUnknownGraphicFormat, cmd->path_);
	}
}

