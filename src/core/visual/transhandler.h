//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Transition handler interface
//---------------------------------------------------------------------------

#ifndef __TRANSHANDLER_H__
#define __TRANSHANDLER_H__

#include "tjsTypes.h"
#include "tjsErrorDefs.h"
#include "drawable.h"

/*[*/
//---------------------------------------------------------------------------
// tTVPTransType
//---------------------------------------------------------------------------
// transition type
#ifdef __BORLANDC__
	#pragma option push -b
#endif
enum tTVPTransType
{
	ttSimple, // transition using only one(self) layer ( eg. simple fading )
	ttExchange // transition using two layer ( eg. cross fading )
};
#ifdef __BORLANDC__
	#pragma option pop
#endif
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTVPTransUpdateType
//---------------------------------------------------------------------------
// transition update type
#ifdef __BORLANDC__
	#pragma option push -b
#endif
enum tTVPTransUpdateType
{
	tutDivisibleFade,
	tutDivisible,
	tutGiveUpdate
};
#ifdef __BORLANDC__
	#pragma option pop
#endif
/*
	there are two types of transition update method;
	tutDivisibleFade, tutDivisible and tutGiveUpdate.

	tutDivisibleFade
		used when the transition processing is region-divisible and
		the transition updates entire area of the layer.
		update area is always given by iTVPTransHandler::Process caller.
		handler must use only given area of the source bitmap on each
		callbacking.

	tutDivisible
		same as tutDivisibleFade, except for its usage of source area.
		handler can use any area of the source bitmap.
		this will somewhat slower than tutDivisibleFade.

	tutGiveUpdate
		used when the transition processing is not region-divisible or
		the transition updates only some small regions rather than entire
		area.
		update area is given by callee of iTVPTransHandler::Process, 
		via iTVPLayerUpdater interface.
*/
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPScanLineProvider
//---------------------------------------------------------------------------
// provides layer scanline
class iTVPTexture2D;
class iTVPScanLineProvider
{
public:
	virtual tjs_error TJS_INTF_METHOD AddRef() = 0;
	virtual tjs_error TJS_INTF_METHOD Release() = 0;
		// call "Release" when done with this object

	virtual tjs_error TJS_INTF_METHOD GetWidth(/*out*/tjs_int *width) = 0;
		// return image width
	virtual tjs_error TJS_INTF_METHOD GetHeight(/*out*/tjs_int *height) = 0;
		// return image height
#if 0
	virtual tjs_error TJS_INTF_METHOD GetPixelFormat(/*out*/tjs_int *bpp) = 0;
		// return image bit depth
	virtual tjs_error TJS_INTF_METHOD GetPitchBytes(/*out*/tjs_int *pitch) = 0;
		// return image bitmap data width in bytes ( offset to next down line )
	virtual tjs_error TJS_INTF_METHOD GetScanLine(/*in*/tjs_int line,
			/*out*/const void ** scanline) = 0;
		// return image pixel scan line pointer
	virtual tjs_error TJS_INTF_METHOD GetScanLineForWrite(/*in*/tjs_int line,
			/*out*/void ** scanline) = 0;
		// return image pixel scan line pointer for writing
#endif
	virtual iTVPTexture2D * GetTexture() = 0;
	virtual iTVPTexture2D * GetTextureForRender() = 0;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPSimpleOptionProvider
//---------------------------------------------------------------------------
// provides option set
class iTVPSimpleOptionProvider
{
public:
	virtual tjs_error TJS_INTF_METHOD AddRef() = 0;
	virtual tjs_error TJS_INTF_METHOD Release() = 0;
		// call this when done with this object

	virtual tjs_error TJS_INTF_METHOD GetAsNumber(
			/*in*/const tjs_char *name, /*out*/tjs_int64 *value) = 0;
		// retrieve option as a number.
	virtual tjs_error TJS_INTF_METHOD GetAsString(
			/*in*/const tjs_char *name, /*out*/const tjs_char **out) = 0;
		// retrieve option as a string.
		// note that you must use the returned string as an one time string
		// pointer; you cannot hold its pointer and/or use it later.

	virtual tjs_error TJS_INTF_METHOD GetValue(
			/*in*/const tjs_char *name, /*out*/tTJSVariant *dest) = 0;
		// retrieve option as a tTJSVariant.

	virtual tjs_error TJS_INTF_METHOD Reserved2() = 0;

