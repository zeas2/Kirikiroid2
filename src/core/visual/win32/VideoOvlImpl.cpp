//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Video Overlay support implementation
//---------------------------------------------------------------------------


#include "tjsCommHead.h"

#include <algorithm>
#include "MsgIntf.h"
#include "VideoOvlImpl.h"
#include "DebugIntf.h"
#include "LayerIntf.h"
#include "LayerBitmapIntf.h"
#include "SysInitIntf.h"
#include "StorageImpl.h"
#include "krmovie.h"
#include "PluginImpl.h"
#include "WaveImpl.h"  // for DirectSound attenuate <-> TVP volume
//#include <evcode.h>

#include "Application.h"
#include "combase.h"

extern void GetVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, struct IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, class iTVPVideoOverlay **out);

extern void GetVideoLayerObject(
	tTJSNI_VideoOverlay* callbackwin, struct IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, class iTVPVideoOverlay **out);

extern void GetMixingVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, struct IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, class iTVPVideoOverlay **out);

extern void GetMFVideoOverlayObject(
	tTJSNI_VideoOverlay* callbackwin, struct IStream *stream, const tjs_char * streamname,
	const tjs_char *type, uint64_t size, class iTVPVideoOverlay **out);

//---------------------------------------------------------------------------
static std::vector<tTJSNI_VideoOverlay *> TVPVideoOverlayVector;
//---------------------------------------------------------------------------
static void TVPAddVideOverlay(tTJSNI_VideoOverlay *ovl)
{
	TVPVideoOverlayVector.push_back(ovl);
}
//---------------------------------------------------------------------------
static void TVPRemoveVideoOverlay(tTJSNI_VideoOverlay *ovl)
{
	std::vector<tTJSNI_VideoOverlay*>::iterator i;
	i = std::find(TVPVideoOverlayVector.begin(), TVPVideoOverlayVector.end(), ovl);
	if(i != TVPVideoOverlayVector.end())
		TVPVideoOverlayVector.erase(i);
}
//---------------------------------------------------------------------------
static void TVPShutdownVideoOverlay()
{
	// shutdown all overlay object and release krmovie.dll / krflash.dll
	std::vector<tTJSNI_VideoOverlay*>::iterator i;
	for(i = TVPVideoOverlayVector.begin(); i != TVPVideoOverlayVector.end(); i++)
	{
		(*i)->Shutdown();
	}
}
static tTVPAtExit TVPShutdownVideoOverlayAtExit
	(TVP_ATEXIT_PRI_PREPARE, TVPShutdownVideoOverlay);
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// tTJSNI_VideoOverlay
//---------------------------------------------------------------------------
tTJSNI_VideoOverlay::tTJSNI_VideoOverlay()
: EventQueue(this,&tTJSNI_VideoOverlay::WndProc)
{
	VideoOverlay = NULL;
	Rect.left = 0;
	Rect.top = 0;
	Rect.right = 320;
	Rect.bottom = 240;
	Visible = false;
//	OwnerWindow = NULL;
	LocalTempStorageHolder = NULL;

	EventQueue.Allocate();

	Layer1 = NULL;
	Layer2 = NULL;
	Mode = vomOverlay;
	Loop = false;
	IsPrepare = false;
	SegLoopStartFrame = -1;
	SegLoopEndFrame = -1;
	IsEventPast = false;
	EventFrame = -1;

	Bitmap[0] = Bitmap[1] = NULL;
	BmpBits[0] = BmpBits[1] = NULL;
}
//---------------------------------------------------------------------------
tjs_error TJS_INTF_METHOD
tTJSNI_VideoOverlay::Construct(tjs_int numparams, tTJSVariant **param,
		iTJSDispatch2 *tjs_obj)
{
	tjs_error hr = inherited::Construct(numparams, param, tjs_obj);
	if(TJS_FAILED(hr)) return hr;

	return TJS_S_OK;
}
//---------------------------------------------------------------------------
void TJS_INTF_METHOD tTJSNI_VideoOverlay::Invalidate()
{
	inherited::Invalidate();

	Close();
	if (CachedOverlay) {
		CachedOverlay->Release();
		CachedOverlay = nullptr;
	}
	EventQueue.Deallocate();
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Open(const ttstr &_name)
{
	// open

	// first, close
	Close();


	// check window
	if(!Window) TVPThrowExceptionMessage(TVPWindowAlreadyMissing);

	// open target storage
	ttstr name(_name);
	ttstr param;

	const tjs_char * param_pos;
	int param_pos_ind;
	param_pos = TJS_strchr(name.c_str(), TJS_W('?'));
	param_pos_ind = (int)(param_pos - name.c_str());
	if(param_pos != NULL)
	{
		param = param_pos;
		name = ttstr(name, param_pos_ind);
	}

	IStream *istream = NULL;
	long size;
	ttstr ext = TVPExtractStorageExt(name).c_str();
	ext.ToLowerCase();

	{
		// prepate IStream
		tTJSBinaryStream *stream0 = NULL;
		try
		{
			stream0 = TVPCreateStream(name);
			size = (long)stream0->GetSize();
		}
		catch(...)
		{
			if(stream0) delete stream0;
			throw;
		}

		istream = TVPCreateIStream(stream0);
	}

	// 'istream' is an IStream instance at this point

	// create video overlay object
	try
	{
		if (CachedOverlay && CachedOverlayMode == Mode && CachedPlayingFile == _name) {
			VideoOverlay = CachedOverlay;
			CachedOverlay = nullptr;
			VideoOverlay->Rewind();
		} else {
			if (CachedOverlay) {
				CachedOverlay->Release();
				CachedOverlay = nullptr;
			}
			if (Mode == vomLayer)
				GetVideoLayerObject(EventQueue.GetOwner(), istream, name.c_str(), ext.c_str(), size, &VideoOverlay);
			else if(Mode == vomMixer)
				GetMixingVideoOverlayObject(EventQueue.GetOwner(), istream, name.c_str(), ext.c_str(), size, &VideoOverlay);
			else if(Mode == vomMFEVR)
				GetMFVideoOverlayObject(EventQueue.GetOwner(), istream, name.c_str(), ext.c_str(), size, &VideoOverlay);
			else
				GetVideoOverlayObject(EventQueue.GetOwner(), istream, name.c_str(), ext.c_str(), size, &VideoOverlay);
		}
		if( (Mode == vomOverlay) || (Mode == vomMixer) || (Mode == vomMFEVR) )
		{
			ResetOverlayParams();
		}
		else
		{	// set font and back buffer to layerVideo
			long	width, height;
			long			size;
			VideoOverlay->GetVideoSize( &width, &height );
			
			if( width <= 0 || height <= 0 )
				TVPThrowExceptionMessage(TVPErrorInKrMovieDLL, (const tjs_char*)TVPInvalidVideoSize);

			size = width * height * 4;
			if( Bitmap[0] != NULL )
				delete Bitmap[0];
			if( Bitmap[1] != NULL )
				delete Bitmap[1];
			Bitmap[0] = new tTVPBaseTexture(width, height, 32);
			Bitmap[1] = new tTVPBaseTexture(width, height, 32);
#if 0
			BmpBits[0] = static_cast<BYTE*>(Bitmap[0]->GetBitmap()->GetScanLine( Bitmap[0]->GetBitmap()->GetHeight()-1 ));
			BmpBits[1] = static_cast<BYTE*>(Bitmap[1]->GetBitmap()->GetScanLine( Bitmap[1]->GetBitmap()->GetHeight()-1 ));
#endif
			VideoOverlay->SetVideoBuffer(Bitmap[0], Bitmap[1], size);
		}
	}
	catch(...)
	{
		if(istream) istream->Release();
		Close();
		throw;
	}
	if(istream) istream->Release();

	// set Status
	ClearWndProcMessages();
	SetStatus(ssStop);
	CachedPlayingFile = _name;
	CachedOverlayMode = Mode;
	if (Loop) VideoOverlay->SetLoopSegement(0, -1);
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Close()
{
	// close
	// release VideoOverlay object
	if(VideoOverlay)
	{
		if (CachedOverlay) {
			CachedOverlay->Release();
			CachedOverlay = nullptr;
		}
		VideoOverlay->SetVisible(false);
		VideoOverlay->Pause();
		CachedOverlay = VideoOverlay;
		VideoOverlay = NULL;
//		::SetFocus(Window->GetWindowHandle());
	}
	if(LocalTempStorageHolder)
		delete LocalTempStorageHolder, LocalTempStorageHolder = NULL;
	ClearWndProcMessages();
	SetStatus(ssUnload);

	if( Bitmap[0] )
		delete Bitmap[0];
	if( Bitmap[1] )
		delete Bitmap[1];

	Bitmap[0] = Bitmap[1] = NULL;
	BmpBits[0] = BmpBits[1] = NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Shutdown()
{
	// shutdown the system
	// this functions closes the overlay object, but must not fire any events.
	bool c = CanDeliverEvents;
	ClearWndProcMessages();
	SetStatus(ssUnload);
	try
	{
		if (VideoOverlay) {
			if (CachedOverlay) {
				CachedOverlay->Release();
				CachedOverlay = nullptr;
			}
			VideoOverlay->SetVisible(false);
			VideoOverlay->Pause();
			CachedOverlay = VideoOverlay;
			VideoOverlay = NULL;
		}
	}
	catch(...)
	{
		CanDeliverEvents = c;
		throw;
	}
	CanDeliverEvents = c;
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Disconnect()
{
	// disconnect the object
	Shutdown();

	Window = NULL;
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Play()
{
	// start playing
	if(VideoOverlay)
	{
		VideoOverlay->Play();
		ClearWndProcMessages();
		if( Mode != vomMFEVR ) SetStatus(ssPlay);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Stop()
{
	// stop playing
	if(VideoOverlay)
	{
		VideoOverlay->Stop();
		ClearWndProcMessages();
		if( Mode != vomMFEVR ) SetStatus(ssStop);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::Pause()
{
	// pause playing
	if(VideoOverlay)
	{
		VideoOverlay->Pause();
//		ClearWndProcMessages();
		if( Mode != vomMFEVR ) SetStatus(ssPause);
	}
}
void tTJSNI_VideoOverlay::Rewind()
{
	// rewind playing
	if(VideoOverlay)
	{
		VideoOverlay->Rewind();
		if (Status == ssPlay)
			VideoOverlay->Play();
		ClearWndProcMessages();

		if( EventFrame >= 0 && IsEventPast )
			IsEventPast = false;
	}
}
void tTJSNI_VideoOverlay::Prepare()
{	// prepare movie
	if( VideoOverlay && (Mode == vomLayer) )
	{
		Pause();
		Rewind();
		IsPrepare = true;
		Play();
	}
}
void tTJSNI_VideoOverlay::SetSegmentLoop( int comeFrame, int goFrame )
{
	SegLoopStartFrame = comeFrame;
	SegLoopEndFrame = goFrame;
	if (VideoOverlay) VideoOverlay->SetLoopSegement(SegLoopStartFrame, SegLoopEndFrame);
}
void tTJSNI_VideoOverlay::SetPeriodEvent( int eventFrame )
{
	EventFrame = eventFrame;

	if( eventFrame <= GetFrame() )
		IsEventPast = true;
	else
		IsEventPast = false;
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetRectangleToVideoOverlay()
{
	// set Rectangle to video overlay
	if(VideoOverlay /*&& OwnerWindow*/)
	{
		tjs_int ofsx, ofsy;
		Window->GetVideoOffset(ofsx, ofsy);
		tjs_int l = Rect.left;
		tjs_int t = Rect.top;
		tjs_int r = Rect.right;
		tjs_int b = Rect.bottom;
		TVPAddLog(TJS_W("Video zoom: (") + ttstr(l) + TJS_W(",") + ttstr(t) + TJS_W(")-(") +
			ttstr(r) + TJS_W(",") + ttstr(b) + TJS_W(") ->"));
		Window->ZoomRectangle(l, t, r, b);
		TVPAddLog(TJS_W("(") + ttstr(l) + TJS_W(",") + ttstr(t) + TJS_W(")-(") +
			ttstr(r) + TJS_W(",") + ttstr(b) + TJS_W(")"));
	//	RECT rect = {l + ofsx, t + ofsy, r + ofsx, b + ofsy};
		VideoOverlay->SetRect(l + ofsx, t + ofsy, r + ofsx, b + ofsy);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetPosition(tjs_int left, tjs_int top)
{
	if( Mode == vomLayer )
	{
		if( Layer1 != NULL ) Layer1->SetPosition( left, top );
		if( Layer2 != NULL ) Layer2->SetPosition( left, top );
	}
	else
	{
		Rect.set_offsets(left, top);
		SetRectangleToVideoOverlay();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetSize(tjs_int width, tjs_int height)
{
	if( Mode == vomLayer ) return;

	Rect.set_size(width, height);
	SetRectangleToVideoOverlay();
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetBounds(const tTVPRect & rect)
{
	if( Mode == vomLayer ) return;

	Rect = rect;
	SetRectangleToVideoOverlay();
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetLeft(tjs_int l)
{
	if( Mode == vomLayer )
	{
		if( Layer1 != NULL ) Layer1->SetLeft( l );
		if( Layer2 != NULL ) Layer2->SetLeft( l );
	}
	else
	{
		Rect.set_offsets(l, Rect.top);
		SetRectangleToVideoOverlay();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetTop(tjs_int t)
{
	if( Mode == vomLayer )
	{
		if( Layer1 != NULL ) Layer1->SetTop( t );
		if( Layer2 != NULL ) Layer2->SetTop( t );
	}
	else
	{
		Rect.set_offsets(Rect.left, t);
		SetRectangleToVideoOverlay();
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetWidth(tjs_int w)
{
	if( Mode == vomLayer ) return;

	Rect.right = Rect.left + w;
	SetRectangleToVideoOverlay();
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetHeight(tjs_int h)
{
	if( Mode == vomLayer ) return;

	Rect.bottom = Rect.top + h;
	SetRectangleToVideoOverlay();
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetVisible(bool b)
{
	Visible = b;
	if(VideoOverlay)
	{
		if( Mode == vomLayer )
		{
			if( Layer1 != NULL ) Layer1->SetVisible( Visible );
			if( Layer2 != NULL ) Layer2->SetVisible( Visible );
		}
		else
		{
			VideoOverlay->SetVisible(Visible);
		}
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::ResetOverlayParams()
{
	// retrieve new window information from owner window and
	// set video owner window / message drain window.
	// also sets rectangle and visible state.
	if(VideoOverlay && Window && (Mode == vomOverlay || Mode == vomMixer || Mode == vomMFEVR) )
	{
//		OwnerWindow = Window->GetWindowHandle();
		VideoOverlay->SetWindow(Window);

//		VideoOverlay->SetMessageDrainWindow(Window->GetSurfaceWindowHandle());

		// set Rectangle
		SetRectangleToVideoOverlay();

		// set Visible
		VideoOverlay->SetVisible(Visible);
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::DetachVideoOverlay()
{
	if(VideoOverlay && Window && (Mode == vomOverlay || Mode == vomMixer || Mode == vomMFEVR) )
	{
		VideoOverlay->SetWindow(NULL);
		VideoOverlay->SetMessageDrainWindow(EventQueue.GetOwner());
			// once set to util window
	}
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetRectOffset(tjs_int ofsx, tjs_int ofsy)
{
	if(VideoOverlay)
	{
// 		RECT r = {Rect.left + ofsx, Rect.top + ofsy,
// 			Rect.right + ofsx, Rect.bottom + ofsy};
		VideoOverlay->SetRect(Rect.left + ofsx, Rect.top + ofsy,
			Rect.right + ofsx, Rect.bottom + ofsy);
	}
}
//---------------------------------------------------------------------------
//void __fastcall tTJSNI_VideoOverlay::WndProc(Messages::TMessage &Msg)
void tTJSNI_VideoOverlay::WndProc( NativeEvent& ev )
{
	// EventQueue's message procedure
	if(VideoOverlay)
	{
		switch(ev.Message) {
		case WM_GRAPHNOTIFY:
		{
			long evcode;
//			LONG_PTR p1, p2;
			bool got = false;
			do {
#if 0
				VideoOverlay->GetEvent(&evcode, &p1, &p2, &got);
				if( got == false)
					return;
#endif
				evcode = ev.WParam;
				switch( evcode )
				{
					case EC_COMPLETE:
						if( Status == ssPlay )
						{
							if( Loop )
							{
								Rewind();
								FirePeriodEvent(perLoop); // fire period event by loop rewind
							}
							else
							{
								// Graph manager seems not to complete playing
								// at this point (rewinding the movie at the event
								// handler called asynchronously from SetStatusAsync
								// makes continuing playing, but the graph seems to
								// be unstable).
								// We manually stop the manager anyway.
								VideoOverlay->Stop();
								SetStatusAsync(ssStop); // All data has been rendered
							}
						}
						break;
					case EC_UPDATE:
						if( Mode == vomLayer && Status == ssPlay )
						{
							int		curFrame = (int)ev.LParam;
							if( Layer1 == NULL && Layer2 == NULL )	// nothing to do.
								return;

							// 2フレーム以上差があるときはGetFrame() を現在のフレームとする
							int frame = GetFrame();
							if( (frame+1) < curFrame || (frame-1) > curFrame )
								curFrame = frame;

							if( (!IsPrepare) && (SegLoopEndFrame > 0) && (frame >= SegLoopEndFrame) ) {
								SetFrame( SegLoopStartFrame > 0 ? SegLoopStartFrame : 0 );
								FirePeriodEvent(perSegLoop); // fire period event by segment loop rewind
								return; // Updateを行わない
							}

							// get video image size
							long	width, height;
							VideoOverlay->GetVideoSize( &width, &height );

							tTJSNI_BaseLayer	*l1 = Layer1;
							tTJSNI_BaseLayer	*l2 = Layer2;

							// Check layer image size
							if( l1 != NULL )
							{
								if( (long)l1->GetImageWidth() != width || (long)l1->GetImageHeight() != height )
									l1->SetImageSize( width, height );
								if( (long)l1->GetWidth() != width || (long)l1->GetHeight() != height )
									l1->SetSize( width, height );
							}
							if( l2 != NULL )
							{
								if( (long)l2->GetImageWidth() != width || (long)l2->GetImageHeight() != height )
									l2->SetImageSize( width, height );
								if( (long)l2->GetWidth() != width || (long)l2->GetHeight() != height )
									l2->SetSize( width, height );
							}
							tTVPBaseTexture *buff = VideoOverlay->GetFrontBuffer(  );
							if( buff == Bitmap[0] )
							{
								if( l1 ) l1->AssignMainImage( Bitmap[0] );
								if( l2 ) l2->AssignMainImage( Bitmap[0] );
							}
							else	// 0じゃなかったら、1とみなす。
							{
								if( l1 ) l1->AssignMainImage( Bitmap[1] );
								if( l2 ) l2->AssignMainImage( Bitmap[1] );
							}
							if( l1 ) l1->Update();
							if( l2 ) l2->Update();
							FireFrameUpdateEvent( curFrame );

							// ! Prepare mode ?
							if( !IsPrepare )
							{
								// Send period event ?
								if( EventFrame >= 0 && !IsEventPast && curFrame >= EventFrame )
								{
									EventFrame = -1;
									FirePeriodEvent(perPeriod); // fire period event by setPeriodEvent()
								}
							}
							else
							{	// Prepare mode
								FirePeriodEvent(perPrepare); // fire period event by prepare()
								Pause();
								Rewind();
								IsPrepare = false;
							}
						}
						else if( Mode == vomMixer && Status == ssPlay )
						{
							int frame = GetFrame();
							if( (!IsPrepare) && (SegLoopEndFrame > 0) && (frame >= SegLoopEndFrame) ) {
								SetFrame( SegLoopStartFrame > 0 ? SegLoopStartFrame : 0 );
								FirePeriodEvent(perSegLoop); // fire period event by segment loop rewind
								return;
							}
							VideoOverlay->PresentVideoImage();
							FireFrameUpdateEvent( frame );
							// Send period event ?
							if( EventFrame >= 0 && !IsEventPast && frame >= EventFrame )
							{
								EventFrame = -1;
								FirePeriodEvent(perPeriod); // fire period event by setPeriodEvent()
							}
						}
						break;
				}
//				VideoOverlay->FreeEventParams( evcode, p1, p2 );
			} while( got );
			return;
		}
		case WM_CALLBACKCMD:
		{
			// wparam : command
			// lparam : argument
			FireCallbackCommand((tjs_char*)ev.WParam, (tjs_char*)ev.LParam);
			return;
		}
		case WM_STATE_CHANGE:
			{
				switch( ev.WParam ) {
				case vsStopped:
					SetStatusAsync( ssStop );
					break;
				case vsPlaying:
					SetStatusAsync( ssPlay );
					break;
				case vsPaused:
					SetStatusAsync( ssPause );
					break;
				case vsReady:
					SetStatusAsync( ssReady );
					break;
				case vsEnded:
					if( Status == ssPlay )
					{
						if( Loop )
						{
							VideoOverlay->Play();
							FirePeriodEvent(perLoop); // fire period event by loop rewind
						}
						else
						{
							VideoOverlay->Stop();
							SetStatusAsync(ssStop); // All data has been rendered
						}
					}
					break;
				}
				return;
			}
		}
	}

	EventQueue.HandlerDefault(ev);
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::SetTimePosition( tjs_uint64 p )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetPosition( p );
	}
}
tjs_uint64 tTJSNI_VideoOverlay::GetTimePosition()
{
	tjs_uint64	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetPosition( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SetFrame( tjs_int f )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetFrame( f );

		if( EventFrame >= f && IsEventPast )
			IsEventPast = false;
	}
}
tjs_int tTJSNI_VideoOverlay::GetFrame()
{
	tjs_int	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetFrame( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SetStopFrame( tjs_int f )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetStopFrame( f );
	}
}
void tTJSNI_VideoOverlay::SetDefaultStopFrame()
{
	if(VideoOverlay)
	{
		VideoOverlay->SetDefaultStopFrame();
	}
}
tjs_int tTJSNI_VideoOverlay::GetStopFrame()
{
	tjs_int	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetStopFrame( &result );
	}
	return result;
}
tjs_real tTJSNI_VideoOverlay::GetFPS()
{
	tjs_real	result = 0.0;
	if(VideoOverlay)
	{
		VideoOverlay->GetFPS( &result );
	}
	return result;
}
tjs_int tTJSNI_VideoOverlay::GetNumberOfFrame()
{
	tjs_int	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetNumberOfFrame( &result );
	}
	return result;
}
tjs_int64 tTJSNI_VideoOverlay::GetTotalTime()
{
	tjs_int64	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetTotalTime( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SetLoop( bool b )
{
	Loop = b;
	if (VideoOverlay) {
		if (Loop)
			VideoOverlay->SetLoopSegement(0, -1);
		else
			VideoOverlay->SetLoopSegement(-1, -1);
	}
}
void tTJSNI_VideoOverlay::SetLayer1( tTJSNI_BaseLayer *l )
{
	Layer1 = l;
}
void tTJSNI_VideoOverlay::SetLayer2( tTJSNI_BaseLayer *l )
{
	Layer2 = l;
}
void tTJSNI_VideoOverlay::SetMode( tTVPVideoOverlayMode m )
{
	// ビデオオープン後のモード変更は禁止
	if( !VideoOverlay )
	{
		Mode = m;
	}
}

tjs_real tTJSNI_VideoOverlay::GetPlayRate()
{
	tjs_real	result = 0.0;
	if(VideoOverlay)
	{
		VideoOverlay->GetPlayRate( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SetPlayRate(tjs_real r)
{
	if(VideoOverlay)
	{
		VideoOverlay->SetPlayRate( r );
	}
}

tjs_int tTJSNI_VideoOverlay::GetAudioBalance()
{
	long	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetAudioBalance( &result );
	}
	return /*TVPDSAttenuateToPan*/( result );
}
void tTJSNI_VideoOverlay::SetAudioBalance(tjs_int b)
{
	if(VideoOverlay)
	{
		VideoOverlay->SetAudioBalance( /*TVPPanToDSAttenuate*/( b ) );
	}
}
tjs_int tTJSNI_VideoOverlay::GetAudioVolume()
{
	long	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetAudioVolume( &result );
	}
	return /*TVPDSAttenuateToVolume*/( result );
}
void tTJSNI_VideoOverlay::SetAudioVolume(tjs_int b)
{
	if(VideoOverlay)
	{
		VideoOverlay->SetAudioVolume( /*TVPVolumeToDSAttenuate*/( b ) );
	}
}
tjs_uint tTJSNI_VideoOverlay::GetNumberOfAudioStream()
{
	unsigned long	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetNumberOfAudioStream( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SelectAudioStream(tjs_uint n)
{
	if(VideoOverlay)
	{
		VideoOverlay->SelectAudioStream( n );
	}
}
tjs_int tTJSNI_VideoOverlay::GetEnabledAudioStream()
{
	long		result = -1;
	if(VideoOverlay)
	{
		VideoOverlay->GetEnableAudioStreamNum( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::DisableAudioStream()
{
	if(VideoOverlay)
	{
		VideoOverlay->DisableAudioStream();
	}
}

tjs_uint tTJSNI_VideoOverlay::GetNumberOfVideoStream()
{
	unsigned long	result = 0;
	if(VideoOverlay)
	{
		VideoOverlay->GetNumberOfVideoStream( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SelectVideoStream(tjs_uint n)
{
	if(VideoOverlay)
	{
		VideoOverlay->SelectVideoStream( n );
	}
}
tjs_int tTJSNI_VideoOverlay::GetEnabledVideoStream()
{
	long		result = -1;
	if(VideoOverlay)
	{
		VideoOverlay->GetEnableVideoStreamNum( &result );
	}
	return result;
}
void tTJSNI_VideoOverlay::SetMixingLayer( tTJSNI_BaseLayer *l )
{
	if(VideoOverlay)
	{
		if( l )
		{
			if( l->GetVisible() )
			{
				float	alpha = static_cast<float>(l->GetOpacity()) / 255.0f;
#if 0
				RECT	dest;
				dest.left = l->GetLeft() + l->GetImageLeft();
				dest.top = l->GetTop() + l->GetImageTop();
				dest.right = dest.left + l->GetImageWidth();
				dest.bottom = dest.top + l->GetImageHeight();

				// tTVPBaseBitmap->tTVPBitmap
				tTVPBitmap *bmp = l->GetMainImage()->GetBitmap();
				if( bmp )
				{
					// 自前でDCを作る
					HDC hdc;
					HDC			ref = GetDC(0);
					HBITMAP		myDIB = CreateDIBitmap( ref, bmp->GetBITMAPINFOHEADER(), CBM_INIT, bmp->GetBits(), bmp->GetBITMAPINFO(), bmp->Is8bit() ? DIB_PAL_COLORS : DIB_RGB_COLORS );
					hdc = CreateCompatibleDC( NULL );
					HGDIOBJ		hOldBmp = SelectObject( hdc, myDIB );

					VideoOverlay->SetMixingBitmap( hdc, &dest, alpha );

					SelectObject( hdc, hOldBmp );
					DeleteObject( myDIB );
					DeleteDC( hdc );
				}
#endif
				VideoOverlay->SetMixingBitmap(l->GetMainImage(), alpha);
			}
			else
			{
				VideoOverlay->ResetMixingBitmap();
			}
		}
		else
		{
			VideoOverlay->ResetMixingBitmap();
		}
	}
}
void tTJSNI_VideoOverlay::ResetMixingBitmap()
{
	if(VideoOverlay)
	{
		VideoOverlay->ResetMixingBitmap();
	}
}
void tTJSNI_VideoOverlay::SetMixingMovieAlpha( tjs_real a )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetMixingMovieAlpha( static_cast<float>(a) );
	}
}
tjs_real tTJSNI_VideoOverlay::GetMixingMovieAlpha()
{
	float	ret = 0.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetMixingMovieAlpha( &ret );
	}
	return static_cast<tjs_real>(ret);
}
void tTJSNI_VideoOverlay::SetMixingMovieBGColor( tjs_uint col )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetMixingMovieBGColor( col );
	}
}
tjs_uint tTJSNI_VideoOverlay::GetMixingMovieBGColor()
{
	unsigned long	ret;
	if(VideoOverlay)
	{
		VideoOverlay->GetMixingMovieBGColor( &ret );
	}
	return static_cast<tjs_uint>(ret);
}



tjs_real tTJSNI_VideoOverlay::GetContrastRangeMin()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetContrastRangeMin( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetContrastRangeMax()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetContrastRangeMax( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetContrastDefaultValue()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetContrastDefaultValue( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetContrastStepSize()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetContrastStepSize( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetContrast()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetContrast( &ret );
	}
	return static_cast<tjs_real>(ret);
}
void tTJSNI_VideoOverlay::SetContrast( tjs_real v )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetContrast( static_cast<float>(v) );
	}
}
tjs_real tTJSNI_VideoOverlay::GetBrightnessRangeMin()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetBrightnessRangeMin( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetBrightnessRangeMax()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetBrightnessRangeMax( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetBrightnessDefaultValue()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetBrightnessDefaultValue( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetBrightnessStepSize()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetBrightnessStepSize( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetBrightness()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetBrightness( &ret );
	}
	return static_cast<tjs_real>(ret);
}
void tTJSNI_VideoOverlay::SetBrightness( tjs_real v )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetBrightness( static_cast<float>(v) );
	}
}

tjs_real tTJSNI_VideoOverlay::GetHueRangeMin()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetHueRangeMin( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetHueRangeMax()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetHueRangeMax( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetHueDefaultValue()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetHueDefaultValue( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetHueStepSize()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetHueStepSize( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetHue()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetHue( &ret );
	}
	return static_cast<tjs_real>(ret);
}
void tTJSNI_VideoOverlay::SetHue( tjs_real v )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetHue( static_cast<float>(v) );
	}
}

tjs_real tTJSNI_VideoOverlay::GetSaturationRangeMin()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetSaturationRangeMin( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetSaturationRangeMax()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetSaturationRangeMax( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetSaturationDefaultValue()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetSaturationDefaultValue( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetSaturationStepSize()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetSaturationStepSize( &ret );
	}
	return static_cast<tjs_real>(ret);
}
tjs_real tTJSNI_VideoOverlay::GetSaturation()
{
	float ret = -1.0f;
	if(VideoOverlay)
	{
		VideoOverlay->GetSaturation( &ret );
	}
	return static_cast<tjs_real>(ret);
}
void tTJSNI_VideoOverlay::SetSaturation( tjs_real v )
{
	if(VideoOverlay)
	{
		VideoOverlay->SetSaturation( static_cast<float>(v) );
	}
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_VideoOverlay::GetOriginalWidth()
{
	// retrieve original (coded in the video stream) width size
	if(!VideoOverlay) return 0;

	long	width, height;
	VideoOverlay->GetVideoSize( &width, &height );

	return (tjs_int)width;
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_VideoOverlay::GetOriginalHeight()
{
	// retrieve original (coded in the video stream) height size

	long	width, height;
	VideoOverlay->GetVideoSize( &width, &height );

	return (tjs_int)height;
}
//---------------------------------------------------------------------------
void tTJSNI_VideoOverlay::ClearWndProcMessages()
{
	// clear WndProc's message queue
	EventQueue.Clear(WM_GRAPHNOTIFY);
	EventQueue.Clear(WM_CALLBACKCMD);
#if 0 // only valuable for DirectShow
	MSG msg;
	while(PeekMessage(&msg, EventQueue.GetOwner(), WM_GRAPHNOTIFY, WM_GRAPHNOTIFY+2, PM_REMOVE))
	{
		if(VideoOverlay)
		{
			long evcode;
			LONG_PTR p1, p2;
			bool got;
			VideoOverlay->GetEvent(&evcode, &p1, &p2, &got); // dummy call
			if( got )
				VideoOverlay->FreeEventParams( evcode, p1, p2 );
		}
	}
#endif
}

bool tTJSNI_VideoOverlay::GetVideoSize(tjs_int &w, tjs_int &h) const  {
	if (VideoOverlay) {
		long _w, _h;
		VideoOverlay->GetVideoSize(&_w, &_h);
		w = _w; h = _h;
		return true;
	}
	return false;
}

//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTJSNC_VideoOverlay::CreateNativeInstance : returns proper instance object
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_VideoOverlay::CreateNativeInstance()
{
	return new tTJSNI_VideoOverlay();
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// TVPCreateNativeClass_VideoOverlay
//---------------------------------------------------------------------------
tTJSNativeClass * TVPCreateNativeClass_VideoOverlay()
{
	return new tTJSNC_VideoOverlay();
}
//---------------------------------------------------------------------------

