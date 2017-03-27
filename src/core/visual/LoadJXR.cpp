#include "tjsCommHead.h"

#include "GraphicsLoaderIntf.h"
#include "LayerBitmapIntf.h"
#include "StorageIntf.h"
#include "MsgIntf.h"
#include "tvpgl.h"

#include "DebugIntf.h"
#include "tjsDictionary.h"
#include "ScriptMgnIntf.h"

static tjs_uint32 GetStride( const tjs_uint32 width, const tjs_uint32 bitCount) {
	const tjs_uint32 byteCount = bitCount / 8;
	const tjs_uint32 stride = (width * byteCount + 3) & ~3;
	return stride;
}

bool TVPAcceptSaveAsJXR(void* formatdata, const ttstr & type, class iTJSDispatch2** dic ) {
	bool result = false;
	if( type.StartsWith(TJS_W("jxr")) ) result = true;
	else if( type == TJS_W(".jxr") ) result = true;
	if( result && dic ) {
		// quality 1 - 255
		// alphaQuality 1 - 255
		// subsampling : select 0 : 4:0:0, 1 : 4:2:0, 2 : 4:2:2, 3 : 4:4:4
		// alpha : bool
		/*
		%[
		"quality" => %["type" => "range", "min" => 1, "max" => 255, "desc" => "1 is lossless, 2 - 255 lossy : 2 is high quality, 255 is low quality", "default"=>1 ]
		"alphaQuality" => %["type" => "range", "min" => 1, "max" => 255, "desc" => "1 is lossless, 2 - 255 lossy : 2 is high quality, 255 is low quality", "default"=>1 ]
		"subsampling" => %["type" => "select", "items" => ["4:0:0","4:2:0","4:2:2","4:4:4"], "desc"=>"subsampling", "default"=>1 ]
		"alpha" => %["type" => "boolean", "default"=>1]
		]
		*/
		tTJSVariant result;
		TVPExecuteExpression(
			TJS_W("(const)%[")
			TJS_W("\"quality\"=>(const)%[\"type\"=>\"range\",\"min\"=>1,\"max\"=>255,\"desc\"=>\"1 is lossless, 2 - 255 lossy : 2 is high quality, 255 is low quality\",\"default\"=>1],")
			TJS_W("\"alphaQuality\"=>(const)%[\"type\"=>\"range\",\"min\"=>1,\"max\"=>255,\"desc\"=>\"1 is lossless, 2 - 255 lossy : 2 is high quality, 255 is low quality\",\"default\"=>1],")
			TJS_W("\"subsampling\"=>(const)%[\"type\"=>\"select\",\"items\"=>(const)[\"4:0:0\",\"4:2:0\",\"4:2:2\",\"4:4:4\"],\"desc\"=>\"subsampling\",\"default\"=>1],")
			TJS_W("\"alpha\"=>(const)%[\"type\"=>\"boolean\",\"desc\"=>\"alpha channel\",\"default\"=>true]")
			TJS_W("]"),
			NULL, &result );
		if( result.Type() == tvtObject ) {
			*dic = result.AsObject();
		}
	}
	return result;
}
// jxrlib を使うバージョンでは書き込みがうまくできていない
//#define TVP_JPEG_XR_USE_WIN_CODEC
#if defined( WIN32 ) && defined( TVP_JPEG_XR_USE_WIN_CODEC )
// Windows 組み込み機能で JPEG XR を開く場合はこちら
#include <wincodec.h>
#include <wincodecsdk.h>
#include <atlbase.h>
#include <comutil.h>
#include "StorageImpl.h"
#pragma comment(lib, "WindowsCodecs.lib")
#ifdef _DEBUG
#pragma comment(lib, "comsuppwd.lib")
#else
#pragma comment(lib, "comsuppw.lib")
#endif


void TVPLoadJXR(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	CoInitialize(NULL);
	{
		tTVPIStreamAdapter* s = new tTVPIStreamAdapter( src );
		CComPtr<tTVPIStreamAdapter> stream( s );
		s->Release();
		{
			CComPtr<IWICBitmapDecoder> decoder;
			HRESULT hr = decoder.CoCreateInstance(CLSID_WICWmpDecoder);
			hr = decoder->Initialize( stream, WICDecodeMetadataCacheOnDemand);
			UINT frameCount = 0;
			hr = decoder->GetFrameCount(&frameCount);
			for( UINT index = 0; index < frameCount; ++index ) {
				CComPtr<IWICBitmapFrameDecode> frame;
				hr = decoder->GetFrame(index, &frame);
				UINT width = 0;
				UINT height = 0;
				GUID pixelFormat = { 0 };
				hr = frame->GetPixelFormat(&pixelFormat);
				hr = frame->GetSize(&width, &height);
				const UINT stride = GetStride(width, 32);
#ifdef _DEBUG
				std::vector<tjs_uint8> buff(stride*height*sizeof(tjs_uint8));
#else
				std::vector<tjs_uint8> buff;
				buff.reserve(stride*height*sizeof(tjs_uint8));
#endif
				sizecallback(callbackdata, width, height);
				WICRect rect = {0, 0, width, height};
				if( !IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppBGRA) ) {
					CComPtr<IWICFormatConverter> converter;
					CComPtr<IWICImagingFactory> wicFactory;
					hr = wicFactory.CoCreateInstance( CLSID_WICImagingFactory );
					wicFactory->CreateFormatConverter(&converter);
					converter->Initialize(frame, GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone, NULL, 0.0f, WICBitmapPaletteTypeCustom);
					hr = converter->CopyPixels( &rect, stride, stride*height, (BYTE*)&buff[0] );
				} else {
					hr = frame->CopyPixels( &rect, stride, stride*height, (BYTE*)&buff[0] );
				}
				int offset = 0;
				for( UINT i = 0; i < height; i++) {
					void *scanline = scanlinecallback(callbackdata, i);
					memcpy( scanline, &buff[offset], width*sizeof(tjs_uint32));
					offset += stride;
					scanlinecallback(callbackdata, -1);
				} 
				break;
			}
		}
		if( stream ) stream->ClearStream();
	}
	CoUninitialize();
}