	virtual tjs_error TJS_INTF_METHOD GetDispatchObject(iTJSDispatch2 **dsp)
		 = 0;
		// retrieve internal dispatch object ( if exists )
};
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// iTVPSimpleImageProvider
//---------------------------------------------------------------------------
// image loader
class iTVPSimpleImageProvider
{
public:
	virtual tjs_error TJS_INTF_METHOD LoadImage(
			/*in*/const tjs_char *name, /*in*/tjs_int bpp,
			/*in*/tjs_uint32 key, 
			/*in*/tjs_uint w,
			/*in*/tjs_uint h,
			/*out*/iTVPScanLineProvider ** scpro) = 0;
		// load an image.
		// returned image be an 8bpp bitmap when bpp == 8, otherwise
		// 32bpp.
		// key is a color key. pass 0x02ffffff for not to apply color key.
		// you must release "scpro" when you done with it.
		// w and h are desired size of the image. if the actual size is smaller
		// than these, the image is to be tiled. give 0, 0 to obtain original
		// sized image.
};
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------
// iTVPLayerUpdater
//---------------------------------------------------------------------------
// layer update region notification interface
class iTVPLayerUpdater
{
public:
	virtual tjs_error TJS_INTF_METHOD UpdateRect(tjs_int left,
		tjs_int top, tjs_int right, tjs_int bottom);
		// notify that the layer image had been changed.
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// tTVPDivisibleData
//---------------------------------------------------------------------------
// structure used by iTVPDivisibleTransHandler::Process
#ifdef _WIN32
#pragma pack(push, 4)
#endif

struct tTVPDivisibleData
{
	/*const*/tjs_int Left; // processing rectangle left
	/*const*/tjs_int Top; // processing rectangle top
	/*const*/tjs_int Width; // processing rectangle width
	/*const*/tjs_int Height; // processing rectangle height
	iTVPScanLineProvider *Dest; // destination image
	tjs_int DestLeft; // destination image rectangle's left
	tjs_int DestTop; // destination image rectangle's top
	/*const*/iTVPScanLineProvider *Src1; // source 1 (self layer image)
	/*const*/tjs_int Src1Left; // source 1 image rectangle's left
	/*const*/tjs_int Src1Top; // source 1 image rectangle's top
	/*const*/iTVPScanLineProvider *Src2; // source 2 (other layer image)
	/*const*/tjs_int Src2Left; // source 2 image rectangle's left
	/*const*/tjs_int Src2Top; // source 2 image rectangle's top
};
/* note that "Src2" will be null when transition type is ttSimple. */
/* Src1Left, Src1Top, Src2Left, Src2Top are not used when the transition is
	tutDivisible. */

#ifdef _WIN32
#pragma pack(pop)
#endif



//---------------------------------------------------------------------------
// iTVPBaseTransHandler
//---------------------------------------------------------------------------
class iTVPBaseTransHandler
{
public:
	virtual tjs_error TJS_INTF_METHOD AddRef() = 0;
	virtual tjs_error TJS_INTF_METHOD Release() = 0;

	virtual tjs_error TJS_INTF_METHOD SetOption(
			/*in*/iTVPSimpleOptionProvider *options // option provider
		) = 0;
		// Set option for current processing transition
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPDivisibleTransHandler
//---------------------------------------------------------------------------
class iTVPDivisibleTransHandler : public iTVPBaseTransHandler
{
public:
	virtual tjs_error TJS_INTF_METHOD StartProcess(
			/*in*/tjs_uint64 tick) = 0;
		// called before one processing time unit.
		// expected return values are:
		// TJS_S_TRUE: continue processing
		// TJS_S_FALSE: break processing

	virtual tjs_error TJS_INTF_METHOD EndProcess() = 0;
		// called after one processing time unit.
		// expected return values are:
		// TJS_S_TRUE: continue processing
		// TJS_S_FALSE: break processing

	virtual tjs_error TJS_INTF_METHOD Process(
			/*in,out*/tTVPDivisibleData *data) = 0;
		// called during StartProcess and EndProcess per an update rectangle.
		// the handler processes given rectangle and put result image to
		// "Dest"( in tTVPDivisibleData ).
		// given "Dest" is a internal image buffer, but callee can change
		// the "Dest" pointer to Src1 or Src2. Also DestLeft and DestTop can
		// be changed to point destination image part.

	virtual tjs_error TJS_INTF_METHOD MakeFinalImage(
			/*in,out*/iTVPScanLineProvider ** dest, // destination
			/*in*/iTVPScanLineProvider * src1, // source 1
			/*in*/iTVPScanLineProvider * src2 // source 2
			) = 0;
		// will be called after StartProcess/EndProcess returns TJS_S_FALSE.
		// this function does not called in some occasions.
		// fill "dest" to make a final image.
		// dest can be set to either src1 or src2.
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPGiveUpdateTransHandler
//---------------------------------------------------------------------------
class iTVPGiveUpdateTransHandler : public iTVPBaseTransHandler
{
public:
	virtual tjs_error TJS_INTF_METHOD Process(
			/*in*/tjs_uint64 tick, // tick count provided by the system in ms
			/*in*/iTVPLayerUpdater * updater, // layer updater object
			/*in*/iTVPScanLineProvider * dest, // destination
			/*in*/iTVPScanLineProvider * src1, // source 1
			/*in*/iTVPScanLineProvider * src2 // source 2
		) = 0;
	// process the transition.
	// callee must call updater->UpdateLayerRect when changing the layer image.
	// updater->UpdateLayerRect can be called more than once.
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPTransHandlerProvider
//---------------------------------------------------------------------------
// transition handler provider abstract class
class iTVPTransHandlerProvider
{
public:
	virtual ~iTVPTransHandlerProvider() {} // add by ZeaS
	virtual tjs_error TJS_INTF_METHOD AddRef() = 0;
	virtual tjs_error TJS_INTF_METHOD Release() = 0;

	virtual tjs_error TJS_INTF_METHOD GetName(
			/*out*/const tjs_char ** name) = 0;
		// return this transition name

	virtual tjs_error TJS_INTF_METHOD StartTransition(
			/*in*/iTVPSimpleOptionProvider *options, // option provider
			/*in*/iTVPSimpleImageProvider *imagepro, // image provider
			/*in*/tTVPLayerType layertype, // destination layer type
			/*in*/tjs_uint src1w, tjs_uint src1h, // source 1 size
			/*in*/tjs_uint src2w, tjs_uint src2h, // source 2 size
			/*out*/tTVPTransType *type, // transition type
			/*out*/tTVPTransUpdateType * updatetype, // update typwe
			/*out*/iTVPBaseTransHandler ** handler // transition handler
			) = 0;
		// start transition and return a handler.
		// "handler" is an object of iTVPDivisibleTransHandler when
		// updatetype is tutDivisibleFade or tutDivisible.
		// Otherwise is an object of iTVPGiveUpdateTransHandler ( cast to
		// each class to use it )
		// layertype is the destination layer type.
};
//---------------------------------------------------------------------------
/*]*/

#endif



