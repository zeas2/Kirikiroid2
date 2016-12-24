//---------------------------------------------------------------------------
/*
	TVP2 ( T Visual Presenter 2 )  A script authoring tool
	Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

	See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Base Layer Bitmap implementation
//---------------------------------------------------------------------------
#ifndef LayerBitmapIntfH
#define LayerBitmapIntfH


#include "ComplexRect.h"
#include "tvpgl.h"
#include "argb.h"
#include "drawable.h"

#ifndef TVP_REVRGB
#define TVP_REVRGB(v) ((v & 0xFF00FF00) | ((v >> 16) & 0xFF) | ((v & 0xFF) << 16))
#endif

/*[*/
//---------------------------------------------------------------------------
// tTVPBBBltMethod and tTVPBBStretchType
//---------------------------------------------------------------------------
enum tTVPBBBltMethod
{
	bmCopy,
	bmCopyOnAlpha,
	bmAlpha,
	bmAlphaOnAlpha,
	bmAdd,
	bmSub,
	bmMul,
	bmDodge,
	bmDarken,
	bmLighten,
	bmScreen,
	bmAddAlpha,
	bmAddAlphaOnAddAlpha,
	bmAddAlphaOnAlpha,
	bmAlphaOnAddAlpha,
	bmCopyOnAddAlpha,
	bmPsNormal,
	bmPsAdditive,
	bmPsSubtractive,
	bmPsMultiplicative,
	bmPsScreen,
	bmPsOverlay,
	bmPsHardLight,
	bmPsSoftLight,
	bmPsColorDodge,
	bmPsColorDodge5,
	bmPsColorBurn,
	bmPsLighten,
	bmPsDarken,
	bmPsDifference,
	bmPsDifference5,
	bmPsExclusion
};

enum tTVPBBStretchType
{
	stNearest = 0, // primal method; nearest neighbor method
	stFastLinear = 1, // fast linear interpolation (does not have so much precision)
	stLinear = 2,  // (strict) linear interpolation
	stCubic = 3,    // cubic interpolation
	stSemiFastLinear = 4,
	stFastCubic = 5,
	stLanczos2 = 6,    // Lanczos 2 interpolation
	stFastLanczos2 = 7,
	stLanczos3 = 8,    // Lanczos 3 interpolation
	stFastLanczos3 = 9,
	stSpline16 = 10,	// Spline16 interpolation
	stFastSpline16 = 11,
	stSpline36 = 12,	// Spline36 interpolation
	stFastSpline36 = 13,
	stAreaAvg = 14,	// Area average interpolation
	stFastAreaAvg = 15,
	stGaussian = 16,
	stFastGaussian = 17,
	stBlackmanSinc = 18,
	stFastBlackmanSinc = 19,

	stTypeMask = 0x0000ffff, // stretch type mask
	stFlagMask = 0xffff0000, // flag mask

	stRefNoClip = 0x10000 // referencing source is not limited by the given rectangle
						// (may allow to see the border pixel to interpolate)
};
/*]*/



//---------------------------------------------------------------------------
// FIXME: for including order problem
//---------------------------------------------------------------------------
#include "LayerBitmapImpl.h"
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// t2DAffineMatrix
//---------------------------------------------------------------------------
struct t2DAffineMatrix
{
	/* structure for subscribing following 2D affine transformation matrix */
	/*
	|                          | a  b  0 |
	| [x', y', 1] =  [x, y, 1] | c  d  0 |
	|                          | tx ty 1 |
	|  thus,
	|
	|  x' =  ax + cy + tx
	|  y' =  bx + dy + ty
	*/

	double a;
	double b;
	double c;
	double d;
	double tx;
	double ty;
};
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
#define TVP_BB_COPY_MAIN 1
#define TVP_BB_COPY_MASK 2
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
extern tTVPGLGammaAdjustData TVPIntactGammaAdjustData;
extern tjs_int TVPDrawThreadNum;
extern tjs_int TVPGetProcessorNum(void);
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
// iTVPBaseBitmap
//---------------------------------------------------------------------------
class tTVPNativeBaseBitmap;
class iTVPBaseBitmap : public tTVPNativeBaseBitmap
{
public:

	//void operator =(const iTVPBaseBitmap & rhs) { Assign(rhs); }

	// metrics
	void SetSizeWithFill(tjs_uint w, tjs_uint h, tjs_uint32 fillvalue);

	// point access
	tjs_uint32 GetPoint(tjs_int x, tjs_int y) const;
	bool SetPoint(tjs_int x, tjs_int y, tjs_uint32 value);
	bool SetPointMain(tjs_int x, tjs_int y, tjs_uint32 color); // for 32bpp
	bool SetPointMask(tjs_int x, tjs_int y, tjs_int mask); // for 32bpp

	// drawing stuff
	virtual bool Fill(tTVPRect rect, tjs_uint32 value);

