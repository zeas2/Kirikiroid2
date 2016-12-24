//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// krmovie.dll ( kirikiri movie playback support DLL ) interface
//---------------------------------------------------------------------------


#ifndef __KRMOVIE_H__
#define __KRMOVIE_H__

//#include "typedefine.h"

//---------------------------------------------------------------------------
enum tTVPVideoStatus { vsStopped, vsPlaying, vsPaused, vsProcessing, vsEnded, vsReady };
//---------------------------------------------------------------------------
#define __stdcall
class tTVPBaseTexture;
//---------------------------------------------------------------------------
// iTVPVideoOverlay
//---------------------------------------------------------------------------
class iTVPVideoOverlay // this is not a COM object
{
public:
	virtual void __stdcall AddRef() = 0;
	virtual void __stdcall Release() = 0;

	virtual void __stdcall SetWindow(class tTJSNI_Window* window) = 0;
	virtual void __stdcall SetMessageDrainWindow(void* window) = 0;
	virtual void __stdcall SetRect(int l, int t, int r, int b) = 0;
	virtual void __stdcall SetVisible(bool b) = 0;
	virtual void __stdcall Play() = 0;
	virtual void __stdcall Stop() = 0;
	virtual void __stdcall Pause() = 0;
	virtual void __stdcall SetPosition(uint64_t tick) = 0;
	virtual void __stdcall GetPosition(uint64_t *tick) = 0;
	virtual void __stdcall GetStatus(tTVPVideoStatus *status) = 0;
// 	virtual void __stdcall GetEvent(long *evcode, LONG_PTR *param1,
// 			LONG_PTR *param2, bool *got) = 0;

//	virtual void __stdcall FreeEventParams(long evcode, LONG_PTR param1, LONG_PTR param2) = 0;

	virtual void __stdcall Rewind() = 0;
	virtual void __stdcall SetFrame( int f ) = 0;
	virtual void __stdcall GetFrame( int *f ) = 0;
	virtual void __stdcall GetFPS( double *f ) = 0;
	virtual void __stdcall GetNumberOfFrame( int *f ) = 0;
	virtual void __stdcall GetTotalTime(int64_t *t) = 0;
	
	virtual void __stdcall GetVideoSize( long *width, long *height ) = 0;
	virtual tTVPBaseTexture* GetFrontBuffer() = 0;
	virtual void __stdcall SetVideoBuffer(tTVPBaseTexture *buff1, tTVPBaseTexture *buff2, long size) = 0;

	virtual void __stdcall SetStopFrame( int frame ) = 0;
	virtual void __stdcall GetStopFrame( int *frame ) = 0;
	virtual void __stdcall SetDefaultStopFrame() = 0;

	virtual void __stdcall SetPlayRate( double rate ) = 0;
	virtual void __stdcall GetPlayRate( double *rate ) = 0;

	virtual void __stdcall SetAudioBalance( long balance ) = 0;
	virtual void __stdcall GetAudioBalance( long *balance ) = 0;
	virtual void __stdcall SetAudioVolume( long volume ) = 0;
	virtual void __stdcall GetAudioVolume( long *volume ) = 0;

	virtual void __stdcall GetNumberOfAudioStream( unsigned long *streamCount ) = 0;
	virtual void __stdcall SelectAudioStream( unsigned long num ) = 0;
	virtual void __stdcall GetEnableAudioStreamNum( long *num ) = 0;
	virtual void __stdcall DisableAudioStream( void ) = 0;

	virtual void __stdcall GetNumberOfVideoStream( unsigned long *streamCount ) = 0;
	virtual void __stdcall SelectVideoStream( unsigned long num ) = 0;
	virtual void __stdcall GetEnableVideoStreamNum( long *num ) = 0;

	virtual void __stdcall SetMixingBitmap(class tTVPBaseTexture *dest, float alpha) = 0;
	virtual void __stdcall ResetMixingBitmap() = 0;

	virtual void __stdcall SetMixingMovieAlpha( float a ) = 0;
	virtual void __stdcall GetMixingMovieAlpha( float *a ) = 0;
	virtual void __stdcall SetMixingMovieBGColor( unsigned long col ) = 0;
	virtual void __stdcall GetMixingMovieBGColor( unsigned long *col ) = 0;

	virtual void __stdcall PresentVideoImage() = 0;

	virtual void __stdcall GetContrastRangeMin( float *v ) = 0;
	virtual void __stdcall GetContrastRangeMax( float *v ) = 0;
	virtual void __stdcall GetContrastDefaultValue( float *v ) = 0;
	virtual void __stdcall GetContrastStepSize( float *v ) = 0;
	virtual void __stdcall GetContrast( float *v ) = 0;
	virtual void __stdcall SetContrast( float v ) = 0;

	virtual void __stdcall GetBrightnessRangeMin( float *v ) = 0;
	virtual void __stdcall GetBrightnessRangeMax( float *v ) = 0;
	virtual void __stdcall GetBrightnessDefaultValue( float *v ) = 0;
	virtual void __stdcall GetBrightnessStepSize( float *v ) = 0;
	virtual void __stdcall GetBrightness( float *v ) = 0;
	virtual void __stdcall SetBrightness( float v ) = 0;

	virtual void __stdcall GetHueRangeMin( float *v ) = 0;
	virtual void __stdcall GetHueRangeMax( float *v ) = 0;
	virtual void __stdcall GetHueDefaultValue( float *v ) = 0;
	virtual void __stdcall GetHueStepSize( float *v ) = 0;
	virtual void __stdcall GetHue( float *v ) = 0;
	virtual void __stdcall SetHue( float v ) = 0;

	virtual void __stdcall GetSaturationRangeMin( float *v ) = 0;
	virtual void __stdcall GetSaturationRangeMax( float *v ) = 0;
	virtual void __stdcall GetSaturationDefaultValue( float *v ) = 0;
	virtual void __stdcall GetSaturationStepSize( float *v ) = 0;
	virtual void __stdcall GetSaturation( float *v ) = 0;
	virtual void __stdcall SetSaturation( float v ) = 0;

	// extended features
	virtual void SetLoopSegement(int beginFrame, int endFrame) = 0;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#define WM_GRAPHNOTIFY  (/*WM_USER+*/15)
#define WM_CALLBACKCMD  (/*WM_USER+*/16)
#define EC_UPDATE		(/*EC_USER*/0x8000+1)
#define WM_STATE_CHANGE	(/*WM_USER+*/18)

#ifndef EC_COMPLETE
#define EC_COMPLETE                         0x01
#endif
//---------------------------------------------------------------------------

#undef __stdcall
#endif