void TVPSaveAsJXR(void* formatdata, tTJSBinaryStream* dst, const class iTVPBaseBitmap* image, const ttstr & mode, class iTJSDispatch2* meta )
{
	CoInitialize(NULL);
	{
		tTVPIStreamAdapter* s = new tTVPIStreamAdapter( dst );
		CComPtr<tTVPIStreamAdapter> stream( s );
		s->Release();
		{
			CComPtr<IWICBitmapEncoder> encoder;
			HRESULT hr = encoder.CoCreateInstance(CLSID_WICWmpEncoder);
			if( SUCCEEDED(hr) ) hr = encoder->Initialize(stream, WICBitmapEncoderNoCache);
			CComPtr<IPropertyBag2> property;
			CComPtr<IWICBitmapFrameEncode> frame;
			if( SUCCEEDED(hr) ) hr = encoder->CreateNewFrame( &frame, &property );
			WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
			if( SUCCEEDED(hr) && meta ) {
				PROPBAG2 option = { 0 };
				option.pstrName = L"UseCodecOptions";	// 詳細なオプションを指定する
				_variant_t varValue( true );
				hr = property->Write( 1, &option, &varValue );

				// EnumCallback を使ってプロパティを設定する
				struct MetaDictionaryEnumCallback : public tTJSDispatch {
					IPropertyBag2 *prop_;
					WICPixelFormatGUID *format_;
					MetaDictionaryEnumCallback( IPropertyBag2 *prop, WICPixelFormatGUID* format ) : prop_(prop), format_(format) {}
					tjs_error TJS_INTF_METHOD FuncCall(tjs_uint32 flag, const tjs_char * membername,
						tjs_uint32 *hint, tTJSVariant *result, tjs_int numparams,
						tTJSVariant **param, iTJSDispatch2 *objthis) { // called from tTJSCustomObject::EnumMembers
						if(numparams < 3) return TJS_E_BADPARAMCOUNT;
						tjs_uint32 flags = (tjs_int)*param[1];
						if(flags & TJS_HIDDENMEMBER) {	// hidden members are not processed
							if(result) *result = (tjs_int)1;
							return TJS_S_OK;
						}
						HRESULT hr;
						// push items
						ttstr value = *param[0];
						if( value == TJS_W("quality") ) {
							tjs_int64 v = param[2]->AsInteger();
							v = v < 1 ? 1 : v > 255 ? 255 : v;
							UCHAR q = (UCHAR)v;
							PROPBAG2 option = { 0 };
							option.pstrName = L"Quality";	//  1 : lossless mode, 2 - 255 : 値が小さい方が高画質
							_variant_t varValue( q );
							hr = prop_->Write( 1, &option, &varValue );
							if( q == 1 ) {
								option.pstrName = L"Lossless";	// ロスレスモード
								_variant_t var( VARIANT_TRUE );
								hr = prop_->Write( 1, &option, &var );
							}
						} else if( value == TJS_W("alphaQuality") ) {
							tjs_int64 v = param[2]->AsInteger();
							v = v < 1 ? 1 : v > 255 ? 255 : v;
							UCHAR q = (UCHAR)v;
							PROPBAG2 option = { 0 };
							option.pstrName = L"InterleavedAlpha";
							if( q == 1 ) {
								_variant_t varValue( VARIANT_TRUE );
								hr = prop_->Write( 1, &option, &varValue );
							} else {
								_variant_t varValue( VARIANT_FALSE );
								hr = prop_->Write( 1, &option, &varValue );
							}
							option.pstrName = L"AlphaQuality";	//  1 : lossless mode, 2 - 255 : 値が小さい方が高画質
							_variant_t varValue( q );
							hr = prop_->Write( 1, &option, &varValue );
						} else if( value == TJS_W("subsampling") ) {
							// 0 : 4:0:0, 1 : 4:2:0, 2 : 4:2:2, 3 : 4:4:4
							tjs_int64 v = param[2]->AsInteger();
							v = v < 0 ? 0 : v > 3 ? 3 : v;
							UCHAR q = (UCHAR)v;
							PROPBAG2 option = { 0 };
							option.pstrName = L"Subsampling";
							_variant_t varValue( q );
							hr = prop_->Write( 1, &option, &varValue );
						} else if( value == TJS_W("alpha") ) {
							// true or false
							tjs_int64 v = param[2]->AsInteger();
							if( v ) {
								*format_ = GUID_WICPixelFormat32bppBGRA;
							} else {
								*format_ = GUID_WICPixelFormat32bppBGR;
							}
						}
						if(result) *result = (tjs_int)1;
						return TJS_S_OK;
					}
				} callback( property, &format );
				meta->EnumMembers(TJS_IGNOREPROP, &tTJSVariantClosure(&callback, NULL), meta);
			}

			if( SUCCEEDED(hr) ) hr = frame->Initialize( property );
			const UINT width = image->GetWidth();
			const UINT height = image->GetHeight();
			if( SUCCEEDED(hr) ) hr = frame->SetSize( width, height );
			if( SUCCEEDED(hr) ) hr = frame->SetPixelFormat(&format);
			const UINT stride = width * sizeof(tjs_uint32);
			const UINT buffersize = stride * height;
			if( SUCCEEDED(hr) ) {
#ifdef _DEBUG
				std::vector<tjs_uint8> buff(buffersize);
#else
				std::vector<tjs_uint8> buff;
				buff.reserve(buffersize);
#endif
				for( UINT i = 0; i < height; i++ ) {
					memcpy( &buff[i*stride], image->GetScanLine(i), stride );
				}
				hr = frame->WritePixels( height, stride, buffersize, &buff[0] );
			}
			if( SUCCEEDED(hr) ) hr = frame->Commit();
			if( SUCCEEDED(hr) ) hr = encoder->Commit();
		}
		if( stream ) stream->ClearStream();
	}
	CoUninitialize();
}