	bool FillColor(tTVPRect rect, tjs_uint32 color, tjs_int opa);

private:
	bool BlendColor(tTVPRect rect, tjs_uint32 color, tjs_int opa, bool additive);
#if 0
        struct PartialFillParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int x;
          tjs_int y;
          tjs_int w;
          tjs_int h;
          tjs_int pitch;
          tjs_uint32 value;
          bool is32bpp;
        };
        static void TJS_USERENTRY PartialFillEntry(void *param);
        void PartialFill(const PartialFillParam *param);

        struct PartialFillColorParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int x;
          tjs_int y;
          tjs_int w;
          tjs_int h;
          tjs_int pitch;
          tjs_uint32 color;
          tjs_int opa;
        };
        static void TJS_USERENTRY PartialFillColorEntry(void *param);
        void PartialFillColor(const PartialFillColorParam *param);

        struct PartialBlendColorParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int x;
          tjs_int y;
          tjs_int w;
          tjs_int h;
          tjs_int pitch;
          tjs_uint32 color;
          tjs_int opa;
          bool additive;
        };
        static void TJS_USERENTRY PartialBlendColorEntry(void *param);
        void PartialBlendColor(const PartialBlendColorParam *param);

        struct PartialRemoveConstOpacityParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int x;
          tjs_int y;
          tjs_int w;
          tjs_int h;
          tjs_int pitch;
          tjs_int level;
        };
        static void TJS_USERENTRY PartialRemoveConstOpacityEntry(void *param);
        void PartialRemoveConstOpacity(const PartialRemoveConstOpacityParam *param);
  
        struct PartialFillMaskParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int x;
          tjs_int y;
          tjs_int w;
          tjs_int h;
          tjs_int pitch;
          tjs_int value;
        };
        static void TJS_USERENTRY PartialFillMaskEntry(void *param);
        void PartialFillMask(const PartialFillMaskParam *param);

        struct PartialCopyRectParam {
          iTVPBaseBitmap *self;
          tjs_int pixelsize;
          tjs_uint8 *dest;
          tjs_int dpitch;
          tjs_int dx;
          tjs_int dy;
          tjs_int w;
          tjs_int h;
          const tjs_int8 *src;
          tjs_int spitch;
          tjs_int sx;
          tjs_int sy;
          tjs_int plane;
          bool backwardCopy;
        };
        static void TJS_USERENTRY PartialCopyRectEntry(void *param);
        void PartialCopyRect(const PartialCopyRectParam *param);

        struct PartialBltParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int dpitch;
          tjs_int dx;
          tjs_int dy;
          tjs_int w;
          tjs_int h;
          const tjs_int8 *src;
          tjs_int spitch;
          tjs_int sx;
          tjs_int sy;
          tTVPBBBltMethod method;
          tjs_int opa;
          bool hda;
        };
        static void TJS_USERENTRY PartialBltEntry(void *param);
        void PartialBlt(const PartialBltParam *param);
#endif
public:
	bool FillColorOnAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa)
	{
		return BlendColor(rect, color, opa, false);
	}

	bool FillColorOnAddAlpha(tTVPRect rect, tjs_uint32 color, tjs_int opa)
	{
		return BlendColor(rect, color, opa, true);
	}

	bool RemoveConstOpacity(tTVPRect rect, tjs_int level);

	bool FillMask(tTVPRect rect, tjs_int value);

	virtual bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane = (TVP_BB_COPY_MAIN|TVP_BB_COPY_MASK));

    /**
     * @param ref : コピー元画像(9patch形式)
     * @param margin : 9patchの右下にある描画領域指定を取得する
     */
    bool Copy9Patch( const iTVPBaseBitmap *ref, tTVPRect& margin );

	bool Blt(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa,
			bool hda = true);
	bool Blt(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
		const tTVPRect &refrect, tTVPLayerType type, tjs_int opa, bool hda = true);

#if 0
private:
	template <typename tFunc>
	static void TVPDoStretchLoop(tFunc func,
		tjs_int x_ref_start,
		tjs_int y_ref_start,
		tjs_int x_len, tjs_int y_len,
		tjs_uint8 * destp, tjs_int destpitch,
		tjs_int x_step, tjs_int y_step,
		const tjs_uint8 * refp, tjs_int refpitch);

	template <typename tStretchFunc>
	static void TVPDoBiLinearStretchLoop(
			tStretchFunc stretch,
			tjs_int rw, tjs_int rh,
			tjs_int dw, tjs_int dh,
			const tTVPRect & srccliprect,
			tjs_int x_ref_start,
			tjs_int y_ref_start,
			tjs_int x_len, tjs_int y_len,
			tjs_uint8 * destp, tjs_int destpitch,
			tjs_int x_step, tjs_int y_step,
			const tjs_uint8 * refp, tjs_int refpitch);
#endif
public:
	bool StretchBlt(tTVPRect cliprect, tTVPRect destrect, const iTVPBaseBitmap *ref,
		tTVPRect refrect, tTVPBBBltMethod method, tjs_int opa,
			bool hda = true, tTVPBBStretchType type = stNearest, tjs_real typeopt=0.0);
