

#ifndef BitmapIntfH
#define BitmapIntfH

#include "tjsNative.h"
class tTVPBitmap;
class tTVPBaseBitmap;
class tTJSNI_Bitmap : public tTJSNativeInstance
{
	typedef tTJSNativeInstance inherited;

protected:
	iTJSDispatch2 *Owner;
	tTVPBaseBitmap* Bitmap;
	bool Loading;

public:
	tTJSNI_Bitmap();
	virtual ~tTJSNI_Bitmap();
	tjs_error TJS_INTF_METHOD
		Construct(tjs_int numparams, tTJSVariant **param,
			iTJSDispatch2 *tjs_obj);
	void TJS_INTF_METHOD Invalidate();

public:
	tTVPBaseBitmap* GetBitmap() { return Bitmap; }
	const tTVPBaseBitmap* GetBitmap() const { return Bitmap; }

	tjs_uint32 GetPixel(tjs_int x, tjs_int y) const;
	void SetPixel(tjs_int x, tjs_int y, tjs_uint32 color);

	tjs_int GetMaskPixel(tjs_int x, tjs_int y) const;
	void SetMaskPixel(tjs_int x, tjs_int y, tjs_int mask);

	void Independ(bool copy = true);

	iTJSDispatch2* Load(const ttstr &name, tjs_uint32 colorkey);
	void LoadAsync(const ttstr &name);
	void Save(const ttstr &name, const ttstr &type, iTJSDispatch2* meta = NULL);

	void SetSize(tjs_uint width, tjs_uint height, bool keepimage = true);
	// for async load
	// @param bits : tTVPBitmapBitsAlloc::Alloc‚ÅŠm•Û‚µ‚½‚à‚Ì‚ðŽg—p‚·‚é‚±‚Æ
	void SetSizeAndImageBuffer(tTVPBitmap* bmp);

	void SetWidth(tjs_uint width);
	tjs_uint GetWidth() const;
	void SetHeight(tjs_uint height);
	tjs_uint GetHeight() const;

	const void * GetPixelBuffer() const;
	void * GetPixelBufferForWrite();
	tjs_int GetPixelBufferPitch() const;

	// copy on wirte 
	void CopyFrom( const tTJSNI_Bitmap* src );

	bool IsLoading() const { return Loading; }

	// for internal
	void CopyFrom( const class iTVPBaseBitmap* src );
	virtual void SetLoading( bool load ) { Loading = load; }
};


class tTJSNC_Bitmap : public tTJSNativeClass
{
	typedef tTJSNativeClass inherited;

public:
	tTJSNC_Bitmap();
	static tjs_uint32 ClassID;

protected:
	tTJSNativeInstance *CreateNativeInstance();
};

extern tTJSNativeClass * TVPCreateNativeClass_Bitmap();
extern iTJSDispatch2 * TVPCreateBitmapObject();
#endif // BitmapIntfH