void TVPLoadHeaderJXR(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic )
{
	CoInitialize(NULL);
	{
		tTVPIStreamAdapter* s = new tTVPIStreamAdapter( src );
		CComPtr<tTVPIStreamAdapter> stream( s );
		s->Release();
		CComPtr<IWICBitmapDecoder> decoder;
		HRESULT hr = decoder.CoCreateInstance(CLSID_WICWmpDecoder);
		if( SUCCEEDED(hr) ) hr = decoder->Initialize( stream, WICDecodeMetadataCacheOnDemand);
		UINT frameCount = 0;
		if( SUCCEEDED(hr) ) hr = decoder->GetFrameCount(&frameCount);
		if( SUCCEEDED(hr) )
		{
			for( UINT index = 0; index < frameCount; ++index ) {
				UINT width = 0;
				UINT height = 0;
				GUID pixelFormat = { 0 };
				double dpiX = 0.0;
				double dpiY = 0.0;
				CComPtr<IWICBitmapFrameDecode> frame;
				if( SUCCEEDED(hr) ) hr = decoder->GetFrame(index, &frame);
				if( SUCCEEDED(hr) ) hr = frame->GetPixelFormat(&pixelFormat);
				if( SUCCEEDED(hr) ) hr = frame->GetSize(&width, &height);
				if( SUCCEEDED(hr) ) hr = frame->GetResolution( &dpiX, &dpiY );
				if( SUCCEEDED(hr) )
				{
					*dic = TJSCreateDictionaryObject();
					tTJSVariant val((tjs_int64)width);
					(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic) );
					val = tTJSVariant((tjs_int64)height);
					(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic) );
					val = tTJSVariant(dpiX);
					(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("dpi x"), 0, &val, (*dic) );
					val = tTJSVariant(dpiY);
					(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("dpi y"), 0, &val, (*dic) );
					if( IsEqualGUID( pixelFormat, GUID_WICPixelFormatBlackWhite) ) {
						val = tTJSVariant(TJS_W("BlackWhite"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat8bppGray) ) {
						val = tTJSVariant(TJS_W("8bppGray"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat16bppBGR555) ) {
						val = tTJSVariant(TJS_W("16bppBGR555"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat16bppGray) ) {
						val = tTJSVariant(TJS_W("16bppGray"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat24bppBGR) ) {
						val = tTJSVariant(TJS_W("24bppBGR"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat24bppRGB) ) {
						val = tTJSVariant(TJS_W("24bppRGB"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppBGR) ) {
						val = tTJSVariant(TJS_W("32bppBGR"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppBGRA) ) {
						val = tTJSVariant(TJS_W("32bppBGRA"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat48bppRGBFixedPoint) ) {
						val = tTJSVariant(TJS_W("48bppRGBFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat16bppGrayFixedPoint) ) {
						val = tTJSVariant(TJS_W("16bppGrayFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppBGR101010) ) {
						val = tTJSVariant(TJS_W("32bppBGR101010"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat48bppRGB) ) {
						val = tTJSVariant(TJS_W("48bppRGB"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppRGBA) ) {
						val = tTJSVariant(TJS_W("64bppRGBA"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat96bppRGBFixedPoint) ) {
						val = tTJSVariant(TJS_W("96bppRGBFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat96bppRGBFixedPoint) ) {
						val = tTJSVariant(TJS_W("96bppRGBFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat128bppRGBFloat) ) {
						val = tTJSVariant(TJS_W("128bppRGBFloat"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppCMYK) ) {
						val = tTJSVariant(TJS_W("32bppCMYK"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppRGBAFixedPoint) ) {
						val = tTJSVariant(TJS_W("64bppRGBAFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat128bppRGBAFixedPoint) ) {
						val = tTJSVariant(TJS_W("128bppRGBAFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppCMYK) ) {
						val = tTJSVariant(TJS_W("64bppCMYK"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat24bpp3Channels) ) {
						val = tTJSVariant(TJS_W("24bpp3Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bpp4Channels) ) {
						val = tTJSVariant(TJS_W("32bpp4Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat40bpp5Channels) ) {
						val = tTJSVariant(TJS_W("40bpp5Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat48bpp6Channels) ) {
						val = tTJSVariant(TJS_W("48bpp6Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat56bpp7Channels) ) {
						val = tTJSVariant(TJS_W("56bpp7Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bpp8Channels) ) {
						val = tTJSVariant(TJS_W("64bpp8Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat48bpp3Channels) ) {
						val = tTJSVariant(TJS_W("48bpp3Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bpp4Channels) ) {
						val = tTJSVariant(TJS_W("64bpp4Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat80bpp5Channels) ) {
						val = tTJSVariant(TJS_W("80bpp5Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat96bpp6Channels) ) {
						val = tTJSVariant(TJS_W("96bpp6Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat112bpp7Channels) ) {
						val = tTJSVariant(TJS_W("112bpp7Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat128bpp8Channels) ) {
						val = tTJSVariant(TJS_W("128bpp8Channels"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat40bppCMYKAlpha) ) {
						val = tTJSVariant(TJS_W("40bppCMYKAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat80bppCMYKAlpha) ) {
						val = tTJSVariant(TJS_W("80bppCMYKAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bpp3ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("32bpp3ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bpp7ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("64bpp7ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat72bpp8ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("72bpp8ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bpp3ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("64bpp3ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat80bpp4ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("80bpp4ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat96bpp5ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("96bpp5ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat112bpp6ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("112bpp6ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat128bpp7ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("128bpp7ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat144bpp8ChannelsAlpha) ) {
						val = tTJSVariant(TJS_W("144bpp8ChannelsAlpha"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppRGBAHalf) ) {
						val = tTJSVariant(TJS_W("64bppRGBAHalf"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat48bppRGBHalf) ) {
						val = tTJSVariant(TJS_W("48bppRGBHalf"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppRGBE) ) {
						val = tTJSVariant(TJS_W("32bppRGBE"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat16bppGrayHalf) ) {
						val = tTJSVariant(TJS_W("16bppGrayHalf"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat32bppGrayFixedPoint) ) {
						val = tTJSVariant(TJS_W("32bppGrayFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppRGBFixedPoint) ) {
						val = tTJSVariant(TJS_W("64bppRGBFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat128bppRGBFixedPoint) ) {
						val = tTJSVariant(TJS_W("128bppRGBFixedPoint"));
					} else if( IsEqualGUID( pixelFormat, GUID_WICPixelFormat64bppRGBHalf) ) {
						val = tTJSVariant(TJS_W("64bppRGBHalf"));
					} else {
						val = tTJSVariant(TJS_W("unknown"));
					}
					(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("pixelformat"), 0, &val, (*dic) );
				}
				break;
			}
		}
		if( stream ) stream->ClearStream();
	}
	CoUninitialize();
}
#else

#include "JXRGlue.h"

/*
jxrlibで読み込みを行う場合は
external/jxrlib/image/vc11projects/CommonLib_vc11.vcxproj
external/jxrlib/image/vc11projects/DecodeLib_vc11.vcxproj
external/jxrlib/image/vc11projects/EncodeLib_vc11.vcxproj
external/jxrlib/jxrgluelib/JXRGlueLib_vc11.vcxproj
をソリューションに加えてビルドすること
*/
#if defined( WIN32 )
#ifdef _DEBUG
// #pragma comment(lib, "JXRCommonLib_d.lib")
// #pragma comment(lib, "JXRDecodeLib_d.lib")
// #pragma comment(lib, "JXREncodeLib_d.lib")
#pragma comment(lib, "JXRGlueLib_d.lib")
#else
#pragma comment(lib, "JXRCommonLib.lib")
#pragma comment(lib, "JXRDecodeLib.lib")
#pragma comment(lib, "JXREncodeLib.lib")
#pragma comment(lib, "JXRGlueLib.lib")
#endif
#endif

static ERR JXR_close( WMPStream** ppWS ) {
	/* 何もしない */
	return WMP_errSuccess;
}
static Bool JXR_EOS(struct WMPStream* pWS) {
	tTJSBinaryStream* src = ((tTJSBinaryStream*)(pWS->state.pvObj));
	return src->GetPosition() >= src->GetSize();
}
static ERR JXR_read(struct WMPStream* pWS, void* pv, size_t cb) {
	tTJSBinaryStream* src = ((tTJSBinaryStream*)(pWS->state.pvObj));
	tjs_uint size = src->Read( pv, (tjs_uint)cb );
	return size == cb ? WMP_errSuccess : WMP_errFileIO;
}
static ERR JXR_write(struct WMPStream* pWS, const void* pv, size_t cb) {
	ERR err = WMP_errSuccess;
	if( 0 != cb ) {
		tTJSBinaryStream* src = ((tTJSBinaryStream*)(pWS->state.pvObj));
		tjs_uint size = src->Write( pv, (tjs_uint)cb );
		err = size == cb ? WMP_errSuccess : WMP_errFileIO;
	}
	return err;
}
static ERR JXR_set_pos( struct WMPStream* pWS, size_t offPos ) {
	tTJSBinaryStream* src = ((tTJSBinaryStream*)(pWS->state.pvObj));
	tjs_uint64 pos = src->Seek(  offPos, TJS_BS_SEEK_SET );
	return pos == offPos ? WMP_errSuccess : WMP_errFileIO;
}
static ERR JXR_get_pos( struct WMPStream* pWS, size_t* poffPos ) {
	tTJSBinaryStream* src = ((tTJSBinaryStream*)(pWS->state.pvObj));
	*poffPos = (size_t)src->GetPosition();
	return WMP_errSuccess;
}


#define SAFE_CALL( func ) if( Failed(err = (func)) ) { TVPThrowExceptionMessage( TJS_W("JPEG XR read error/%1"), (tjs_int)err ); }
//---------------------------------------------------------------------------
void TVPLoadJXR(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx,  tTVPGraphicLoadMode mode)
{
	if( glmNormal != mode ) {
		// not supprted yet.
		TVPThrowExceptionMessage( TJS_W("JPEG XR read error/%1"), TJS_W("not supprted yet.") );
	}
	
	//PKFactory* pFactory = NULL;
	PKImageDecode* pDecoder = NULL;
	//PKFormatConverter* pConverter = NULL;
	WMPStream* pStream = NULL;
	//PKCodecFactory* pCodecFactory = NULL;
	try {
		ERR err;
		//SAFE_CALL( PKCreateFactory(&pFactory, PK_SDK_VERSION) );
		//SAFE_CALL( PKCreateCodecFactory(&pCodecFactory, WMP_SDK_VERSION) );
		//SAFE_CALL(pCodecFactory->CreateDecoderFromFile("test.jxr", &pDecoder));

		const PKIID* pIID = NULL;
		SAFE_CALL( GetImageDecodeIID(".jxr", &pIID) );
		// create stream
		pStream = (WMPStream*)calloc(1, sizeof(WMPStream));
		pStream->state.pvObj = (void*)src;
		pStream->Close = JXR_close;
		pStream->EOS = JXR_EOS;
		pStream->Read = JXR_read;
		pStream->Write = JXR_write;
		pStream->SetPos = JXR_set_pos;
		pStream->GetPos = JXR_get_pos;
		// Create decoder
		SAFE_CALL( PKCodecFactory_CreateCodec(pIID, (void **)&pDecoder) );
		// attach stream to decoder
		SAFE_CALL( pDecoder->Initialize(pDecoder, pStream) );
		pDecoder->fStreamOwner = !0;

		PKPixelFormatGUID srcFormat;
		pDecoder->GetPixelFormat( pDecoder, &srcFormat );
		PKPixelInfo PI;
		PI.pGUIDPixFmt = &srcFormat;
		PixelFormatLookup(&PI, LOOKUP_FORWARD);

		pDecoder->WMP.wmiSCP.bfBitstreamFormat = SPATIAL;
        if(!!(PI.grBit & PK_pixfmtHasAlpha))
            pDecoder->WMP.wmiSCP.uAlphaMode = 2;
        else
            pDecoder->WMP.wmiSCP.uAlphaMode = 0;
		pDecoder->WMP.wmiSCP.sbSubband = SB_ALL;
		pDecoder->WMP.bIgnoreOverlap = FALSE;
		pDecoder->WMP.wmiI.cfColorFormat = PI.cfColorFormat;
		pDecoder->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
		pDecoder->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
		pDecoder->WMP.wmiI.cThumbnailWidth = pDecoder->WMP.wmiI.cWidth;
		pDecoder->WMP.wmiI.cThumbnailHeight = pDecoder->WMP.wmiI.cHeight;
		pDecoder->WMP.wmiI.bSkipFlexbits = FALSE;
		pDecoder->WMP.wmiI.cROILeftX = 0;
		pDecoder->WMP.wmiI.cROITopY = 0;
		pDecoder->WMP.wmiI.cROIWidth = pDecoder->WMP.wmiI.cThumbnailWidth;
		pDecoder->WMP.wmiI.cROIHeight = pDecoder->WMP.wmiI.cThumbnailHeight;
		pDecoder->WMP.wmiI.oOrientation = O_NONE;
		pDecoder->WMP.wmiI.cPostProcStrength = 0;
		pDecoder->WMP.wmiSCP.bVerbose = FALSE;

		int width = 0;
		int height = 0;
		pDecoder->GetSize( pDecoder, &width, &height );
		if( width == 0 || height == 0 ) {
			TVPThrowExceptionMessage( TJS_W("JPEG XR read error/%1"), TJS_W("width or height is zero.") );
		}
		sizecallback(callbackdata, width, height, pDecoder->WMP.wmiSCP.uAlphaMode ? gpfRGBA : gpfRGB);
		const tjs_uint32 stride = GetStride( (tjs_uint32)width, (tjs_uint32)32 );
		PKRect rect = {0, 0, width, height};
#ifdef _DEBUG
		std::vector<tjs_uint8> buff(stride*height*sizeof(tjs_uint8));
#else
		std::vector<tjs_uint8> buff;
		buff.reserve(stride*height*sizeof(tjs_uint8));
#endif
		// rect で1ラインずつ指定してデコードする方法はjxrlibではうまくいかない様子
		int offset = 0;
		if (!IsEqualGUID(srcFormat, GUID_PKPixelFormat32bppRGBA)) {
			if( IsEqualGUID( srcFormat, GUID_PKPixelFormat24bppRGB ) ) {
				pDecoder->Copy( pDecoder, &rect, (U8*)&buff[0], stride );
				for( int i = 0; i < height; i++) {
					void *scanline = scanlinecallback(callbackdata, i);
					tjs_uint8* d = (tjs_uint8*)scanline;
					tjs_uint8* s = (tjs_uint8*)&buff[offset];
					for( int x = 0; x < width; x++ ) {
						d[0] = s[2];
						d[1] = s[1];
						d[2] = s[0];
						d[3] = 0xff;
						d+=4;
						s+=3;
					}
					offset += stride;
					scanlinecallback(callbackdata, -1);
				}
			} else {
				TVPThrowExceptionMessage( TJS_W("JPEG XR read error/%1"), TJS_W("Not supported this file format yet.") );
			}
			/*
			Converter はどうもおかしいので使わない。
			SAFE_CALL( pCodecFactory->CreateFormatConverter(&pConverter) );
			SAFE_CALL( pConverter->Initialize(pConverter, pDecoder, NULL, GUID_PKPixelFormat32bppPBGRA) );
			pConverter->Copy( pConverter, &rect, (U8*)&buff[0], width*sizeof(int));
			*/
		} else {
			// アルファチャンネルが入っている時メモリリークしている(jxrlib直した)
			pDecoder->Copy( pDecoder, &rect, (U8*)&buff[0], stride );
			for( int i = 0; i < height; i++) {
				void *scanline = scanlinecallback(callbackdata, i);
				memcpy( scanline, &buff[offset], width*sizeof(tjs_uint32));
				offset += stride;
				scanlinecallback(callbackdata, -1);
			} 
		}
		//if( pConverter ) pConverter->Release(&pConverter);
		if( pDecoder ) pDecoder->Release(&pDecoder);
		//if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);
		if( pStream ) free( pStream );
	} catch(...) {
		//if( pConverter ) pConverter->Release(&pConverter);
		if( pDecoder ) pDecoder->Release(&pDecoder);
		//if( pCodecFactory ) pCodecFactory->Release(&pCodecFactory);
		if( pStream ) free( pStream );
		throw;
	}
}

// Y, U, V, YHP, UHP, VHP
static int DPK_QPS_420[12][6] = {      // for 8 bit only
    { 66, 65, 70, 72, 72, 77 },
    { 59, 58, 63, 64, 63, 68 },
    { 52, 51, 57, 56, 56, 61 },
    { 48, 48, 54, 51, 50, 55 },
    { 43, 44, 48, 46, 46, 49 },
    { 37, 37, 42, 38, 38, 43 },
    { 26, 28, 31, 27, 28, 31 },
    { 16, 17, 22, 16, 17, 21 },
    { 10, 11, 13, 10, 10, 13 },
    {  5,  5,  6,  5,  5,  6 },
    {  2,  2,  3,  2,  2,  2 }
};
static int DPK_QPS_8[12][6] = {
    { 67, 79, 86, 72, 90, 98 },
    { 59, 74, 80, 64, 83, 89 },
    { 53, 68, 75, 57, 76, 83 },
    { 49, 64, 71, 53, 70, 77 },
    { 45, 60, 67, 48, 67, 74 },
    { 40, 56, 62, 42, 59, 66 },
    { 33, 49, 55, 35, 51, 58 },
    { 27, 44, 49, 28, 45, 50 },
    { 20, 36, 42, 20, 38, 44 },
    { 13, 27, 34, 13, 28, 34 },
    {  7, 17, 21,  8, 17, 21 }, // Photoshop 100%
    {  2,  5,  6,  2,  5,  6 }
};
// TODO : 以下の処理ではうまく書き込めていない。どうも設定が不味い様子
void TVPSaveAsJXR(void* formatdata, tTJSBinaryStream* dst, const class iTVPBaseBitmap* image, const ttstr & mode, class iTJSDispatch2* meta )
{
	PKImageEncode* pEncoder = NULL;
	WMPStream* pStream = NULL;
	try {
		ERR err;

		const PKIID* pIID = NULL;
		SAFE_CALL( GetImageEncodeIID(".jxr", &pIID) );
		// create stream
		pStream = (WMPStream*)calloc(1, sizeof(WMPStream));
		pStream->state.pvObj = (void*)dst;
		pStream->Close = JXR_close;
		pStream->EOS = JXR_EOS;
		pStream->Read = JXR_read;
		pStream->Write = JXR_write;
		pStream->SetPos = JXR_set_pos;
		pStream->GetPos = JXR_get_pos;
		// Create encoder
		SAFE_CALL( PKCodecFactory_CreateCodec(pIID, (void **)&pEncoder) );
		
		//PKPixelInfo PI;
		//PI.pGUIDPixFmt = &GUID_PKPixelFormat32bppBGRA;
		//PixelFormatLookup(&PI, LOOKUP_FORWARD);
		
		const tjs_uint width = image->GetWidth();
		const tjs_uint height = image->GetHeight();
		const tjs_uint stride = width * sizeof(tjs_uint32);
		const tjs_uint buffersize = stride * height;
		CWMIStrCodecParam wmiSCP;
		
		wmiSCP.bVerbose = FALSE;
		wmiSCP.cfColorFormat = YUV_444;	// Y_ONLY / YUV_420 / YUV_422 / YUV_444
		wmiSCP.bdBitDepth = BD_SHORT; // BD_LONG;
		//wmiSCP.bfBitstreamFormat = SPATIAL;
		wmiSCP.bfBitstreamFormat = FREQUENCY;
		//wmiSCP.uAlphaMode = 2;	// alpha + color
		wmiSCP.uAlphaMode = 0;	// color

		wmiSCP.bProgressiveMode = TRUE;
		wmiSCP.olOverlap = OL_ONE;
		wmiSCP.cNumOfSliceMinus1H = wmiSCP.cNumOfSliceMinus1V = 0;
		wmiSCP.sbSubband = SB_ALL;

		wmiSCP.uiDefaultQPIndex = 1;
		wmiSCP.uiDefaultQPIndexAlpha = 1;
		
		U32 uTileY = 1 * MB_HEIGHT_PIXEL;
        wmiSCP.cNumOfSliceMinus1H = (U32)height < (uTileY >> 1) ? 0 : (height + (uTileY >> 1)) / uTileY - 1;
		U32 uTileX = 1 * MB_HEIGHT_PIXEL;
		wmiSCP.cNumOfSliceMinus1V = (U32)width < (uTileX >> 1) ? 0 : (width + (uTileX >> 1)) / uTileX - 1;

		// attach stream to encoder
		SAFE_CALL( pEncoder->Initialize(pEncoder, pStream, &wmiSCP, sizeof(wmiSCP)) );
		
		/*
		float quality = 0.99f;
		int qi;
		float qf;
		qi = (int) (10.f * quality);
        qf = 10.f * quality - (float) qi;
		int* pQPs = (pEncoder->WMP.wmiSCP.cfColorFormat == YUV_420 || pEncoder->WMP.wmiSCP.cfColorFormat == YUV_422) ? DPK_QPS_420[qi] : DPK_QPS_8[qi];
		pEncoder->WMP.wmiSCP.uiDefaultQPIndex = (U8) (0.5f + (float) pQPs[0] * (1.f - qf) + (float) (pQPs + 6)[0] * qf);
		pEncoder->WMP.wmiSCP.uiDefaultQPIndexU = (U8) (0.5f + (float) pQPs[1] * (1.f - qf) + (float) (pQPs + 6)[1] * qf);
		pEncoder->WMP.wmiSCP.uiDefaultQPIndexV = (U8) (0.5f + (float) pQPs[2] * (1.f - qf) + (float) (pQPs + 6)[2] * qf);
		pEncoder->WMP.wmiSCP.uiDefaultQPIndexYHP = (U8) (0.5f + (float) pQPs[3] * (1.f - qf) + (float) (pQPs + 6)[3] * qf);
		pEncoder->WMP.wmiSCP.uiDefaultQPIndexUHP = (U8) (0.5f + (float) pQPs[4] * (1.f - qf) + (float) (pQPs + 6)[4] * qf);
		pEncoder->WMP.wmiSCP.uiDefaultQPIndexVHP = (U8) (0.5f + (float) pQPs[5] * (1.f - qf) + (float) (pQPs + 6)[5] * qf);
		*/
		
		pEncoder->WMP.wmiSCP.uiDefaultQPIndex = 1;
		if(pEncoder->WMP.wmiSCP.uAlphaMode == 2)
			pEncoder->WMP.wmiSCP_Alpha.uiDefaultQPIndex = wmiSCP.uiDefaultQPIndexAlpha;

		pEncoder->WMP.wmiSCP.olOverlap = OL_ONE;

		// 以下でアルファの設定か
		// pEncoder->WMP.wmiI_Alpha;
        // pEncoder->WMP.wmiSCP_Alpha;
		//pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat32bppBGRA);
		pEncoder->SetPixelFormat(pEncoder, GUID_PKPixelFormat32bppRGB);
		pEncoder->SetSize(pEncoder, image->GetWidth(), image->GetHeight() );
		//Float rX = 98.0, rY = 98.0;
		//pEncoder->SetResolution(pEncoder, rX, rY);
		
#ifdef _DEBUG
		std::vector<tjs_uint8> buff(buffersize);
#else
		std::vector<tjs_uint8> buff;
		buff.reserve(buffersize);
#endif
		for (tjs_uint i = 0; i < height; i++) {
			memcpy( &buff[i*stride], image->GetScanLine(i), stride );
		}
		pEncoder->WritePixels( pEncoder, height, &buff[0], stride );

		pEncoder->Release(&pEncoder);
		if( pStream ) free( pStream );
	} catch(...) {
		if( pEncoder ) pEncoder->Release(&pEncoder);
		if( pStream ) free( pStream );
		throw;
	}
}
void TVPLoadHeaderJXR(void* formatdata, tTJSBinaryStream *src, iTJSDispatch2** dic )
{
	PKImageDecode* pDecoder = NULL;
	WMPStream* pStream = NULL;
	try {
		ERR err;

		const PKIID* pIID = NULL;
		SAFE_CALL( GetImageDecodeIID(".jxr", &pIID) );
		// create stream
		pStream = (WMPStream*)calloc(1, sizeof(WMPStream));
		pStream->state.pvObj = (void*)src;
		pStream->Close = JXR_close;
		pStream->EOS = JXR_EOS;
		pStream->Read = JXR_read;
		pStream->Write = JXR_write;
		pStream->SetPos = JXR_set_pos;
		pStream->GetPos = JXR_get_pos;
		// Create decoder
		SAFE_CALL( PKCodecFactory_CreateCodec(pIID, (void **)&pDecoder) );
		// attach stream to decoder
		SAFE_CALL( pDecoder->Initialize(pDecoder, pStream) );
		pDecoder->fStreamOwner = !0;

		PKPixelFormatGUID srcFormat;
		pDecoder->GetPixelFormat( pDecoder, &srcFormat );
		PKPixelInfo PI;
		PI.pGUIDPixFmt = &srcFormat;
		PixelFormatLookup(&PI, LOOKUP_FORWARD);

		pDecoder->WMP.wmiSCP.bfBitstreamFormat = SPATIAL;
        if(!!(PI.grBit & PK_pixfmtHasAlpha))
            pDecoder->WMP.wmiSCP.uAlphaMode = 2;
        else
            pDecoder->WMP.wmiSCP.uAlphaMode = 0;
		pDecoder->WMP.wmiSCP.sbSubband = SB_ALL;
		pDecoder->WMP.bIgnoreOverlap = FALSE;
		pDecoder->WMP.wmiI.cfColorFormat = PI.cfColorFormat;
		pDecoder->WMP.wmiI.bdBitDepth = PI.bdBitDepth;
		pDecoder->WMP.wmiI.cBitsPerUnit = PI.cbitUnit;
		pDecoder->WMP.wmiI.cThumbnailWidth = pDecoder->WMP.wmiI.cWidth;
		pDecoder->WMP.wmiI.cThumbnailHeight = pDecoder->WMP.wmiI.cHeight;
		pDecoder->WMP.wmiI.bSkipFlexbits = FALSE;
		pDecoder->WMP.wmiI.cROILeftX = 0;
		pDecoder->WMP.wmiI.cROITopY = 0;
		pDecoder->WMP.wmiI.cROIWidth = pDecoder->WMP.wmiI.cThumbnailWidth;
		pDecoder->WMP.wmiI.cROIHeight = pDecoder->WMP.wmiI.cThumbnailHeight;
		pDecoder->WMP.wmiI.oOrientation = O_NONE;
		pDecoder->WMP.wmiI.cPostProcStrength = 0;
		pDecoder->WMP.wmiSCP.bVerbose = FALSE;

		int width = 0;
		int height = 0;
		pDecoder->GetSize( pDecoder, &width, &height );
	
		*dic = TJSCreateDictionaryObject();
		tTJSVariant val(width);
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("width"), 0, &val, (*dic) );
		val = tTJSVariant(height);
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("height"), 0, &val, (*dic));
		val = tTJSVariant((tjs_int64)(pDecoder->WMP.wmiSCP.uAlphaMode ? 32 : 24));
		(*dic)->PropSet(TJS_MEMBERENSURE, TJS_W("bpp"), 0, &val, (*dic));

		if( pDecoder ) pDecoder->Release(&pDecoder);
		if( pStream ) free( pStream );
	} catch(...) {
		if( pDecoder ) pDecoder->Release(&pDecoder);
		if( pStream ) free( pStream );
		throw;
	}
}
#endif