#if 0
private:
	template <typename tFuncStretch, typename tFuncAffine>
	static void TVPDoAffineLoop(
			tFuncStretch stretch,
			tFuncAffine affine,
			tjs_int sxs,
			tjs_int sys,
			tjs_uint8 *dest,
			tjs_int l,
			tjs_int len,
			const tjs_uint8 *src,
			tjs_int srcpitch,
			tjs_int sxl,
			tjs_int syl,
			const tTVPRect & srcrect);

	template <typename tFuncStretch, typename tFuncAffine>
	static void TVPDoBilinearAffineLoop(
			tFuncStretch stretch,
			tFuncAffine affine,
			tjs_int sxs,
			tjs_int sys,
			tjs_uint8 *dest,
			tjs_int l,
			tjs_int len,
			const tjs_uint8 *src,
			tjs_int srcpitch,
			tjs_int sxl,
			tjs_int syl,
			const tTVPRect & srccliprect,
			const tTVPRect & srcrect);

        struct PartialAffineBltParam {
          iTVPBaseBitmap *self;
          tjs_uint8 *dest;
          tjs_int destpitch;
          tjs_int yc;
          tjs_int yclim;
          tjs_int scanlinestart;
          tjs_int scanlineend;
          tjs_int *points_x;
          tjs_int *points_y;
          tTVPRect *refrect;
          tjs_int sxstep;
          tjs_int systep;
          tTVPRect *destrect;
          tTVPBBBltMethod method;
          tjs_int opa;
          bool hda;
          tTVPBBStretchType type;
          bool clear;
          tjs_uint32 clearcolor;
          const tjs_uint8 *src;
          tjs_int srcpitch;
          tTVPRect *srccliprect;
          tTVPRect *srcrect;
          // below variable returns value to main thread.
          tjs_int leftlimit;
          tjs_int rightlimit;
          tjs_int mostupper;
          tjs_int mostbottom;
          bool firstline;
        };
        static void TJS_USERENTRY PartialAffineBltEntry(void *param);
        void PartialAffineBlt(PartialAffineBltParam *param);

	int InternalAffineBlt(tTVPRect destrect, const iTVPBaseBitmap *ref,
		tTVPRect refrect, const tTVPPointD * points,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect = NULL,
			bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
				tjs_uint32 clearcolor = 0);
#endif
public:
	bool AffineBlt(tTVPRect destrect, const iTVPBaseBitmap *ref,
		tTVPRect refrect, const tTVPPointD * points,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect = NULL,
			bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
				tjs_uint32 clearcolor = 0);

	bool AffineBlt(tTVPRect destrect, const iTVPBaseBitmap *ref,
		tTVPRect refrect, const t2DAffineMatrix & matrix,
			tTVPBBBltMethod method, tjs_int opa,
			tTVPRect * updaterect = NULL,
			bool hda = true, tTVPBBStretchType mode = stNearest, bool clear = false,
				tjs_uint32 clearcolor = 0);

private:
	bool InternalDoBoxBlur(tTVPRect rect, tTVPRect area, bool hasalpha);

public:
	bool DoBoxBlur(const tTVPRect & rect, const tTVPRect & area);
	bool DoBoxBlurForAlpha(const tTVPRect & rect, const tTVPRect & area);

	void UDFlip(const tTVPRect &rect);
	void LRFlip(const tTVPRect &rect);

	void DoGrayScale(tTVPRect rect);

	void AdjustGamma(tTVPRect rect, const tTVPGLGammaAdjustData & data);
	void AdjustGammaForAdditiveAlpha(tTVPRect rect, const tTVPGLGammaAdjustData & data);


	void ConvertAddAlphaToAlpha();
	void ConvertAlphaToAddAlpha();

	// font and text related functions are implemented in each platform.
};
//---------------------------------------------------------------------------
class iTVPRenderManager;
class tTVPBaseBitmap : public iTVPBaseBitmap // for ProvinceImage
{
public:
	tTVPBaseBitmap(tjs_uint w, tjs_uint h, tjs_uint bpp = 32);
	tTVPBaseBitmap(const iTVPBaseBitmap& r) : iTVPBaseBitmap(r) {}
	virtual bool AssignBitmap(tTVPBitmap *bmp);
	virtual iTVPRenderManager* GetRenderManager();
	bool Fill(tTVPRect rect, tjs_uint32 value) override;
	virtual bool CopyRect(tjs_int x, tjs_int y, const iTVPBaseBitmap *ref,
		tTVPRect refrect, tjs_int plane = (TVP_BB_COPY_MAIN | TVP_BB_COPY_MASK));
	void UDFlip(const tTVPRect &rect);
	void LRFlip(const tTVPRect &rect);
};
//---------------------------------------------------------------------------
class tTVPBaseTexture : public iTVPBaseBitmap
{
public:
	tTVPBaseTexture(tjs_uint w, tjs_uint h, tjs_uint bpp = 32);
	tTVPBaseTexture(const iTVPBaseBitmap& r) : iTVPBaseBitmap(r) {}
	virtual bool AssignBitmap(tTVPBitmap *bmp);
	virtual iTVPRenderManager* GetRenderManager();
	void Update(const void *pixel, unsigned int pitch, int x, int y, int w, int h);
};
#endif
