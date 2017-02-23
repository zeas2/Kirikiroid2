#include "tvpgl_arm_intf.h"
#include "tvpgl_asm_init.h"
#include <arm_neon.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>

//#define TEST_ARM_NEON_CODE
//#define DEBUG_ARM_NEON
//#define LOG_NEON_TEST

#ifdef __cplusplus
#if defined(_MSC_VER)
#include <opencv2/core/mat.hpp>
#endif
#include <vector>
extern "C" {
#endif
extern unsigned char TVPNegativeMulTable[256*256];
extern unsigned char TVPOpacityOnOpacityTable[256*256];
extern unsigned short TVPRecipTableForOpacityOnOpacity[256];
#ifdef __cplusplus
};
#endif

#define __CAT_NAME(a, b) a##b
#define _CAT_NAME(a, b) __CAT_NAME(a, b)

template <int elmcount, typename TElmDst, typename TElmSrc, typename TDst, typename TSrc,
	typename LoadFuncS, typename LoadFuncD, typename StoreFunc, typename CFunc, typename OPFunc, typename ...TArg>
void do_blend_(LoadFuncS lfuncs, LoadFuncD lfuncd, StoreFunc sfunc, CFunc c_func, OPFunc op_func, TDst *dest, const TSrc *src, tjs_int len, TArg... args) {
	TDst* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & (elmcount - 1))) & (elmcount - 1)) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, src, PreFragLen, args...);
			dest += PreFragLen;
			src += PreFragLen;
		}
	}

	TDst* pVecEndDst = pEndDst - elmcount + 1;
	if ((intptr_t)src & (elmcount - 1)) {
		while (dest < pVecEndDst) {
			TElmSrc s = lfuncs((uint8_t *)src);
			TElmDst d = lfuncd((uint8_t *)__builtin_assume_aligned(dest, elmcount));
			d = op_func(s, d, args...);
			sfunc((uint8_t *)__builtin_assume_aligned(dest, elmcount), d);
			dest += elmcount;
			src += elmcount;
		}
	} else {
		while (dest < pVecEndDst) {
			TElmSrc s = lfuncs((uint8_t *)__builtin_assume_aligned(src, elmcount));
			TElmDst d = lfuncd((uint8_t *)__builtin_assume_aligned(dest, elmcount));
			d = op_func(s, d, args...);
			sfunc((uint8_t *)__builtin_assume_aligned(dest, elmcount), d);
			dest += elmcount;
			src += elmcount;
		}
	}

	if (dest < pEndDst) {
		c_func(dest, src, pEndDst - dest, args...);
	}
}

static uint8x8_t __vld1_u8(uint8_t* p) { return vld1_u8(p); }
static uint8x8x4_t __vld4_u8(uint8_t* p) { return vld4_u8(p); }
static void __vst4_u8(uint8_t* p, uint8x8x4_t v) { return vst4_u8(p, v); }
template <typename CFunc, typename OPFunc, typename ...TArg>
void do_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, TArg... args) {
	do_blend_<8, uint8x8x4_t, uint8x8x4_t>(__vld4_u8, __vld4_u8, __vst4_u8, c_func, op_func, dest, src, len, args...);
}
static uint8x16x4_t __vld4q_u8(uint8_t* p) {
	__builtin_prefetch(p, 0, 0);
	return vld4q_u8(p);
}
static void __vst4q_u8(uint8_t* p, uint8x16x4_t v) { return vst4q_u8(p, v); }
template <typename CFunc, typename OPFunc, typename ...TArg>
void do_blend_128(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, TArg... args) {
	do_blend_<16, uint8x16x4_t, uint8x16x4_t>(__vld4q_u8, __vld4q_u8, __vst4q_u8, c_func, op_func, dest, src, len, args...);
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_blend_lum(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, TArg... args) {
	do_blend_<8, uint8x8x4_t, uint8x8_t>(__vld1_u8, __vld4_u8, __vst4_u8, c_func, op_func, dest, src, len, args...);
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_stretch_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len,
	const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, PreFragLen, src, srcstart, srcstep, args...);
			dest += PreFragLen;
			srcstart += PreFragLen * srcstep;
		}
	}
	uint8_t strechbuff[32 + 16];
	tjs_uint32 *tmp = (tjs_uint32*)((((intptr_t)strechbuff) + 7) & ~7);
	tjs_uint32* pVecEndDst = pEndDst - 7;
	while (dest < pVecEndDst) {
		for (int i = 0; i < 8; ++i) {
			tmp[i] = src[(srcstart) >> 16];
			srcstart += srcstep;
		}
		uint8x8x4_t s = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp, 8));
		uint8x8x4_t d = vld4_u8((uint8_t *)__builtin_assume_aligned(dest, 8));
		d = op_func(s, d, args...);
		vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
		dest += 8;
	}
	if (dest < pEndDst) {
		c_func(dest, pEndDst - dest, src, srcstart, srcstep, args...);
	}
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_lintrans_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len,
	const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, PreFragLen, src, sx, sy, stepx, stepy, srcpitch, args...);
			dest += PreFragLen;
			sx += stepx * PreFragLen;
			sy += stepy * PreFragLen;
		}
	}
	uint8_t strechbuff[32 + 16];
	tjs_uint32 *tmp = (tjs_uint32*)((((intptr_t)strechbuff) + 7) & ~7);
	tjs_uint32* pVecEndDst = pEndDst - 7;
	while (dest < pVecEndDst) {
		for (int i = 0; i < 8; ++i) {
			tmp[i] = *((const tjs_uint32*)((const tjs_uint8*)src + ((sy) >> 16)*srcpitch) + ((sx) >> 16));
			sx += stepx;
			sy += stepy;
		}
		uint8x8x4_t s = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp, 8));
		uint8x8x4_t d = vld4_u8((uint8_t *)__builtin_assume_aligned(dest, 8));
		d = op_func(s, d, args...);
		vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
		dest += 8;
	}
	if (dest < pEndDst) {
		c_func(dest, pEndDst - dest, src, sx, sy, stepx, stepy, srcpitch, args...);
	}
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_interp_stretch_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len,
	const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, PreFragLen, src1, src2, _blend_y, srcstart, srcstep, args...);
			dest += PreFragLen;
			srcstart += PreFragLen * srcstep;
		}
	}
	tjs_int blend_y = _blend_y + (_blend_y >> 7); /* adjust blend ratio */

	uint8_t tmpbuff[4 * 8 * 3 + 16];
	tjs_uint32 *tmp1_0 = (tjs_uint32*)((((intptr_t)tmpbuff) + 15) & ~15);
	tjs_uint32 *tmp1_1 = tmp1_0 + 8;
	uint16_t *blend_x = (uint16_t *)__builtin_assume_aligned(tmp1_1 + 8, 8);
	tjs_uint32* pVecEndDst = pEndDst - 7;
	while (dest < pVecEndDst) {
		tjs_int start = srcstart;
		for (int i = 0; i < 8; ++i) {
			int addr = start >> 16;
			tmp1_0[i] = src2[addr];
			tmp1_1[i] = src2[addr + 1];
			blend_x[i] = (start & 0xffff) >> 8;
			start += srcstep;
		}
		// TVPBlendARGB(src2[sp], src2[sp+1], blend_x)
		uint8x8x4_t b = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_0, 8));
		uint8x8x4_t a = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_1, 8));
		uint16x8_t ratio = vld1q_u16(blend_x); // qreg = 5
		// TVPBlendARGB: a * ratio + b * (1 - ratio) => b + (a - b) * ratio
		uint16x8_t s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
		uint16x8_t s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
		uint16x8_t s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
		uint16x8_t s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio); // qreg = 9

		uint8x8x4_t s_argb8;
		s_argb8.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
		s_argb8.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
		s_argb8.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
		s_argb8.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 11

		start = srcstart;
		for (int i = 0; i < 8; ++i) {
			int addr = (start) >> 16;
			tmp1_0[i] = src1[addr];
			tmp1_1[i] = src1[addr + 1];
			start += srcstep;
		}
		// TVPBlendARGB(src1[sp], src1[sp+1], blend_x)
		b = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_0, 8));
		a = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_1, 8));
		s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
		s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
		s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
		s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio);
		uint8x8x4_t s2;
		s2.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
		s2.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
		s2.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
		s2.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 13
		s_a16 = vmulq_n_u16(vsubl_u8(s_argb8.val[3], s2.val[3]), blend_y);
		s_r16 = vmulq_n_u16(vsubl_u8(s_argb8.val[2], s2.val[2]), blend_y);
		s_g16 = vmulq_n_u16(vsubl_u8(s_argb8.val[1], s2.val[1]), blend_y);
		s_b16 = vmulq_n_u16(vsubl_u8(s_argb8.val[0], s2.val[0]), blend_y);
		s_argb8.val[3] = vadd_u8(s2.val[3], vshrn_n_u16(s_a16, 8));
		s_argb8.val[2] = vadd_u8(s2.val[2], vshrn_n_u16(s_r16, 8));
		s_argb8.val[1] = vadd_u8(s2.val[1], vshrn_n_u16(s_g16, 8));
		s_argb8.val[0] = vadd_u8(s2.val[0], vshrn_n_u16(s_b16, 8));
		uint8x8x4_t d_argb8 = vld4_u8((uint8_t *)__builtin_assume_aligned(dest, 8));
		d_argb8 = op_func(s_argb8, d_argb8, args...);
		vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d_argb8);
		srcstart = start;
		dest += 8;
	}
	if (dest < pEndDst) {
		c_func(dest, pEndDst - dest, src1, src2, _blend_y, srcstart, srcstep, args...);
	}
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_interp_lintrans_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len,
	const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, PreFragLen, src, sx, sy, stepx, stepy, srcpitch, args...);
			dest += PreFragLen;
			sx += stepx * PreFragLen;
			sy += stepy * PreFragLen;
		}
	}
	uint8_t tmpbuff[4 * 8 * 4 + 2 * 8 * 2 + 16];
	tjs_uint32 *tmp0_0 = (tjs_uint32*)((((intptr_t)tmpbuff) + 15) & ~15);
	tjs_uint32 *tmp0_1 = tmp0_0 + 8;
	tjs_uint32 *tmp1_0 = tmp0_1 + 8;
	tjs_uint32 *tmp1_1 = tmp1_0 + 8;
	uint16_t *blend_x = (uint16_t *)__builtin_assume_aligned(tmp1_1 + 8, 8);
	uint16_t *blend_y = (uint16_t *)__builtin_assume_aligned(blend_x + 8, 8);
	tjs_uint32* pVecEndDst = pEndDst - 7;
	while (dest < pVecEndDst) {
		for (int i = 0; i < 8; ++i) {
			const tjs_uint32 *p0, *p1;
			int bld_x, bld_y;
			bld_x = (sx & 0xffff) >> 8;
			bld_x += bld_x >> 7;
			bld_y = (sy & 0xffff) >> 8;
			bld_y += bld_y >> 7;
			blend_x[i] = bld_x;
			blend_y[i] = bld_y;

			p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy >> 16))*srcpitch) + (sx >> 16);
			p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);

			tmp0_0[i] = p0[0];
			tmp0_1[i] = p0[1];
			tmp1_0[i] = p1[0];
			tmp1_1[i] = p1[1];

			sx += stepx;
			sy += stepy;
		}
		// TVPBlendARGB(src2[sp], src2[sp+1], blend_x)
		uint8x8x4_t b = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_0, 8));
		uint8x8x4_t a = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp1_1, 8));
		uint16x8_t ratio = vld1q_u16(blend_x); // qreg = 5
		// TVPBlendARGB: a * ratio + b * (1 - ratio) => b + (a - b) * ratio
		uint16x8_t s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
		uint16x8_t s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
		uint16x8_t s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
		uint16x8_t s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio); // qreg = 9

		uint8x8x4_t s_argb8;
		s_argb8.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
		s_argb8.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
		s_argb8.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
		s_argb8.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 11

		b = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp0_0, 8));
		a = vld4_u8((uint8_t *)__builtin_assume_aligned(tmp0_1, 8));
		s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
		s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
		s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
		s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio);
		uint8x8x4_t s2;
		s2.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
		s2.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
		s2.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
		s2.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 13

		ratio = vld1q_u16(blend_y);
		s_a16 = vmulq_u16(vsubl_u8(s_argb8.val[3], s2.val[3]), ratio);
		s_r16 = vmulq_u16(vsubl_u8(s_argb8.val[2], s2.val[2]), ratio);
		s_g16 = vmulq_u16(vsubl_u8(s_argb8.val[1], s2.val[1]), ratio);
		s_b16 = vmulq_u16(vsubl_u8(s_argb8.val[0], s2.val[0]), ratio);
		s_argb8.val[3] = vadd_u8(s2.val[3], vshrn_n_u16(s_a16, 8));
		s_argb8.val[2] = vadd_u8(s2.val[2], vshrn_n_u16(s_r16, 8));
		s_argb8.val[1] = vadd_u8(s2.val[1], vshrn_n_u16(s_g16, 8));
		s_argb8.val[0] = vadd_u8(s2.val[0], vshrn_n_u16(s_b16, 8));
		uint8x8x4_t d_argb8 = vld4_u8((uint8_t *)__builtin_assume_aligned(dest, 8));
		d_argb8 = op_func(s_argb8, d_argb8, args...);
		vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d_argb8);
		dest += 8;
	}
	if (dest < pEndDst) {
		c_func(dest, pEndDst - dest, src, sx, sy, stepx, stepy, srcpitch, args...);
	}
}

template <typename TElm, int elmcount, typename LoadFunc, typename StoreFunc, typename CFunc, typename OPFunc, typename ...TArg>
void do_apply_pixel_(LoadFunc lfunc, StoreFunc sfunc, CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, PreFragLen, args...);
			dest += PreFragLen;
		}
	}
	tjs_uint32* pVecEndDst = pEndDst - elmcount + 1;
	while (dest < pVecEndDst) {
		TElm d = lfunc((uint8_t *)__builtin_assume_aligned(dest, 8));
		d = op_func(d, args...);
		sfunc((uint8_t *)__builtin_assume_aligned(dest, 8), d);
		dest += 8;
	}

	if (dest < pEndDst) {
		c_func(dest, pEndDst - dest, args...);
	}
}
template <typename CFunc, typename OPFunc, typename ...TArg>
void do_apply_pixel(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, tjs_int len, TArg... args) {
	do_apply_pixel_<uint8x8x4_t, 8>(__vld4_u8, __vst4_u8, c_func, op_func, dest, len, args...);
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_blend_2(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	{
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
		if (PreFragLen > len) PreFragLen = len;
		if (PreFragLen) {
			c_func(dest, src1, src2, PreFragLen, args...);
			dest += PreFragLen;
			src1 += PreFragLen;
			src2 += PreFragLen;
		}
	}

	tjs_uint32* pVecEndDst = pEndDst - 7;
	if (((intptr_t)src1 & 7) || ((intptr_t)src2 & 7)) {
		while (dest < pVecEndDst) {
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 4));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 4));
			uint8x8x4_t d = op_func(s2, s1, args...);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 8;
			src1 += 8;
			src2 += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 8));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 8));
			uint8x8x4_t d = op_func(s2, s1, args...);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 8;
			src1 += 8;
			src2 += 8;
		}
	}

	if (dest < pEndDst) {
		c_func(dest, src1, src2, pEndDst - dest, args...);
	}
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_univ_blend(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
	if (PreFragLen > len) PreFragLen = len;
	if (PreFragLen) {
		c_func(dest, src1, src2, rule, table, PreFragLen, args...);
		dest += PreFragLen;
		src1 += PreFragLen;
		src2 += PreFragLen;
		rule += PreFragLen;
	}
	tjs_uint32* pVecEndDst = pEndDst - 7;
	if (((intptr_t)src1 & 7) || ((intptr_t)src2 & 7)) {
		while (dest < pVecEndDst) {
			uint8x8_t opa;
			opa = vset_lane_u8(table[*rule++], opa, 0);
			opa = vset_lane_u8(table[*rule++], opa, 1);
			opa = vset_lane_u8(table[*rule++], opa, 2);
			opa = vset_lane_u8(table[*rule++], opa, 3);
			opa = vset_lane_u8(table[*rule++], opa, 4);
			opa = vset_lane_u8(table[*rule++], opa, 5);
			opa = vset_lane_u8(table[*rule++], opa, 6);
			opa = vset_lane_u8(table[*rule++], opa, 7);
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 4));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 4));
			uint8x8x4_t d = op_func(s2, s1, opa);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			src1 += 8;
			src2 += 8;
			dest += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x8_t opa;
			opa = vset_lane_u8(table[*rule++], opa, 0);
			opa = vset_lane_u8(table[*rule++], opa, 1);
			opa = vset_lane_u8(table[*rule++], opa, 2);
			opa = vset_lane_u8(table[*rule++], opa, 3);
			opa = vset_lane_u8(table[*rule++], opa, 4);
			opa = vset_lane_u8(table[*rule++], opa, 5);
			opa = vset_lane_u8(table[*rule++], opa, 6);
			opa = vset_lane_u8(table[*rule++], opa, 7);
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 8));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 8));
			uint8x8x4_t d = op_func(s2, s1, opa);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			src1 += 8;
			src2 += 8;
			dest += 8;
		}
	}
	if (dest < pEndDst) {
		c_func(dest, src1, src2, rule, table, pEndDst - dest);
	}
}

template <typename CFunc, typename OPFunc, typename ...TArg>
void do_univ_switch(CFunc c_func, OPFunc op_func, tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv, TArg... args) {
	tjs_uint32* pEndDst = dest + len;
	tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
	if (PreFragLen > len) PreFragLen = len;
	if (PreFragLen) {
		c_func(dest, src1, src2, rule, table, PreFragLen, src1lv, src2lv, args...);
		dest += PreFragLen;
		src1 += PreFragLen;
		src2 += PreFragLen;
		rule += PreFragLen;
	}
	tjs_uint32* pVecEndDst = pEndDst - 7;
	if (((intptr_t)src1 & 7) || ((intptr_t)src2 & 7)) {
		while (dest < pVecEndDst) {
			uint8x8_t opa; tjs_int o;
#define SET_LANE(i) \
	o = *rule++; if (o >= src1lv) { o = 0; } else if (o < src2lv) { o = 255; } else { o = table[o]; } opa = vset_lane_u8(o, opa, i)
			SET_LANE(0); SET_LANE(1); SET_LANE(2); SET_LANE(3); SET_LANE(4); SET_LANE(5); SET_LANE(6); SET_LANE(7);
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 4));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 4));
			uint8x8x4_t d = op_func(s2, s1, opa);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			src1 += 8;
			src2 += 8;
			dest += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x8_t opa; tjs_int o;
			SET_LANE(0); SET_LANE(1); SET_LANE(2); SET_LANE(3); SET_LANE(4); SET_LANE(5); SET_LANE(6); SET_LANE(7);
#undef SET_LANE
			uint8x8x4_t s1 = vld4_u8((uint8_t *)__builtin_assume_aligned(src1, 8));
			uint8x8x4_t s2 = vld4_u8((uint8_t *)__builtin_assume_aligned(src2, 8));
			uint8x8x4_t d = op_func(s2, s1, opa);
			vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			src1 += 8;
			src2 += 8;
			dest += 8;
		}
	}
	if (dest < pEndDst) {
		c_func(dest, src1, src2, rule, table, pEndDst - dest, src1lv, src2lv, args...);
	}
}

template<typename TFunc, typename ...TArg>
uint8x8x4_t do_SrcAlphaBranch(uint8x8x4_t s, uint8x8x4_t d, TFunc func, TArg... args) {
	uint64_t a = vget_lane_u64(vreinterpret_u64_u8(s.val[3]), 0);
	if (!a) {
		return d;
	} else if (!~a) {
		return s;
	}
	return func(s, d, args...);
}
template<typename TFunc, typename ...TArg>
uint8x8x4_t do_SrcAddAlphaBranch(uint8x8x4_t s, uint8x8x4_t d, TFunc func, TArg... args) {
	uint64_t a = vget_lane_u64(vreinterpret_u64_u8(s.val[3]), 0);
	if (!a) {
		return d;
	}
	return func(s, d, args...);
}
static uint8x8x4_t do_copy_src(uint8x8x4_t s, uint8x8x4_t d) {
	return s;
}

#ifndef Region_AlphaBlend
static uint8x8x4_t do_AlphaBlend(uint8x8x4_t s, uint8x8x4_t d) {
	// d + s * a - d * a
	d.val[0] = vadd_u8(d.val[0], vsubhn_u16(vmull_u8(s.val[0], s.val[3]), vmull_u8(d.val[0], s.val[3])));
	d.val[1] = vadd_u8(d.val[1], vsubhn_u16(vmull_u8(s.val[1], s.val[3]), vmull_u8(d.val[1], s.val[3])));
	d.val[2] = vadd_u8(d.val[2], vsubhn_u16(vmull_u8(s.val[2], s.val[3]), vmull_u8(d.val[2], s.val[3])));
	return d;
}
static uint8x8x4_t do_AlphaBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vshrn_n_u16(vmull_u8(s.val[3], vdup_n_u8(opa)), 8);
	return do_AlphaBlend(s, d);
}
static uint8x8x4_t do_AlphaBlend_d_(uint8x8x4_t s, uint8x8x4_t d, uint16x8_t sopa) {
	uint8_t tmpbuff[32 + 16];
	uint16_t *tmpsa = (uint16_t*)((((intptr_t)tmpbuff) + 15) & ~15);
	vst1q_u16((uint16_t *)__builtin_assume_aligned(tmpsa, 16), sopa);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[0]], s.val[3], 0);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[1]], s.val[3], 1);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[2]], s.val[3], 2);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[3]], s.val[3], 3);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[4]], s.val[3], 4);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[5]], s.val[3], 5);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[6]], s.val[3], 6);
	s.val[3] = vset_lane_u8(TVPOpacityOnOpacityTable[tmpsa[7]], s.val[3], 7);
	return do_AlphaBlend(s, d);
}
static uint8x8x4_t do_AlphaBlend_d(uint8x8x4_t s, uint8x8x4_t d) {
	//( 255 - (255-a)*(255-b)/ 255 ); 
	uint16x8_t isd_a16 = vmull_u8(vmvn_u8(s.val[3]), vmvn_u8(d.val[3]));
	uint16x8_t sopa = vorrq_u16(vshll_n_u8(s.val[3], 8), vmovl_u8(d.val[3]));
	d.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
	return do_AlphaBlend_d_(s, d, sopa);
}
static uint8x8x4_t do_AlphaBlend_do(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t s_a16 = vmull_u8(s.val[3], opa8);
	s.val[3] = vshrn_n_u16(s_a16, 8);
	return do_AlphaBlend_d(s, d);
}
static uint8x8x4_t do_AddAlphaBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[3] = vmvn_u8(s.val[3]);
	// s + d * (1 - sa)
	uint16x8_t d_r16 = vmull_u8(d.val[2], s.val[3]);
	uint16x8_t d_g16 = vmull_u8(d.val[1], s.val[3]);
	uint16x8_t d_b16 = vmull_u8(d.val[0], s.val[3]);

	// 8-bit to do saturated add
	d.val[2] = vqadd_u8(vshrn_n_u16(d_r16, 8), s.val[2]);
	d.val[1] = vqadd_u8(vshrn_n_u16(d_g16, 8), s.val[1]);
	d.val[0] = vqadd_u8(vshrn_n_u16(d_b16, 8), s.val[0]);

	return d;
}
static uint8x8x4_t do_ConvertAlphaToAdditiveAlpha(uint8x8x4_t s) {
	s.val[2] = vshrn_n_u16(vmull_u8(s.val[2], s.val[3]), 8);
	s.val[1] = vshrn_n_u16(vmull_u8(s.val[1], s.val[3]), 8);
	s.val[0] = vshrn_n_u16(vmull_u8(s.val[0], s.val[3]), 8);
	return s;
}
static uint8x8x4_t do_AlphaBlend_da(uint8x8x4_t s, uint8x8x4_t d) {
	//Da = Sa + Da - SaDa = Sa + (1 - Sa)Da
	uint16x8_t a16 = vmull_u8(vmvn_u8(s.val[3]), d.val[3]);
	uint8x8_t tmp = vshrn_n_u16(a16, 8);
	d.val[3] = vadd_u8(s.val[3], tmp);
	return d;
}
static uint8x8x4_t do_AlphaBlend_a(uint8x8x4_t s, uint8x8x4_t d) {
	d = do_AlphaBlend_da(s, d);
	s = do_ConvertAlphaToAdditiveAlpha(s);
	return do_AddAlphaBlend(s, d);
}
static uint8x8x4_t do_AlphaBlend_ao(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t s_a16 = vmull_u8(s.val[3], opa8);
	s.val[3] = vshrn_n_u16(s_a16, 8);
	return do_AlphaBlend_a(s, d);
}
static uint8x8x4_t do_ConstAlphaBlend(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	return do_AlphaBlend(s, d);
}
static uint8x8x4_t do_ConstAlphaBlend_d(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	return do_AlphaBlend_d(s, d);
}
static uint8x8x4_t do_ConstAlphaBlend_a(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	d = do_AlphaBlend_da(s, d);
	return do_AddAlphaBlend(s, d);
}
static uint8x8x4_t do_ConstAlphaBlend_SD(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	d.val[3] = vadd_u8(d.val[3], vsubhn_u16(vmull_u8(s.val[3], vdup_n_u8(opa)), vmull_u8(d.val[3], vdup_n_u8(opa))));
	return do_ConstAlphaBlend(s, d, opa);
}
static uint8x8x4_t do_ConstAlphaBlend_SD_d(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t sa = vmull_u8(s.val[3], opa8);
	uint8x8_t a = vadd_u8(d.val[3], vsubhn_u16(sa, vmull_u8(d.val[3], opa8)));
	s.val[3] = vshrn_n_u16(sa, 8);
	d.val[3] = vshrn_n_u16(vmull_u8(d.val[3], vdup_n_u8(~opa)), 8);
	d = do_AlphaBlend_d(s, d);
	d.val[3] = a;
	return d;
}
static uint8x8x4_t do_ConstAlphaBlend_SD_a(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	d.val[3] = vadd_u8(d.val[3], vsubhn_u16(vmull_u8(s.val[3], vdup_n_u8(opa)), vmull_u8(d.val[3], vdup_n_u8(opa))));
	return do_ConstAlphaBlend(s, d, opa);
}
#endif

static uint8x8x4_t do_AlphaColorMat(uint8x8x4_t s, tjs_uint32 color) {
	uint8x8x4_t d;
	d.val[0] = vdup_n_u8(color & 0xff);
	d.val[1] = vdup_n_u8((color >> 8) & 0xff);
	d.val[2] = vdup_n_u8((color >> 16) & 0xff);
	d.val[3] = vdup_n_u8(0xff);
	return do_AlphaBlend(s, d);
}
static void TVPAlphaColorMat_frag(tjs_uint32 *dest, tjs_int len, const tjs_uint32 color) {
	TVPAlphaColorMat_c(dest, color, len);
}
static void TVPAlphaColorMat_NEON(tjs_uint32 *dest, const tjs_uint32 color, tjs_int len) {
	do_apply_pixel(TVPAlphaColorMat_frag, do_AlphaColorMat, dest, len, color);
}

#ifndef Region_AdditiveAlphaBlend

static uint8x8x4_t do_AddAlphaBlendSrc(uint8x8x4_t s, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t s_a16 = vmull_u8(s.val[3], opa8);
	uint16x8_t s_r16 = vmull_u8(s.val[2], opa8);
	uint16x8_t s_g16 = vmull_u8(s.val[1], opa8);
	uint16x8_t s_b16 = vmull_u8(s.val[0], opa8);
	s.val[3] = vshrn_n_u16(s_a16, 8);
	s.val[2] = vshrn_n_u16(s_r16, 8);
	s.val[1] = vshrn_n_u16(s_g16, 8);
	s.val[0] = vshrn_n_u16(s_b16, 8);
	return s;
}
static uint8x8x4_t do_AddAlphaBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s = do_AddAlphaBlendSrc(s, opa);
	return do_AddAlphaBlend(s, d);
}
static uint8x8x4_t do_AddAlphaBlend_a(uint8x8x4_t s, uint8x8x4_t d) {
	//Da = Sa + Da - SaDa
	uint16x8_t d_a16 = vmull_u8(s.val[3], d.val[3]);
	uint16x8_t t = vaddl_u8(s.val[3], d.val[3]);
	d_a16 = vsubq_u16(t, vshrq_n_u16(d_a16, 8));
	d.val[3] = vmovn_u16(vsubq_u16(d_a16, vshrq_n_u16(d_a16, 8)));
	return do_AddAlphaBlend(s, d);
}
static uint8x8x4_t do_AddAlphaBlend_ao(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s = do_AddAlphaBlendSrc(s, opa);
	return do_AddAlphaBlend_a(s, d);
}
#endif

static uint8x8x4_t do_AlphaBlend_branch(uint8x8x4_t s, uint8x8x4_t d) {
	return do_SrcAlphaBranch(s, d, do_AlphaBlend);
}
static uint8x8x4_t do_AlphaBlend_a_branch(uint8x8x4_t s, uint8x8x4_t d) {
	return do_SrcAddAlphaBranch(s, d, do_AlphaBlend_a);
}
static uint8x8x4_t do_AlphaBlend_d_branch(uint8x8x4_t s, uint8x8x4_t d) {
	return do_SrcAlphaBranch(s, d, do_AlphaBlend_d);
}
static void TVPAlphaBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPAlphaBlend_HDA_c, do_AlphaBlend_branch, dest, src, len);
}
static void TVPAlphaBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAlphaBlend_HDA_o_c, do_AlphaBlend_o, dest, src, len, opa);
}
static void TVPAlphaBlend_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPAlphaBlend_a_c, do_AlphaBlend_a_branch, dest, src, len);
}
static void TVPAlphaBlend_ao_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAlphaBlend_ao_c, do_AlphaBlend_ao, dest, src, len, opa);
}
static void TVPAlphaBlend_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPAlphaBlend_d_c, do_AlphaBlend_d_branch, dest, src, len);
}
static void TVPAlphaBlend_do_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAlphaBlend_do_c, do_AlphaBlend_do, dest, src, len, opa);
}
static void TVPAdditiveAlphaBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPAdditiveAlphaBlend_HDA_c, do_AddAlphaBlend, dest, src, len);
}
static void TVPAdditiveAlphaBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAdditiveAlphaBlend_HDA_o_c, do_AddAlphaBlend_o, dest, src, len, opa);
}
static void TVPAdditiveAlphaBlend_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPAdditiveAlphaBlend_a_c, do_AddAlphaBlend_a, dest, src, len);
}
static void TVPAdditiveAlphaBlend_ao_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAdditiveAlphaBlend_ao_c, do_AddAlphaBlend_ao, dest, src, len, opa);
}
static void TVPConvertAlphaToAdditiveAlpha_NEON(tjs_uint32 *dest, tjs_int len) {
	do_apply_pixel(TVPConvertAlphaToAdditiveAlpha_c, do_ConvertAlphaToAdditiveAlpha, dest, len);
}

#ifndef Region_StretchBlend

static void TVPStretchAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchAlphaBlend_HDA_c, do_AlphaBlend_branch, dest, len, src, srcstart, srcstep);
}
static void TVPStretchAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchAlphaBlend_HDA_o_c, do_AlphaBlend_o, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchAlphaBlend_a_c, do_AlphaBlend_a_branch, dest, len, src, srcstart, srcstep);
}
static void TVPStretchAlphaBlend_ao_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchAlphaBlend_ao_c, do_AlphaBlend_ao, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchAlphaBlend_d_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchAlphaBlend_d_c, do_AlphaBlend_d_branch, dest, len, src, srcstart, srcstep);
}
static void TVPStretchAlphaBlend_do_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchAlphaBlend_do_c, do_AlphaBlend_do, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchAdditiveAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchAdditiveAlphaBlend_HDA_c, do_AddAlphaBlend, dest, len, src, srcstart, srcstep);
}
static void TVPStretchAdditiveAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchAdditiveAlphaBlend_HDA_o_c, do_AddAlphaBlend_o, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchAdditiveAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchAdditiveAlphaBlend_a_c, do_AddAlphaBlend_a, dest, len, src, srcstart, srcstep);
}
static void TVPStretchAdditiveAlphaBlend_ao_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchAdditiveAlphaBlend_ao_c, do_AddAlphaBlend_ao, dest, len, src, srcstart, srcstep, opa);
}
static void TVPLinTransAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransAlphaBlend_HDA_c, do_AlphaBlend_branch, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPLinTransAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransAlphaBlend_HDA_o_c, do_AlphaBlend_o, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransAlphaBlend_a_c, do_AlphaBlend_a_branch, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPLinTransAlphaBlend_ao_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransAlphaBlend_ao_c, do_AlphaBlend_ao, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransAlphaBlend_d_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransAlphaBlend_d_c, do_AlphaBlend_d_branch, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPLinTransAlphaBlend_do_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransAlphaBlend_do_c, do_AlphaBlend_do, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransAdditiveAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransAlphaBlend_HDA_c, do_AddAlphaBlend, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPLinTransAdditiveAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransAlphaBlend_HDA_o_c, do_AddAlphaBlend_o, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransAdditiveAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransAlphaBlend_a_c, do_AddAlphaBlend_a, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPLinTransAdditiveAlphaBlend_ao_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransAlphaBlend_ao_c, do_AddAlphaBlend_ao, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPInterpStretchCopy_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep) {
	do_interp_stretch_blend(TVPInterpStretchCopy_c, do_copy_src, dest, len, src1, src2, _blend_y, srcstart, srcstep);
}
static void TVPInterpLinTransCopy_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_interp_lintrans_blend(TVPInterpLinTransCopy_c, do_copy_src, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPInterpStretchAdditiveAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep) {
	do_interp_stretch_blend(TVPInterpStretchAdditiveAlphaBlend_c, do_AddAlphaBlend, dest, len, src1, src2, _blend_y, srcstart, srcstep);
}
static void TVPInterpStretchAdditiveAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_interp_stretch_blend(TVPInterpStretchAdditiveAlphaBlend_o_c, do_AddAlphaBlend_o, dest, len, src1, src2, _blend_y, srcstart, srcstep, opa);
}
static void TVPInterpLinTransAdditiveAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_interp_lintrans_blend(TVPInterpLinTransAdditiveAlphaBlend_c, do_AddAlphaBlend, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}
static void TVPInterpLinTransAdditiveAlphaBlend_o_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_interp_lintrans_blend(TVPInterpLinTransAdditiveAlphaBlend_o_c, do_AddAlphaBlend_o, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPInterpStretchConstAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_interp_stretch_blend(TVPInterpStretchConstAlphaBlend_c, do_ConstAlphaBlend_SD, dest, len, src1, src2, _blend_y, srcstart, srcstep, opa);
}
static void TVPInterpLinTransConstAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_interp_lintrans_blend(TVPInterpLinTransConstAlphaBlend_c, do_ConstAlphaBlend_SD, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}

#endif

static uint8x8x4_t do_CopyOpaqueImage_64(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[3] = vdup_n_u8(0xFF);
	return s;
}
static uint8x16x4_t do_CopyOpaqueImage_128(uint8x16x4_t s) {
	s.val[3] = vdupq_n_u8(0xFF);
	return s;
}
static void TVPCopyOpaqueImage_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPCopyOpaqueImage_c, do_CopyOpaqueImage_64, dest, src, len);
}
static void TVPStretchCopyOpaqueImage_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep) {
	do_stretch_blend(TVPStretchCopyOpaqueImage_c, do_CopyOpaqueImage_64, dest, len, src, srcstart, srcstep);
}
static void TVPLinTransCopyOpaqueImage_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch) {
	do_lintrans_blend(TVPLinTransCopyOpaqueImage_c, do_CopyOpaqueImage_64, dest, len, src, sx, sy, stepx, stepy, srcpitch);
}

#ifndef Region_ConstAlphaBlend
static void TVPConstAlphaBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPConstAlphaBlend_HDA_c, do_ConstAlphaBlend, dest, src, len, opa);
}
static void TVPConstAlphaBlend_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPConstAlphaBlend_d_c, do_ConstAlphaBlend_d, dest, src, len, opa);
}
static void TVPConstAlphaBlend_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPConstAlphaBlend_a_c, do_ConstAlphaBlend_a, dest, src, len, opa);
}

static void TVPStretchConstAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchConstAlphaBlend_HDA_c, do_ConstAlphaBlend, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchConstAlphaBlend_d_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchConstAlphaBlend_d_c, do_ConstAlphaBlend_d, dest, len, src, srcstart, srcstep, opa);
}
static void TVPStretchConstAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep, tjs_int opa) {
	do_stretch_blend(TVPStretchConstAlphaBlend_a_c, do_ConstAlphaBlend_a, dest, len, src, srcstart, srcstep, opa);
}

static void TVPLinTransConstAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransConstAlphaBlend_HDA_c, do_ConstAlphaBlend, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransConstAlphaBlend_d_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransConstAlphaBlend_d_c, do_ConstAlphaBlend_d, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}
static void TVPLinTransConstAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch, tjs_int opa) {
	do_lintrans_blend(TVPLinTransConstAlphaBlend_a_c, do_ConstAlphaBlend_a, dest, len, src, sx, sy, stepx, stepy, srcpitch, opa);
}

static void TVPConstAlphaBlend_SD_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa) {
	do_blend_2(TVPConstAlphaBlend_SD_c, do_ConstAlphaBlend_SD, dest, src1, src2, len, opa);
}
static void TVPConstAlphaBlend_SD_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa) {
	do_blend_2(TVPConstAlphaBlend_SD_d_c, do_ConstAlphaBlend_SD_d, dest, src1, src2, len, opa);
}
static void TVPConstAlphaBlend_SD_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len, tjs_int opa) {
	do_blend_2(TVPConstAlphaBlend_SD_a_c, do_ConstAlphaBlend_SD_a, dest, src1, src2, len, opa);
}
#endif

#ifndef Region_UnivTransBlend
static uint8x8x4_t do_UnivTransBlend(uint8x8x4_t s2, uint8x8x4_t s1, uint8x8_t opa) {
	s2.val[3] = opa;
	return do_AlphaBlend(s2, s1);
}
static uint8x8x4_t do_UnivTransBlend_d(uint8x8x4_t s2, uint8x8x4_t s1, uint8x8_t opa) {
	uint16x8_t s1_a16 = vmull_u8(s1.val[3], vmvn_u8(opa)); // a1*(256-opa)
	uint16x8_t d_a16 = vmulq_u16(vsubl_u8(s2.val[3], s1.val[3]), vmovl_u8(opa));
	uint16x8_t o16 = vsriq_n_u16(vmull_u8(s2.val[3], opa), s1_a16, 8); // addr
	s1.val[3] = vadd_u8(s1.val[3], vshrn_n_u16(d_a16, 8));
	return do_AlphaBlend_d_(s2, s1, o16);
}
static uint8x8x4_t do_UnivTransBlend_a(uint8x8x4_t s2, uint8x8x4_t s1, uint8x8_t opa) {
	s1.val[3] = vadd_u8(s1.val[3], vsubhn_u16(vmull_u8(s2.val[3], opa), vmull_u8(s1.val[3], opa)));
	return do_UnivTransBlend(s2, s1, opa);
}
static void TVPUnivTransBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len) {
	do_univ_blend(TVPUnivTransBlend_c, do_UnivTransBlend, dest, src1, src2, rule, table, len);
}
static void TVPUnivTransBlend_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len) {
	do_univ_blend(TVPUnivTransBlend_d_c, do_UnivTransBlend_d, dest, src1, src2, rule, table, len);
}
static void TVPUnivTransBlend_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len) {
	do_univ_blend(TVPUnivTransBlend_a_c, do_UnivTransBlend_a, dest, src1, src2, rule, table, len);
}
static void TVPUnivTransBlend_switch_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv) {
	do_univ_switch(TVPUnivTransBlend_switch_c, do_UnivTransBlend, dest, src1, src2, rule, table, len, src1lv, src2lv);
}
static void TVPUnivTransBlend_switch_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv) {
	do_univ_switch(TVPUnivTransBlend_switch_d_c, do_UnivTransBlend_d, dest, src1, src2, rule, table, len, src1lv, src2lv);
}
static void TVPUnivTransBlend_switch_a_NEON(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len, tjs_int src1lv, tjs_int src2lv) {
	do_univ_switch(TVPUnivTransBlend_switch_a_c, do_UnivTransBlend_a, dest, src1, src2, rule, table, len, src1lv, src2lv);
}
#endif

#ifndef Region_ApplyColorMap

static uint8x8x4_t do_mergeColorSrc(uint8x8_t s, tjs_uint32 color) {
	uint8x8x4_t src;
	src.val[2] = vdup_n_u8((color >> 16) & 0xFF);
	src.val[1] = vdup_n_u8((color >> 8) & 0xFF);
	src.val[0] = vdup_n_u8((color >> 0) & 0xFF);
	src.val[3] = s;
	return src;
}

static uint8x8x4_t do_ApplyColorMap(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color) {
	uint8x8x4_t src = do_mergeColorSrc(s, color);
	return do_AlphaBlend(src, d);
}
static uint8x8x4_t do_ApplyColorMap_o(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint8x8x4_t src = do_mergeColorSrc(s, color);
	return do_AlphaBlend_o(src, d, opa);
}
static uint8x8x4_t do_ApplyColorMap_d(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color) {
	uint16x8_t s_a16 = vshll_n_u8(s, 8);
	uint16x8_t isd_a16 = vmull_u8(vmvn_u8(s), vmvn_u8(d.val[3]));
	uint16x8_t sopa = vorrq_u16(s_a16, vmovl_u8(d.val[3]));
	uint8x8x4_t src = do_mergeColorSrc(s, color);
	d.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
	return do_AlphaBlend_d_(src, d, sopa);
}
static uint8x8x4_t do_ApplyColorMap_a(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color) {
	uint8x8x4_t src = do_mergeColorSrc(s, color);
	return do_AlphaBlend_a(src, d);
}
static uint8x8x4_t do_ApplyColorMap_do(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint16x8_t s_a16 = vmull_u8(s, vdup_n_u8(opa));
	s = vshrn_n_u16(s_a16, 8);
	return do_ApplyColorMap_d(s, d, color);
}
static uint8x8x4_t do_ApplyColorMap_ao(uint8x8_t s, uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint16x8_t s_a16 = vmull_u8(s, vdup_n_u8(opa));
	s = vshrn_n_u16(s_a16, 8);
	return do_ApplyColorMap_a(s, d, color);
}

static void TVPApplyColorMap_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color) {
	do_blend_lum(TVPApplyColorMap_HDA_c, do_ApplyColorMap, dest, src, len, color);
}
static void TVPApplyColorMap_o_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_blend_lum(TVPApplyColorMap_HDA_o_c, do_ApplyColorMap_o, dest, src, len, color, opa);
}
static void TVPApplyColorMap_d_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color) {
	do_blend_lum(TVPApplyColorMap_d_c, do_ApplyColorMap_d, dest, src, len, color);
}
static void TVPApplyColorMap_a_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color) {
	do_blend_lum(TVPApplyColorMap_a_c, do_ApplyColorMap_a, dest, src, len, color);
}
static void TVPApplyColorMap_do_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_blend_lum(TVPApplyColorMap_do_c, do_ApplyColorMap_do, dest, src, len, color, opa);
}
static void TVPApplyColorMap_ao_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_blend_lum(TVPApplyColorMap_ao_c, do_ApplyColorMap_ao, dest, src, len, color, opa);
}
#endif

#ifndef Region_ConstColorAlphaBlend
static uint8x8x4_t do_ConstColorAlphaBlend(uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint16x8_t s_r16 = vdupq_n_u16(((color >> 16) & 0xFF) * opa);
	uint16x8_t s_g16 = vdupq_n_u16(((color >> 8) & 0xFF) * opa);
	uint16x8_t s_b16 = vdupq_n_u16(((color >> 0) & 0xFF) * opa);
	uint8x8_t s_ia8 = vdup_n_u8(opa ^ 0xFF);
	uint16x8_t d_r16 = vmull_u8(d.val[2], s_ia8);
	uint16x8_t d_g16 = vmull_u8(d.val[1], s_ia8);
	uint16x8_t d_b16 = vmull_u8(d.val[0], s_ia8);
	d.val[2] = vshrn_n_u16(vaddq_u16(d_r16, s_r16), 8);
	d.val[1] = vshrn_n_u16(vaddq_u16(d_g16, s_g16), 8);
	d.val[0] = vshrn_n_u16(vaddq_u16(d_b16, s_b16), 8);
	return d;
}
static uint8x8x4_t do_ConstColorAlphaBlend_d(uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint16x8_t hopa16 = vdupq_n_u16(opa << 8);
	uint8x8_t s_ia8 = vdup_n_u8(opa ^ 0xFF);
	uint8x8x4_t s;
	s.val[2] = vdup_n_u8((color >> 16) & 0xFF);
	s.val[1] = vdup_n_u8((color >> 8) & 0xFF);
	s.val[0] = vdup_n_u8((color >> 0) & 0xFF);
	uint16x8_t isd_a16 = vmull_u8(s_ia8, vmvn_u8(d.val[3]));
	uint16x8_t s_a16 = vorrq_u16(hopa16, vmovl_u8(d.val[3]));
	d.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8)); //(255-((255-dopa)*(255-opa)>>8))
	return do_AlphaBlend_d_(s, d, s_a16);
}
static uint8x8x4_t do_ConstColorAlphaBlend_a(uint8x8x4_t d, tjs_uint32 color, tjs_int opa) {
	uint8x8x4_t s;
	s.val[2] = vdup_n_u8((((color >> 16) & 0xFF) * opa) >> 8);
	s.val[1] = vdup_n_u8((((color >> 8) & 0xFF) * opa) >> 8);
	s.val[0] = vdup_n_u8((((color >> 0) & 0xFF) * opa) >> 8);
	s.val[3] = vdup_n_u8(opa);
	d = do_AlphaBlend_da(s, d);
	return do_AddAlphaBlend(s, d);
}
static void TVPConstColorAlphaBlend_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_apply_pixel(TVPConstColorAlphaBlend_c, do_ConstColorAlphaBlend, dest, len, color, opa);
}
static void TVPConstColorAlphaBlend_d_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_apply_pixel(TVPConstColorAlphaBlend_d_c, do_ConstColorAlphaBlend_d, dest, len, color, opa);
}
static void TVPConstColorAlphaBlend_a_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa) {
	do_apply_pixel(TVPConstColorAlphaBlend_a_c, do_ConstColorAlphaBlend_a, dest, len, color, opa);
}
#endif

static uint8x8x4_t do_RemoveConstOpacity(uint8x8x4_t d, tjs_int opa) {
	d.val[3] = vshrn_n_u16(vmull_u8(d.val[3], vdup_n_u8(opa)), 8);
	return d;
}
static uint8x8x4_t do_RemoveOpacity(uint8x8_t s, uint8x8x4_t d) {
	d.val[3] = vshrn_n_u16(vmull_u8(d.val[3], vmvn_u8(s)), 8);
	return d;
}
static uint8x8x4_t do_RemoveOpacity_o(uint8x8_t s, uint8x8x4_t d, tjs_int _strength) {
	uint8x8_t strength = vdup_n_u8(_strength);
	uint16x8_t s16 = vmull_u8(s, strength); // s * str(8pix)
	s16 = vmull_u8(vshrn_n_u16(vmvnq_u16(s16), 8), d.val[3]); // da * (65535 - s * str)
	d.val[3] = vshrn_n_u16(s16, 8);
	return d;
}
static void TVPRemoveConstOpacity_NEON(tjs_uint32 *dest, tjs_int len, tjs_int strength)
{
	do_apply_pixel(TVPRemoveConstOpacity_c, do_RemoveConstOpacity, dest, len, 255 - strength);
}
static void TVPRemoveOpacity_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len)
{
	do_blend_lum(TVPRemoveOpacity_c, do_RemoveOpacity, dest, src, len);
}
static void TVPRemoveOpacity_o_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int _strength)
{
	do_blend_lum(TVPRemoveOpacity_o_c, do_RemoveOpacity_o, dest, src, len, _strength);
}

#ifndef Region_AddBlend
static uint8x16x4_t do_AddBlend_HDA_128(uint8x16x4_t s, uint8x16x4_t d) {
	d.val[2] = vqaddq_u8(d.val[2], s.val[2]);
	d.val[1] = vqaddq_u8(d.val[1], s.val[1]);
	d.val[0] = vqaddq_u8(d.val[0], s.val[0]);
	return d;
}
static uint8x16x4_t do_AddBlend_NonHDA_128(uint8x16x4_t s, uint8x16x4_t d) {
	d = do_AddBlend_HDA_128(s, d);
	d.val[3] = vqaddq_u8(d.val[3], s.val[3]);
	return d;
}
static uint8x8x4_t do_AddBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	s = do_ConvertAlphaToAdditiveAlpha(s);
	d.val[2] = vqadd_u8(d.val[2], s.val[2]);
	d.val[1] = vqadd_u8(d.val[1], s.val[1]);
	d.val[0] = vqadd_u8(d.val[0], s.val[0]);
	return d;
}
static void TVPAddBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPAddBlend_c, do_AddBlend_NonHDA_128, dest, src, len);
}
static void TVPAddBlend_HDA_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPAddBlend_HDA_c, do_AddBlend_HDA_128, dest, src, len);
}
static void TVPAddBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPAddBlend_HDA_o_c, do_AddBlend_o, dest, src, len, opa);
}
#endif

#ifndef Region_SubBlend
static uint8x16x4_t do_SubBlend_HDA(uint8x16x4_t s, uint8x16x4_t d) {
	d.val[2] = vqsubq_u8(d.val[2], vmvnq_u8(s.val[2]));
	d.val[1] = vqsubq_u8(d.val[1], vmvnq_u8(s.val[1]));
	d.val[0] = vqsubq_u8(d.val[0], vmvnq_u8(s.val[0]));
	return d;
}
static uint8x16x4_t do_SubBlend_NonHDA(uint8x16x4_t s, uint8x16x4_t d) {
	d = do_SubBlend_HDA(s, d);
	d.val[3] = vqsubq_u8(d.val[3], vmvnq_u8(s.val[3]));
	return d;
}
static uint8x8x4_t do_SubBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	s.val[2] = vmvn_u8(s.val[2]);
	s.val[1] = vmvn_u8(s.val[1]);
	s.val[0] = vmvn_u8(s.val[0]);
	s.val[2] = vshrn_n_u16(vmull_u8(s.val[2], opa8), 8);
	s.val[1] = vshrn_n_u16(vmull_u8(s.val[1], opa8), 8);
	s.val[0] = vshrn_n_u16(vmull_u8(s.val[0], opa8), 8);
	d.val[2] = vqsub_u8(d.val[2], s.val[2]);
	d.val[1] = vqsub_u8(d.val[1], s.val[1]);
	d.val[0] = vqsub_u8(d.val[0], s.val[0]);
	return d;
}
static void TVPSubBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPSubBlend_c, do_SubBlend_NonHDA, dest, src, len);
}
static void TVPSubBlend_HDA_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPSubBlend_HDA_c, do_SubBlend_HDA, dest, src, len);
}
static void TVPSubBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPSubBlend_HDA_o_c, do_SubBlend_o, dest, src, len, opa);
}
#endif

#ifndef Region_MulBlend
static uint8x8x4_t do_MulBlend_HDA(uint8x8x4_t s, uint8x8x4_t d) {
	d.val[2] = vshrn_n_u16(vmull_u8(s.val[2], d.val[2]), 8);
	d.val[1] = vshrn_n_u16(vmull_u8(s.val[1], d.val[1]), 8);
	d.val[0] = vshrn_n_u16(vmull_u8(s.val[0], d.val[0]), 8);
	return d;
}
static uint8x8x4_t do_MulBlend_o_HDA(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t s_r16 = vmull_u8(vmvn_u8(s.val[2]), opa8);
	uint16x8_t s_g16 = vmull_u8(vmvn_u8(s.val[1]), opa8);
	uint16x8_t s_b16 = vmull_u8(vmvn_u8(s.val[0]), opa8);
	s.val[2] = vmvn_u8(vshrn_n_u16(s_r16, 8));
	s.val[1] = vmvn_u8(vshrn_n_u16(s_g16, 8));
	s.val[0] = vmvn_u8(vshrn_n_u16(s_b16, 8));
	return do_MulBlend_HDA(s, d);
}
static uint8x8x4_t do_MulBlend_NonHDA(uint8x8x4_t s, uint8x8x4_t d) {
	d = do_MulBlend_HDA(s, d);
	d.val[3] = vdup_n_u8(0);
	return d;
}
static uint8x8x4_t do_MulBlend_o_NonHDA(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	d = do_MulBlend_o_HDA(s, d, opa);
	d.val[3] = vdup_n_u8(0);
	return d;
}
static void TVPMulBlend_HDA_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPMulBlend_HDA_c, do_MulBlend_HDA, dest, src, len);
}
static void TVPMulBlend_HDA_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPMulBlend_HDA_o_c, do_MulBlend_o_HDA, dest, src, len, opa);
}
static void TVPMulBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPMulBlend_c, do_MulBlend_NonHDA, dest, src, len);
}
static void TVPMulBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPMulBlend_o_c, do_MulBlend_o_NonHDA, dest, src, len, opa);
}
#endif
static uint8x8x4_t do_ColorDodgeBlend(uint8x8x4_t s_argb8, uint8x8x4_t d_argb8) {
	uint8_t tmpbuff[16 + 8];
	uint8_t *tmpb = (uint8_t *)__builtin_assume_aligned((uint8_t*)((((intptr_t)tmpbuff) + 7) & ~7), 8);
	for (int i = 0; i < 3; ++i) {
		// d = d * 255 / (255 - s)
		s_argb8.val[i] = vmvn_u8(s_argb8.val[i]);
		uint16x8_t tmp = vsubl_u8(s_argb8.val[i], d_argb8.val[i]);
		uint8x8_t mask = vshrn_n_u16(tmp, 8); // 00 or FF
		vst1_u8(tmpb, s_argb8.val[i]);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[0]], tmp, 0);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[1]], tmp, 1);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[2]], tmp, 2);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[3]], tmp, 3);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[4]], tmp, 4);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[5]], tmp, 5);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[6]], tmp, 6);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[7]], tmp, 7);
		tmp = vmulq_u16(vmovl_u8(d_argb8.val[i]), tmp);
		d_argb8.val[i] = vorr_u8(vshrn_n_u16(tmp, 8), mask);
	}
	return d_argb8;
}
static uint8x8x4_t do_ColorDodgeBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	s = do_ConvertAlphaToAdditiveAlpha(s);
	return do_ColorDodgeBlend(s, d);
}
static void TVPColorDodgeBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPColorDodgeBlend_HDA_c, do_ColorDodgeBlend, dest, src, len);
}
static void TVPColorDodgeBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPColorDodgeBlend_HDA_o_c, do_ColorDodgeBlend_o, dest, src, len, opa);
}

static uint8x16x4_t do_DarkenBlend(uint8x16x4_t s, uint8x16x4_t d) {
	d.val[0] = vminq_u8(s.val[0], d.val[0]);
	d.val[1] = vminq_u8(s.val[1], d.val[1]);
	d.val[2] = vminq_u8(s.val[2], d.val[2]);
	return d;
}
static uint8x8x4_t do_DarkenBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[2] = vmin_u8(s.val[2], d.val[2]);
	s.val[1] = vmin_u8(s.val[1], d.val[1]);
	s.val[0] = vmin_u8(s.val[0], d.val[0]);
	s.val[3] = vdup_n_u8(opa);
	return do_AlphaBlend(s, d);
}
static uint8x16x4_t do_LightenBlend(uint8x16x4_t s, uint8x16x4_t d) {
	d.val[0] = vmaxq_u8(s.val[0], d.val[0]);
	d.val[1] = vmaxq_u8(s.val[1], d.val[1]);
	d.val[2] = vmaxq_u8(s.val[2], d.val[2]);
	return d;
}
static uint8x8x4_t do_LightenBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[2] = vmax_u8(s.val[2], d.val[2]);
	s.val[1] = vmax_u8(s.val[1], d.val[1]);
	s.val[0] = vmax_u8(s.val[0], d.val[0]);
	s.val[3] = vdup_n_u8(opa);
	return do_AlphaBlend(s, d);
}
static uint8x8x4_t do_ScreenBlend(uint8x8x4_t s_argb8, uint8x8x4_t d_argb8) {
	uint16x8_t d_r16 = vmull_u8(vmvn_u8(s_argb8.val[2]), vmvn_u8(d_argb8.val[2]));
	uint16x8_t d_g16 = vmull_u8(vmvn_u8(s_argb8.val[1]), vmvn_u8(d_argb8.val[1]));
	uint16x8_t d_b16 = vmull_u8(vmvn_u8(s_argb8.val[0]), vmvn_u8(d_argb8.val[0]));
	d_argb8.val[2] = vmvn_u8(vshrn_n_u16(d_r16, 8));
	d_argb8.val[1] = vmvn_u8(vshrn_n_u16(d_g16, 8));
	d_argb8.val[0] = vmvn_u8(vshrn_n_u16(d_b16, 8));
	return d_argb8;
}
static uint8x8x4_t do_ScreenBlend_o(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	s.val[3] = vdup_n_u8(opa);
	s = do_ConvertAlphaToAdditiveAlpha(s);
	return do_ScreenBlend(s, d);
}
static void TVPDarkenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPDarkenBlend_HDA_c, do_DarkenBlend, dest, src, len);
}
static void TVPDarkenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPDarkenBlend_HDA_o_c, do_DarkenBlend_o, dest, src, len, opa);
}
static void TVPLightenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPLightenBlend_HDA_c, do_LightenBlend, dest, src, len);
}
static void TVPLightenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPDarkenBlend_HDA_o_c, do_LightenBlend_o, dest, src, len, opa);
}
static void TVPScreenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPScreenBlend_HDA_c, do_ScreenBlend, dest, src, len);
}
static void TVPScreenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPScreenBlend_HDA_o_c, do_ScreenBlend_o, dest, src, len, opa);
}

static void TVPFastLinearInterpV2_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src0, const tjs_uint32 *src1)
{
    tjs_uint32* pEndDst = dest + len;
    {
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPFastLinearInterpV2_c(dest, PreFragLen, src0, src1);
            dest += PreFragLen;
            src0 += PreFragLen;
            src1 += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = pEndDst-3;
	if ((((intptr_t)src0) & 7) || (((intptr_t)src1) & 7)) {
		while (dest < pVecEndDst) {
			uint8x16_t s0 = vld1q_u8((uint8_t *)__builtin_assume_aligned(src0, 4));
			uint8x16_t s1 = vld1q_u8((uint8_t *)__builtin_assume_aligned(src1, 4));

			vst1q_u8((uint8_t *)__builtin_assume_aligned(dest, 8), vhaddq_u8(s0, s1));
			dest += 4;
			src0 += 4;
			src1 += 4;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16_t s0 = vld1q_u8((uint8_t *)__builtin_assume_aligned(src0, 8));
			uint8x16_t s1 = vld1q_u8((uint8_t *)__builtin_assume_aligned(src1, 8));

			vst1q_u8((uint8_t *)__builtin_assume_aligned(dest, 8), vhaddq_u8(s0, s1));
			dest += 4;
			src0 += 4;
			src1 += 4;
		}
	}

    if(dest < pEndDst) {
        TVPFastLinearInterpV2_c(dest, pEndDst - dest, src0, src1);
    }
}
static uint8x8x4_t do_CopyMask(uint8x8x4_t s, uint8x8x4_t d) {
	d.val[3] = s.val[3];
	return d;
}
static void TVPCopyMask_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPCopyMask_c, do_CopyMask, dest, src, len);
}
static uint8x8x4_t do_CopyColor(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[3] = d.val[3];
	return s;
}
static void TVPCopyColor_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPCopyColor_c, do_CopyColor, dest, src, len);
}
static uint8x8x4_t do_BindMaskToMain(uint8x8_t s, uint8x8x4_t d) {
	d.val[3] = s;
	return d;
}
static void TVPBindMaskToMain_NEON(tjs_uint32 *main, const tjs_uint8 *mask, tjs_int len) {
	do_blend_lum(TVPBindMaskToMain_c, do_BindMaskToMain, main, mask, len);
}
static void TVPFillARGB_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 value)
{
	tjs_uint32* pEndDst = dest + len;
	while((((intptr_t)dest)&~15) && dest < pEndDst) {
		*dest++ = value;
	}

	uint32x4_t v = vdupq_n_u32(value);
	tjs_uint32* pVecEndDst = pEndDst-3;
	while(dest < pVecEndDst) {
		vst1q_u32((uint32_t *)__builtin_assume_aligned(dest, 16), v);
		dest += 4;
	}
	while(dest < pEndDst) {
		*dest++ = value;
	}
}
static uint8x8x4_t do_FillColor(uint8x8x4_t d, tjs_uint32 color) {
    d.val[0] = vdup_n_u8(color & 0xff);
    d.val[1] = vdup_n_u8((color >> 8) & 0xff);
    d.val[2] = vdup_n_u8((color >> 16) & 0xff);
	return d;
}

static void TVPFillColor_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 color) {
	do_apply_pixel(TVPFillColor_c, do_FillColor, dest, len, color);
}
static uint8x8x4_t do_FillMask(uint8x8x4_t d, tjs_uint32 mask) {
    d.val[3] = vdup_n_u8(mask);
	return d;
}
static void TVPFillMask_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 mask) {
	do_apply_pixel(TVPFillMask_c, do_FillMask, dest, len, mask);
}

static void TVPAddSubVertSum16_NEON(tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len)
{
    tjs_uint16* pEndDst = dest + len * 4;
	// dest is always aligned
//     {
// 		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest) / 4;
//         if(PreFragLen > len) PreFragLen = len;
//         if(PreFragLen) {
//             TVPAddSubVertSum16_c(dest, addline, subline, PreFragLen);
//             dest += PreFragLen * 4;
//             addline += PreFragLen;
//             subline += PreFragLen;
//         }
//     }

    tjs_uint16* pVecEndDst = pEndDst-7;
	if ((((intptr_t)addline) & 7) || (((intptr_t)subline) & 7)) {
		while (dest < pVecEndDst) {
			uint8x8x4_t add = vld4_u8((uint8_t *)__builtin_assume_aligned(addline, 4));
			uint8x8x4_t sub = vld4_u8((uint8_t *)__builtin_assume_aligned(subline, 4));
			uint16x8x4_t d = vld4q_u16((uint16_t *)__builtin_assume_aligned(dest, 8));
			d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));
			d.val[2] = vaddq_u16(d.val[2], vsubl_u8(add.val[2], sub.val[2]));
			d.val[1] = vaddq_u16(d.val[1], vsubl_u8(add.val[1], sub.val[1]));
			d.val[0] = vaddq_u16(d.val[0], vsubl_u8(add.val[0], sub.val[0]));
			vst4q_u16((uint16_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 8 * 4;
			addline += 8;
			subline += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x8x4_t add = vld4_u8((uint8_t *)__builtin_assume_aligned(addline, 8));
			uint8x8x4_t sub = vld4_u8((uint8_t *)__builtin_assume_aligned(subline, 8));
			uint16x8x4_t d = vld4q_u16((uint16_t *)__builtin_assume_aligned(dest, 8));
			d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));
			d.val[2] = vaddq_u16(d.val[2], vsubl_u8(add.val[2], sub.val[2]));
			d.val[1] = vaddq_u16(d.val[1], vsubl_u8(add.val[1], sub.val[1]));
			d.val[0] = vaddq_u16(d.val[0], vsubl_u8(add.val[0], sub.val[0]));
			vst4q_u16((uint16_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 8 * 4;
			addline += 8;
			subline += 8;
		}
	}

    if(dest < pEndDst) {
        TVPAddSubVertSum16_c(dest, addline, subline, (pEndDst - dest) / 4);
    }
}

static void TVPAddSubVertSum16_d_NEON(tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len)
{
    tjs_uint16* pEndDst = dest + len * 4;
	// dest is always aligned
//     {
// 		tjs_int PreFragLen = (((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest)) / 4;
//         if(PreFragLen > len) PreFragLen = len;
//         if(PreFragLen) {
//             TVPAddSubVertSum16_d_c(dest, addline, subline, PreFragLen);
//             dest += PreFragLen * 4;
//             addline += PreFragLen;
//             subline += PreFragLen;
//         }
//     }

    tjs_uint16* pVecEndDst = pEndDst-7;
	while (dest < pVecEndDst) {
		uint8x8x4_t add = vld4_u8((uint8_t *)__builtin_assume_aligned(addline, 4));
		uint8x8x4_t sub = vld4_u8((uint8_t *)__builtin_assume_aligned(subline, 4));
		uint16x8x4_t d = vld4q_u16((uint16_t*)__builtin_assume_aligned(dest, 8));

        uint16x8_t add_a = vaddl_u8(add.val[3], vshr_n_u8(add.val[3], 7));
        uint16x8_t sub_a = vaddl_u8(sub.val[3], vshr_n_u8(sub.val[3], 7));
        d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));

        uint16x8_t add_16 = vmulq_u16(vmovl_u8(add.val[2]), add_a);
        uint16x8_t sub_16 = vmulq_u16(vmovl_u8(sub.val[2]), sub_a);
        add_16 = vshrq_n_u16(add_16, 8);
        sub_16 = vshrq_n_u16(sub_16, 8);
        d.val[2] = vaddq_u16(d.val[2], vsubq_u16(add_16, sub_16));

        add_16 = vmulq_u16(vmovl_u8(add.val[1]), add_a);
        sub_16 = vmulq_u16(vmovl_u8(sub.val[1]), sub_a);
        add_16 = vshrq_n_u16(add_16, 8);
        sub_16 = vshrq_n_u16(sub_16, 8);
        d.val[1] = vaddq_u16(d.val[1], vsubq_u16(add_16, sub_16));

        add_16 = vmulq_u16(vmovl_u8(add.val[0]), add_a);
        sub_16 = vmulq_u16(vmovl_u8(sub.val[0]), sub_a);
        add_16 = vshrq_n_u16(add_16, 8);
        sub_16 = vshrq_n_u16(sub_16, 8);
        d.val[0] = vaddq_u16(d.val[0], vsubq_u16(add_16, sub_16));

		vst4q_u16((uint16_t*)__builtin_assume_aligned(dest, 8), d);
        dest += 8 * 4;
        addline += 8;
        subline += 8;
    }

    if(dest < pEndDst) {
        TVPAddSubVertSum16_d_c(dest, addline, subline, (pEndDst - dest) / 4);
    }
}

static void TVPDoBoxBlurAvg16_NEON(tjs_uint32 *dest, tjs_uint16 *_sum, const tjs_uint16 * add, const tjs_uint16 * sub, tjs_int n, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
	// dest is always aligned
//     {
// 		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
//         if(PreFragLen > len) PreFragLen = len;
//         if(PreFragLen) {
// 			TVPDoBoxBlurAvg16_c(dest, _sum, add, sub, n, PreFragLen);
//             dest += PreFragLen;
//             add += PreFragLen;
//             sub += PreFragLen;
//         }
//     }

	static const int32_t c_shl_n[4] = { 0, 8, 16, 24 };

    uint32x4_t rcp = vdupq_n_u32((1<<16) / n);
	int32x4_t shl_n = vld1q_s32(c_shl_n);
    uint16x4_t half_n = vdup_n_u16(n >> 1);

    tjs_uint32* pVecEndDst = pEndDst-7;
	uint16x4_t sum = vld1_u16(_sum);
	while (dest < pVecEndDst) {
		uint32x4_t t = vmulq_u32(vaddl_u16(sum, half_n), rcp);
		uint32x4_t d = vshlq_u32(vshrq_n_u32(t, 16), shl_n);

// 		t0 = vmul_u32(vaddl_u16(src_sum.val[2], half_n));
// 		vorr_u32(d, vshl_n_u32(vshr_n_u32(t1, 16), 8));
// 		t1 = vmul_u32(vaddl_u16(src_sum.val[3], half_n));
// 		vorr_u32(d, vshl_n_u32(vshr_n_u32(t0, 16), 8));
// 		vorr_u32(d, vshl_n_u32(vshr_n_u32(t1, 16), 8));
// 
// 		uint16x4x4_t src_add = vld4_u16

//         uint16x8_t add = vld1q_u16(add);
//         uint16x8_t sub = vld1q_u16(sub);
//         uint16x8_t d = vld4q_u16(dest);
//         d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));
//         d.val[2] = vaddq_u16(d.val[2], vsubl_u8(add.val[2], sub.val[2]));
//         d.val[1] = vaddq_u16(d.val[1], vsubl_u8(add.val[1], sub.val[1]));
//         d.val[0] = vaddq_u16(d.val[0], vsubl_u8(add.val[0], sub.val[0]));
        vst1q_u32(dest, d);
        dest += 8;
        add += 8;
        sub += 8;
    }

    if(dest < pEndDst) {
		TVPDoBoxBlurAvg16_c(dest, _sum, add, sub, n, pEndDst - dest);
    }
}

static uint8x8x4_t do_Expand8BitTo32BitGray(uint8x8_t s, uint8x8x4_t d) {
	d.val[0] = s;
	d.val[1] = s;
	d.val[2] = s;
    d.val[3] = vdup_n_u8(0xFF);
	return d;
}
static void TVPExpand8BitTo32BitGray_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len) {
	do_blend_lum(TVPExpand8BitTo32BitGray_c, do_Expand8BitTo32BitGray, dest, src, len);
}
static uint8x16x4_t do_ReverseRGB(uint8x16x4_t s, uint8x16x4_t d) {
	uint8x16_t t = s.val[0];
	s.val[0] = s.val[2];
	s.val[2] = t;
	return s;
}
static void TVPReverseRGB_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend_128(TVPReverseRGB_c, do_ReverseRGB, dest, src, len);
}

static void TVPUpscale65_255_NEON(tjs_uint8 *dest, tjs_int len) {
	// dest is already aligned by 16 bytes
	tjs_uint8* pEndDst = dest + len;
	tjs_uint8* pVecEndDst = pEndDst - 15;

	while (dest < pVecEndDst) {
		uint8x16_t d = vld1q_u8((uint8_t *)__builtin_assume_aligned(dest, 16));
		d = vqshlq_n_u8(d, 2);
		vst1q_u8((uint8_t *)__builtin_assume_aligned(dest, 16), d);
		dest += 16;
	}
	while (dest < pEndDst) {
		tjs_uint c = *dest << 2;
		*dest = c > 255 ? 255 : c;
		++dest;
	}
}

static const unsigned char rgb555_lut[4][8] = {
    { 0, 0x8, 0x10, 0x18, 0x21, 0x29, 0x31, 0x39 },
    { 0x42, 0x4A, 0x52, 0x5A, 0x63, 0x6B, 0x73, 0x7B },
    { 0x84, 0x8C, 0x94, 0x9C, 0xA5, 0xAD, 0xB5, 0xBD },
    { 0xC6, 0xCE, 0xD6, 0xDE, 0xE7, 0xEF, 0xF7, 0xFF } };

static void TVPBLConvert15BitTo32Bit_NEON(tjs_uint32 *dest, const tjs_uint16 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
        if (PreFragLen > len) PreFragLen = len;
        if (PreFragLen) {
            TVPBLConvert15BitTo32Bit_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = pEndDst - 7;

    if (dest < pVecEndDst)
    {
        uint8x8x4_t d;
        d.val[3] = vdup_n_u8(0xFF);
#if 0 //def __LP64__
        uint8x16x2_t lut;
        lut.val[0] = vld1q_u8(rgb555_lut[0]);
        lut.val[1] = vld1q_u8(rgb555_lut[2]);
        
        while (dest < pVecEndDst) {
            uint16x8_t s = vshlq_n_u16(vld1q_u16(src), 1);
            d.val[0] = vtbl2q_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
            s = vshlq_n_u16(s, 5);
            d.val[1] = vtbl2q_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
            s = vshlq_n_u16(s, 5);
            d.val[2] = vtbl2q_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
            vst4_u8((uint8_t*)dest, d);
            dest += 8;
            src += 8;
        }
#else
        uint8x8x4_t lut;
        lut.val[0] = vld1_u8(rgb555_lut[0]);
        lut.val[1] = vld1_u8(rgb555_lut[1]);
        lut.val[2] = vld1_u8(rgb555_lut[2]);
        lut.val[3] = vld1_u8(rgb555_lut[3]);

		if (((intptr_t)src) & 7) {
			while (dest < pVecEndDst) {
				uint16x8_t s = vshlq_n_u16(vld1q_u16((uint16_t *)__builtin_assume_aligned(src, 4)), 1);
				d.val[0] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[1] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[2] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
				dest += 8;
				src += 8;
			}
		} else {
			while (dest < pVecEndDst) {
				uint16x8_t s = vshlq_n_u16(vld1q_u16((uint16_t *)__builtin_assume_aligned(src, 8)), 1);
				d.val[0] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[1] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[2] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				vst4_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
				dest += 8;
				src += 8;
			}
		}
#endif
    }

    if (dest < pEndDst) {
        TVPBLConvert15BitTo32Bit_c(dest, src, pEndDst - dest);
    }
}

static void TVPConvert24BitTo32Bit_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
		tjs_int PreFragLen = ((-(((tjs_int)(intptr_t)dest) & 7)) & 7) / sizeof(*dest);
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPConvert24BitTo32Bit_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen * 3;
        }
    }

    tjs_uint32* pVecEndDst = pEndDst-15;
    uint8x16x4_t d;
    d.val[3] = vdupq_n_u8(0xFF);
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			uint8x16x3_t s = vld3q_u8((uint8_t *)__builtin_assume_aligned(src, 4));
			d.val[2] = s.val[0];
			d.val[1] = s.val[1];
			d.val[0] = s.val[2];
			vst4q_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 16;
			src += 16 * 3;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16x3_t s = vld3q_u8((uint8_t *)__builtin_assume_aligned(src, 8));
			d.val[2] = s.val[0];
			d.val[1] = s.val[1];
			d.val[0] = s.val[2];
			vst4q_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 16;
			src += 16 * 3;
		}
	}

    if(dest < pEndDst) {
        TVPConvert24BitTo32Bit_c(dest, src, pEndDst - dest);
    }
}

static void TVPConvert32BitTo24Bit_NEON(tjs_uint8 *dest, const tjs_uint8 *src, tjs_int len) {
	const tjs_uint8* pEndSrc = src + len;
	{
		tjs_int PreFragLen = (-(((tjs_int)(intptr_t)src) & 7)) & 7;
		if (PreFragLen > len) PreFragLen = len;
		const tjs_uint8 *pend = src + PreFragLen; // in bytes
		while (src < pend)
		{
			dest[0] = src[0];
			dest[1] = src[1];
			dest[2] = src[2];
			dest += 3;
			src += 4;
		}
	}

	const tjs_uint8* pVecEndSrc = pEndSrc - 15;
	uint8x16x3_t d;
	if (((intptr_t)dest) & 7) {
		while (src < pVecEndSrc) {
			uint8x16x4_t s = vld4q_u8((uint8_t *)__builtin_assume_aligned(src, 8));
			d.val[0] = s.val[0];
			d.val[1] = s.val[1];
			d.val[2] = s.val[2];
			vst3q_u8((uint8_t *)__builtin_assume_aligned(dest, 4), d);
			dest += 16 * 3;
			src += 16 * 4;
		}
	} else {
		while (src < pVecEndSrc) {
			uint8x16x4_t s = vld4q_u8((uint8_t *)__builtin_assume_aligned(src, 8));
			d.val[0] = s.val[0];
			d.val[1] = s.val[1];
			d.val[2] = s.val[2];
			vst3q_u8((uint8_t *)__builtin_assume_aligned(dest, 8), d);
			dest += 16 * 3;
			src += 16 * 4;
		}
	}

	while (src < pEndSrc) {
		dest[0] = src[0];
		dest[1] = src[1];
		dest[2] = src[2];
		dest += 3;
		src += 4;
	}
}

static uint8x8x4_t do_GrayScale(uint8x8x4_t s) {
    uint8x8_t const_19 = vdup_n_u8(19), const_183 = vdup_n_u8(183), const_54 = vdup_n_u8(54);
	uint16x8_t r = vmull_u8(s.val[0], const_19);
	uint16x8_t g = vmull_u8(s.val[1], const_183);
	uint16x8_t b = vmull_u8(s.val[2], const_54);
	r = vaddq_u16(r, g);
	r = vaddq_u16(r, b);
	s.val[2] = s.val[1] = s.val[0] = vshrn_n_u16(r, 8);
	return s;
}
static void TVPDoGrayScale_NEON(tjs_uint32 *dest, tjs_int len) {
	do_apply_pixel(TVPDoGrayScale_c, do_GrayScale, dest, len);
}
#ifndef Region_PsBlend
template <uint8x8x4_t op_func(uint8x8x4_t, uint8x8x4_t)>
uint8x8x4_t do_PsAlphaBlend_so(uint8x8x4_t s, uint8x8x4_t d, tjs_int opa) {
	uint8x8_t opa8 = vdup_n_u8(opa);
	uint16x8_t a = vmull_u8(s.val[3], opa8);
	s.val[3] = vshrn_n_u16(a, 8);
	return op_func(s, d);
}
static void TVPPsAlphaBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsAlphaBlend_HDA_c, do_AlphaBlend, dest, src, len);
}
static void TVPPsAlphaBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsAlphaBlend_HDA_o_c, do_PsAlphaBlend_so<do_AlphaBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsAddBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vqadd_u8(s.val[2], d.val[2]);
	s.val[1] = vqadd_u8(s.val[1], d.val[1]);
	s.val[0] = vqadd_u8(s.val[0], d.val[0]);
	return do_AlphaBlend(s, d);
}
static void TVPPsAddBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsAddBlend_HDA_c, do_PsAddBlend, dest, src, len);
}
static void TVPPsAddBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsAddBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsAddBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsSubBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vqsub_u8(d.val[2], vmvn_u8(s.val[2]));
	s.val[1] = vqsub_u8(d.val[1], vmvn_u8(s.val[1]));
	s.val[0] = vqsub_u8(d.val[0], vmvn_u8(s.val[0]));
	return do_AlphaBlend(s, d);
}
static void TVPPsSubBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsSubBlend_HDA_c, do_PsSubBlend, dest, src, len);
}
static void TVPPsSubBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsSubBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsSubBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsMulBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vshrn_n_u16(vmull_u8(s.val[2], d.val[2]), 8);
	s.val[1] = vshrn_n_u16(vmull_u8(s.val[1], d.val[1]), 8);
	s.val[0] = vshrn_n_u16(vmull_u8(s.val[0], d.val[0]), 8);
	return do_AlphaBlend(s, d);
}
static void TVPPsMulBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsMulBlend_HDA_c, do_PsMulBlend, dest, src, len);
}
static void TVPPsMulBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsMulBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsMulBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsScreenBlend(uint8x8x4_t s, uint8x8x4_t d) {
	uint16x8_t d_r16 = vmull_u8(s.val[2], d.val[2]);
	uint16x8_t d_g16 = vmull_u8(s.val[1], d.val[1]);
	uint16x8_t d_b16 = vmull_u8(s.val[0], d.val[0]);
	d_r16 = vsubl_u8(s.val[2], vshrn_n_u16(d_r16, 8));
	d_g16 = vsubl_u8(s.val[1], vshrn_n_u16(d_g16, 8));
	d_b16 = vsubl_u8(s.val[0], vshrn_n_u16(d_b16, 8));
	d_r16 = vmulq_u16(d_r16, vmovl_u8(s.val[3]));
	d_g16 = vmulq_u16(d_g16, vmovl_u8(s.val[3]));
	d_b16 = vmulq_u16(d_b16, vmovl_u8(s.val[3]));
	d.val[2] = vadd_u8(d.val[2], vshrn_n_u16(d_r16, 8));
	d.val[1] = vadd_u8(d.val[1], vshrn_n_u16(d_g16, 8));
	d.val[0] = vadd_u8(d.val[0], vshrn_n_u16(d_b16, 8));
	return d;
}
static void TVPPsScreenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsScreenBlend_HDA_c, do_PsScreenBlend, dest, src, len);
}
static void TVPPsScreenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsScreenBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsScreenBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsOverlayBlend(uint8x8x4_t s, uint8x8x4_t d) {
	// (s+d-s*d)*2-1, d>=128
	// s*d*2, d<128
	const uint8x8_t mask80 = vdup_n_u8(0x80);
	const uint8x8_t mask1 = vdup_n_u8(0x1);
	const uint8x8_t maskFE = vdup_n_u8(0xFE);
	for (int i = 0; i < 3; ++i) {
		uint16x8_t sa = vmull_u8(vorr_u8(s.val[i], mask1), d.val[i]);
		uint8x8_t n = vtst_u8(d.val[i], mask80); // n = d>=128
		uint8x8_t d1 = vand_u8(d.val[i], n), s1 = vand_u8(vand_u8(s.val[i], n), maskFE);
		sa = vshrq_n_u16(sa, 7);
		uint16x8_t t = vshll_n_u8(vadd_u8(s1, d1), 1);
		t = vsubw_u8(t, n);
		t = vsubq_u16(t, sa);
		s.val[i] = vand_u8(vmovn_u16(t), n);
		s.val[i] = vorr_u8(s.val[i], vand_u8(vmovn_u16(sa), vmvn_u8(n)));

// 		uint8x8_t threshold = vtst_u8(d.val[i], mask80); // = (128<=s)?0xFF:0
// 		uint16x8_t ms2 = vshlq_n_u16(vaddl_u8(s.val[i], d.val[i]), 1);	// = (dst+src)*2
// 		uint16x8_t ms = vshrq_n_u16(vmull_u8(s.val[i], d.val[i]), 7);	// = dst*src*2/255
// 		ms2 = vsubw_u8(vsubq_u16(ms2, ms), threshold);	// = (d+s-d*s)*2-255
// 		s.val[i] = vand_u8(vmovn_u16(ms2), threshold);
// 		threshold = vmvn_u8(threshold);
// 		threshold = vand_u8(vmovn_u16(ms), threshold);
// 		s.val[i] = vorr_u8(s.val[i], threshold);
	}

	return do_AlphaBlend(s, d);
}
static void TVPPsOverlayBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsOverlayBlend_HDA_c, do_PsOverlayBlend, dest, src, len);
}
static void TVPPsOverlayBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsOverlayBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsOverlayBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsHardLightBlend(uint8x8x4_t s, uint8x8x4_t d) {
	// (s+d-s*d)*2-1, s>=128
	// s*d*2, s<128
	const uint8x8_t mask80 = vdup_n_u8(0x80);
	const uint8x8_t mask1 = vdup_n_u8(0x1);
	const uint8x8_t maskFE = vdup_n_u8(0xFE);
	for (int i = 0; i < 3; ++i) {
		uint16x8_t sa = vmull_u8(vorr_u8(s.val[i], mask1), d.val[i]);
		uint8x8_t n = vtst_u8(s.val[i], mask80); // n = d>=128
		uint8x8_t d1 = vand_u8(d.val[i], n), s1 = vand_u8(vand_u8(s.val[i], n), maskFE);
		sa = vshrq_n_u16(sa, 7);
		uint16x8_t t = vshll_n_u8(vadd_u8(s1, d1), 1);
		t = vsubw_u8(t, n);
		t = vsubq_u16(t, sa);
		s.val[i] = vand_u8(vmovn_u16(t), n);
		s.val[i] = vorr_u8(s.val[i], vand_u8(vmovn_u16(sa), vmvn_u8(n)));

// 		uint8x8_t threshold = vtst_u8(s.val[i], mask80); // = (128<=s)?0xFF:0
// 		uint16x8_t ms2 = vshlq_n_u16(vaddl_u8(s.val[i], d.val[i]), 1);	// = (dst+src)*2
// 		uint16x8_t ms = vshrq_n_u16(vmull_u8(s.val[i], d.val[i]), 7);	// = dst*src*2/255
// 		ms2 = vqsubq_u16(vsubq_u16(ms2, ms), maskFF);	// = (d+s-d*s)*2-255
// 		s.val[i] = vand_u8(vmovn_u16(ms2), threshold);
// 		threshold = vmvn_u8(threshold);
// 		threshold = vand_u8(vmovn_u16(ms), threshold);
// 		s.val[i] = vorr_u8(s.val[i], threshold);
	}

	return do_AlphaBlend(s, d);
}
static void TVPPsHardLightBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsHardLightBlend_HDA_c, do_PsHardLightBlend, dest, src, len);
}
static void TVPPsHardLightBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsHardLightBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsHardLightBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsLightenBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vmax_u8(s.val[2], d.val[2]);
	s.val[1] = vmax_u8(s.val[1], d.val[1]);
	s.val[0] = vmax_u8(s.val[0], d.val[0]);
	return do_AlphaBlend(s, d);
}
static void TVPPsLightenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsLightenBlend_HDA_c, do_PsLightenBlend, dest, src, len);
}
static void TVPPsLightenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsLightenBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsLightenBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsDarkenBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vmin_u8(s.val[2], d.val[2]);
	s.val[1] = vmin_u8(s.val[1], d.val[1]);
	s.val[0] = vmin_u8(s.val[0], d.val[0]);
	return do_AlphaBlend(s, d);
}
static void TVPPsDarkenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsDarkenBlend_HDA_c, do_PsDarkenBlend, dest, src, len);
}
static void TVPPsDarkenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsDarkenBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsDarkenBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsDiff5Blend(uint8x8x4_t s, uint8x8x4_t d) {
	uint16x8_t s_r16 = vmull_u8(s.val[2], s.val[3]);
	uint16x8_t s_g16 = vmull_u8(s.val[1], s.val[3]);
	uint16x8_t s_b16 = vmull_u8(s.val[0], s.val[3]);
	d.val[2] = vabd_u8(vshrn_n_u16(s_r16, 8), d.val[2]);
	d.val[1] = vabd_u8(vshrn_n_u16(s_g16, 8), d.val[1]);
	d.val[0] = vabd_u8(vshrn_n_u16(s_b16, 8), d.val[0]);
	return d;
}
static void TVPPsDiff5Blend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsDiff5Blend_HDA_c, do_PsDiff5Blend, dest, src, len);
}
static void TVPPsDiff5Blend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsDiff5Blend_HDA_o_c, do_PsAlphaBlend_so<do_PsDiff5Blend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsDiffBlend(uint8x8x4_t s, uint8x8x4_t d) {
	s.val[2] = vabd_u8(s.val[2], d.val[2]);
	s.val[1] = vabd_u8(s.val[1], d.val[1]);
	s.val[0] = vabd_u8(s.val[0], d.val[0]);
	return do_AlphaBlend(s, d);
}
static void TVPPsDiffBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsDiffBlend_HDA_c, do_PsDiffBlend, dest, src, len);
}
static void TVPPsDiffBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsDiffBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsDiffBlend>, dest, src, len, opa);
}

static uint8x8x4_t do_PsExclusionBlend(uint8x8x4_t s, uint8x8x4_t d) {
	// c = ((s+d-(s*d*2)/255)-d)*a + d = (s-(s*d*2)/255)*a + d
	uint16x8_t d_r16 = vmull_u8(s.val[2], d.val[2]);
	uint16x8_t d_g16 = vmull_u8(s.val[1], d.val[1]);
	uint16x8_t d_b16 = vmull_u8(s.val[0], d.val[0]);
	d_r16 = vsubq_u16(vmovl_u8(s.val[2]), vshrq_n_u16(d_r16, 7));
	d_g16 = vsubq_u16(vmovl_u8(s.val[1]), vshrq_n_u16(d_g16, 7));
	d_b16 = vsubq_u16(vmovl_u8(s.val[0]), vshrq_n_u16(d_b16, 7));
	d_r16 = vmulq_u16(d_r16, vmovl_u8(s.val[3]));
	d_g16 = vmulq_u16(d_g16, vmovl_u8(s.val[3]));
	d_b16 = vmulq_u16(d_b16, vmovl_u8(s.val[3]));
	d.val[2] = vadd_u8(d.val[2], vshrn_n_u16(d_r16, 8));
	d.val[1] = vadd_u8(d.val[1], vshrn_n_u16(d_g16, 8));
	d.val[0] = vadd_u8(d.val[0], vshrn_n_u16(d_b16, 8));
	return d;
}
static void TVPPsExclusionBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
	do_blend(TVPPsExclusionBlend_HDA_c, do_PsExclusionBlend, dest, src, len);
}
static void TVPPsExclusionBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
	do_blend(TVPPsExclusionBlend_HDA_o_c, do_PsAlphaBlend_so<do_PsExclusionBlend>, dest, src, len, opa);
}
#endif

#if TVP_TLG6_W_BLOCK_SIZE != 8
#error TVP_TLG6_W_BLOCK_SIZE must be 8 !
#endif

static uint8x8x4_t filter_insts_0_neon(uint8x8x4_t m) { return m; }
// ( 1, IB+IG, IG, IR+IG)
static uint8x8x4_t filter_insts_1_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	m.val[2] = vadd_u8(m.val[2], m.val[1]);
	return m;
}
// ( 2, IB, IG+IB, IR+IB+IG)
static uint8x8x4_t filter_insts_2_neon(uint8x8x4_t m) {
	m.val[1] = vadd_u8(m.val[1], m.val[0]);
	m.val[2] = vadd_u8(m.val[2], m.val[1]);
	return m;
}
// ( 3, IB+IR+IG, IG+IR, IR)
static uint8x8x4_t filter_insts_3_neon(uint8x8x4_t m) {
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	return m;
}
// ( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG)
static uint8x8x4_t filter_insts_4_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[2]);
	m.val[1] = vadd_u8(m.val[1], m.val[0]);
	m.val[2] = vadd_u8(m.val[2], m.val[1]);
	return m;
}
// ( 5, IB+IR, IG+IB+IR, IR)
static uint8x8x4_t filter_insts_5_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[2]);
	m.val[1] = vadd_u8(m.val[1], m.val[0]);
	return m;
}
// ( 6, IB+IG, IG, IR)
static uint8x8x4_t filter_insts_6_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	return m;
}
// ( 7, IB, IG+IB, IR)
static uint8x8x4_t filter_insts_7_neon(uint8x8x4_t m) {
	m.val[1] = vadd_u8(m.val[1], m.val[0]);
	return m;
}
// ( 8, IB, IG, IR+IG)
static uint8x8x4_t filter_insts_8_neon(uint8x8x4_t m) {
	m.val[2] = vadd_u8(m.val[2], m.val[1]);
	return m;
}
// ( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB)
static uint8x8x4_t filter_insts_9_neon(uint8x8x4_t m) {
	m.val[2] = vadd_u8(m.val[2], m.val[0]);
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	return m;
}
// (10, IB+IR, IG+IR, IR)
static uint8x8x4_t filter_insts_10_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[2]);
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	return m;
}
// (11, IB, IG+IB, IR+IB)
static uint8x8x4_t filter_insts_11_neon(uint8x8x4_t m) {
	m.val[1] = vadd_u8(m.val[1], m.val[0]);
	m.val[2] = vadd_u8(m.val[2], m.val[0]);
	return m;
}
// (12, IB, IG+IR+IB, IR+IB)
static uint8x8x4_t filter_insts_12_neon(uint8x8x4_t m) {
	m.val[2] = vadd_u8(m.val[2], m.val[0]);
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	return m;
}
// (13, IB+IG, IG+IR+IB+IG, IR+IB+IG)
static uint8x8x4_t filter_insts_13_neon(uint8x8x4_t m) {
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	m.val[2] = vadd_u8(m.val[2], m.val[0]);
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	return m;
}
// (14, IB+IG+IR, IG+IR, IR+IB+IG+IR)
static uint8x8x4_t filter_insts_14_neon(uint8x8x4_t m) {
	m.val[1] = vadd_u8(m.val[1], m.val[2]);
	m.val[0] = vadd_u8(m.val[0], m.val[1]);
	m.val[2] = vadd_u8(m.val[2], m.val[0]);
	return m;
}
// (15, IB, IG+(IB<<1), IR+(IB<<1))
static uint8x8x4_t filter_insts_15_neon(uint8x8x4_t m) {
	uint8x8_t t = vshl_n_u8(m.val[0], 1);
	m.val[1] = vadd_u8(m.val[1], t);
	m.val[2] = vadd_u8(m.val[2], t);
	return m;
}

static uint8x8x4_t tlg6_forward_input_neon(uint32_t* in) {
	return vld4_u8((uint8_t*)__builtin_assume_aligned(in, 8));
}

static uint8x8x4_t tlg6_backward_input_neon(uint32_t* in) {
	uint8x8x4_t ret = vld4_u8((uint8_t*)__builtin_assume_aligned(in, 8));
	ret.val[0] = vrev64_u8(ret.val[0]);
	ret.val[1] = vrev64_u8(ret.val[1]);
	ret.val[2] = vrev64_u8(ret.val[2]);
	ret.val[3] = vrev64_u8(ret.val[3]);
	return ret;
}

static inline uint8x8x4_t do_unpack_pixel_rgba(uint8x8x4_t minput) {
	// BGRA -> RGBA
	minput.val[0] = veor_u8(minput.val[0], minput.val[2]);
	minput.val[2] = veor_u8(minput.val[2], minput.val[0]);
	minput.val[0] = veor_u8(minput.val[0], minput.val[2]);

	uint8x8x2_t m01 = vtrn_u8(minput.val[0], minput.val[1]);
	uint8x8x2_t m23 = vtrn_u8(minput.val[2], minput.val[3]);
	uint16x8x2_t m = vtrnq_u16(
		vreinterpretq_u16_u8(vcombine_u8(m01.val[0], m01.val[1])),
		vreinterpretq_u16_u8(vcombine_u8(m23.val[0], m23.val[1])));
	minput.val[0] = vreinterpret_u8_u16(vget_low_u16(m.val[0]));
	minput.val[1] = vreinterpret_u8_u16(vget_high_u16(m.val[0]));
	minput.val[2] = vreinterpret_u8_u16(vget_low_u16(m.val[1]));
	minput.val[3] = vreinterpret_u8_u16(vget_high_u16(m.val[1]));
	return minput;
}

/*
 +---+---+
 |lt | t |        / min(l, t), if lt >= max(l, t);
 +---+---+  ret = | max(l, t), if lt >= min(l, t);
 | l |ret|        \ l + t - lt, otherwise;
 +---+---+
*/
static inline uint8x8_t do_med_neon(uint8x8_t a, uint8x8_t b, const uint8x8_t& c, uint8x8_t v) {
	uint8x8_t a2 = a;
	a = vmax_u8(a, b);	// = max_a_b
	b = vmin_u8(b, a2);	// = min_a_b
	v = vadd_u8(v, a);
	a = vmin_u8(a, c);	// = max_a_b < c ? max_a_b : c
	v = vadd_u8(v, b);
	a = vmax_u8(a, b);	// = min_a_b < a ? a : min_a_b
	return vsub_u8(v, a);
}

#define vshr_n_u8_64(s, n) vreinterpret_u8_u64(vshr_n_u64(vreinterpret_u64_u8(s), n))

template<
	uint8x8x4_t filter(uint8x8x4_t),
	uint8x8x4_t input(uint32_t *)>
static inline void do_filter_med_neon(uint32_t& inp, uint32_t& inup, uint32_t *in, uint32_t *prevline, uint32_t *curline) {
	uint8x8_t p = vreinterpret_u8_u32(vdup_n_u32(inp));
	uint8x8_t up = vreinterpret_u8_u32(vdup_n_u32(inup));
	uint8x8x4_t minput = input(in);
	minput = filter(minput);
	minput = do_unpack_pixel_rgba(minput);
	uint8x8_t u;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_med_neon(p, u, up, minput.val[0]);
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_med_neon(p, u, up, minput.val[1]);
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_med_neon(p, u, up, minput.val[2]);
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_med_neon(p, u, up, minput.val[3]);
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_med_neon(p, u, up, vshr_n_u8_64(minput.val[0], 32));
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_med_neon(p, u, up, vshr_n_u8_64(minput.val[1], 32));
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_med_neon(p, u, up, vshr_n_u8_64(minput.val[2], 32));
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_med_neon(p, u, up, vshr_n_u8_64(minput.val[3], 32));
	up = u;
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	inp = curline[-1];
	inup = vget_lane_u32(vreinterpret_u32_u8(u), 0);
}

static inline uint8x8_t do_avg_neon(const uint8x8_t &a, const uint8x8_t &b, const uint8x8_t &v) {
	return vadd_u8(vrhadd_u8(a, b), v);
}

template<
	uint8x8x4_t filter(uint8x8x4_t),
	uint8x8x4_t input(uint32_t *)>
inline void do_filter_avg_neon(tjs_uint32& inp, tjs_uint32& up, tjs_uint32 *in, tjs_uint32 *prevline, tjs_uint32 *curline) {
	uint8x8_t p = vreinterpret_u8_u32(vdup_n_u32(inp));
	uint8x8x4_t minput = input(in);
	minput = filter(minput);
	minput = do_unpack_pixel_rgba(minput);
	uint8x8_t u;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_avg_neon(p, u, minput.val[0]);
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_avg_neon(p, u, minput.val[1]);
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_avg_neon(p, u, minput.val[2]);
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_avg_neon(p, u, minput.val[3]);
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_avg_neon(p, u, vshr_n_u8_64(minput.val[0], 32));
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_avg_neon(p, u, vshr_n_u8_64(minput.val[1], 32));
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	u = vld1_u8((uint8_t*)__builtin_assume_aligned(prevline, 8));
	p = do_avg_neon(p, u, vshr_n_u8_64(minput.val[2], 32));
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);

	u = vshr_n_u8_64(u, 32);
	p = do_avg_neon(p, u, vshr_n_u8_64(minput.val[3], 32));
	*curline++ = vget_lane_u32(vreinterpret_u32_u8(p), 0);
	prevline += 2;

	inp = curline[-1];
	up = vget_lane_u32(vreinterpret_u32_u8(u), 0);
}

/*
	chroma/luminosity decoding
	(this does reordering, color correlation filter, MED/AVG  at a time)
*/
static void TVPTLG6DecodeLine_NEON(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width/*, tjs_int start_block*/, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
{
// 	std::vector<tjs_uint32> tmp; tmp.resize(TVP_TLG6_W_BLOCK_SIZE * (block_limit - start_block)); tjs_uint32* _curline = curline;
// 	TVPTLG6DecodeLine_c(prevline, &tmp.front(), width, start_block, block_limit, filtertypes, skipblockbytes, in, initialp, oddskip, dir);
	tjs_int start_block = 0;
	uint32_t p, up;

	if(start_block)
	{
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p = curline[-1];
		up = prevline[-1];
	}
	else
	{
		p = up = initialp;
	}

	oddskip *= TVP_TLG6_W_BLOCK_SIZE;	// oddskip * 8
	if (dir & 1) {
		// forward
		skipblockbytes -= TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		for (int i = start_block; i < block_limit; i++) {
			if (i & 1) {
				in += oddskip;
			} else {
				in -= oddskip;
			}
			switch (filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_FORWARD(N) \
	case (N<<1)+0: do_filter_med_neon<filter_insts_##N##_neon, tlg6_forward_input_neon>(p, up, in, prevline, curline); break;\
	case (N<<1)+1: do_filter_avg_neon<filter_insts_##N##_neon, tlg6_forward_input_neon>(p, up, in, prevline, curline); break;
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(0);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(1);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(2);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(3);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(4);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(5);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(6);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(7);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(8);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(9);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_FORWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_FORWARD
			}
			prevline += 8; curline += 8; in += 8;
			in += skipblockbytes;
		}
	} else {
		// backward
		skipblockbytes += TVP_TLG6_W_BLOCK_SIZE;
		in += skipblockbytes * start_block;
		in += oddskip;
		//in += (TVP_TLG6_W_BLOCK_SIZE - 1);
		in += TVP_TLG6_W_BLOCK_SIZE;
		for (int i = start_block; i < block_limit; i++) {
			if (i & 1) {
				in += oddskip;
			} else{
				in -= oddskip;
			}
			in -= 8;
			switch (filtertypes[i]) {
#define TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(N) \
	case (N<<1)+0: do_filter_med_neon<filter_insts_##N##_neon, tlg6_backward_input_neon>(p, up, in, prevline, curline); break;\
	case (N<<1)+1: do_filter_avg_neon<filter_insts_##N##_neon, tlg6_backward_input_neon>(p, up, in, prevline, curline); break;
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(0);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(1);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(2);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(3);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(4);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(5);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(6);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(7);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(8);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(9);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(10);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(11);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(12);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(13);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(14);
				TVP_TLG6_DO_CHROMA_DECODE_BACKWARD(15);
#undef TVP_TLG6_DO_CHROMA_DECODE_BACKWARD
			}
			prevline += 8; curline += 8;
			in += skipblockbytes;
		}
	}

// 	for (int i = 0; i < tmp.size(); ++i) {
// 		assert(tmp[i] == _curline[i]);
// 	}
}

static void TVPTLG5ComposeColors3To4_NEON(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width)
{
    const tjs_uint8 * p2 = buf[0];
    const tjs_uint8 * p1 = buf[1];
    const tjs_uint8 * p0 = buf[2];
    int x = 0;
    uint8x8x3_t pc;
    pc.val[0] = vdup_n_u8(0);
    pc.val[1] = vdup_n_u8(0);
    pc.val[2] = vdup_n_u8(0);
    uint8x8x4_t rgba;
    rgba.val[3] = vdup_n_u8(0xFF);
    for(x = 0; x < width - 7; x += 8) {
        uint8x8x3_t c;
        c.val[1] = vld1_u8(p1 + x);
        c.val[0] = vadd_u8(vld1_u8(p0 + x), c.val[1]);
        c.val[2] = vadd_u8(vld1_u8(p2 + x), c.val[1]);
        pc.val[0] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[0], 7)), c.val[0]);
        pc.val[1] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[1], 7)), c.val[1]);
        pc.val[2] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[2], 7)), c.val[2]);
        for(int i = 0; i < 7; ++i) {
            c.val[0] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[0]), 8));
            c.val[1] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[1]), 8));
            c.val[2] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[2]), 8));
            pc.val[0] = vadd_u8(pc.val[0], c.val[0]);
            pc.val[1] = vadd_u8(pc.val[1], c.val[1]);
            pc.val[2] = vadd_u8(pc.val[2], c.val[2]);
        }
		uint8x8x4_t up = vld4_u8((uint8_t*)__builtin_assume_aligned(upper, 8));
        rgba.val[0] = vadd_u8(pc.val[0], up.val[0]);
        rgba.val[1] = vadd_u8(pc.val[1], up.val[1]);
        rgba.val[2] = vadd_u8(pc.val[2], up.val[2]);
		vst4_u8((uint8_t*)__builtin_assume_aligned(outp, 8), rgba);
        outp += 4 * 8;
        upper += 4 * 8;
    }

    if(x < width) {
        tjs_uint8 _pc[3];
        tjs_uint8 _c[3];
        _pc[0] = vget_lane_u8(pc.val[0], 7);
        _pc[1] = vget_lane_u8(pc.val[1], 7);
        _pc[2] = vget_lane_u8(pc.val[2], 7);
        for(; x < width; x++)
        {
            _c[0] = p0[x];
            _c[1] = p1[x];
            _c[2] = p2[x];
            _c[0] += _c[1]; _c[2] += _c[1];
            *(tjs_uint32 *)outp =
                ((((_pc[0] += _c[0]) + upper[0]) & 0xff)      ) +
                ((((_pc[1] += _c[1]) + upper[1]) & 0xff) <<  8) +
                ((((_pc[2] += _c[2]) + upper[2]) & 0xff) << 16) +
                0xff000000;
            outp += 4;
            upper += 4;
        }
    }
}

static void TVPTLG5ComposeColors4To4_NEON(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width)
{
#ifdef TEST_ARM_NEON_CODE
	TVPTLG5ComposeColors4To4_c(outp, upper, buf, width);
	tjs_uint8 *orig_outp = outp;
	tjs_uint8 *test_outp = outp = new tjs_uint8[width * 4];
#endif
    const tjs_uint8 * p2 = buf[0];
    const tjs_uint8 * p1 = buf[1];
    const tjs_uint8 * p0 = buf[2];
    const tjs_uint8 * p3 = buf[3];
    int x = 0;
    uint8x8x4_t pc;
    pc.val[0] = vdup_n_u8(0);
    pc.val[1] = vdup_n_u8(0);
    pc.val[2] = vdup_n_u8(0);
    pc.val[3] = vdup_n_u8(0);
    for(x = 0; x < width - 7; x += 8) {
        uint8x8x4_t c;
        c.val[1] = vld1_u8(p1 + x);
        c.val[0] = vadd_u8(vld1_u8(p0 + x), c.val[1]);
        c.val[2] = vadd_u8(vld1_u8(p2 + x), c.val[1]);
        c.val[3] = vld1_u8(p3 + x);
        pc.val[0] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[0], 7)), c.val[0]);
        pc.val[1] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[1], 7)), c.val[1]);
        pc.val[2] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[2], 7)), c.val[2]);
        pc.val[3] = vadd_u8(vdup_n_u8(vget_lane_u8(pc.val[3], 7)), c.val[3]);
        for(int i = 0; i < 7; ++i) {
            c.val[0] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[0]), 8));
            c.val[1] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[1]), 8));
            c.val[2] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[2]), 8));
            c.val[3] = vreinterpret_u8_u64(vshl_n_u64(vreinterpret_u64_u8(c.val[3]), 8));
            pc.val[0] = vadd_u8(pc.val[0], c.val[0]);
            pc.val[1] = vadd_u8(pc.val[1], c.val[1]);
            pc.val[2] = vadd_u8(pc.val[2], c.val[2]);
            pc.val[3] = vadd_u8(pc.val[3], c.val[3]);
        }
		uint8x8x4_t up = vld4_u8((uint8_t*)__builtin_assume_aligned(upper, 8));
        uint8x8x4_t rgba;
        rgba.val[0] = vadd_u8(pc.val[0], up.val[0]);
        rgba.val[1] = vadd_u8(pc.val[1], up.val[1]);
        rgba.val[2] = vadd_u8(pc.val[2], up.val[2]);
        rgba.val[3] = vadd_u8(pc.val[3], up.val[3]);
		vst4_u8((uint8_t*)__builtin_assume_aligned(outp, 8), rgba);
        outp += 4 * 8;
        upper += 4 * 8;
    }

    if(x < width) {
        tjs_uint8 _pc[4];
        tjs_uint8 _c[4];
        _pc[0] = vget_lane_u8(pc.val[0], 7);
        _pc[1] = vget_lane_u8(pc.val[1], 7);
        _pc[2] = vget_lane_u8(pc.val[2], 7);
        _pc[3] = vget_lane_u8(pc.val[3], 7);
        for(; x < width; x++)
        {
            _c[0] = p0[x];
            _c[1] = p1[x];
            _c[2] = p2[x];
            _c[3] = p3[x];
            _c[0] += _c[1]; _c[2] += _c[1];
            *(tjs_uint32 *)outp =
                ((((_pc[0] += _c[0]) + upper[0]) & 0xff)      ) +
                ((((_pc[1] += _c[1]) + upper[1]) & 0xff) <<  8) +
                ((((_pc[2] += _c[2]) + upper[2]) & 0xff) << 16) +
                ((((_pc[3] += _c[3]) + upper[3]) & 0xff) << 24);
            outp += 4;
            upper += 4;
        }
    }
#ifdef TEST_ARM_NEON_CODE
	for (int i = 0; i < width * 4; ++i) {
		assert(test_outp[i] == orig_outp[i]);
	}
	delete[]test_outp;
#endif
}

static tjs_int TVPTLG5DecompressSlide_NEON(tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr) {
	// test
// 	std::vector<tjs_uint8> tmp; tmp.resize(1024 * 768 * 4);
// 	std::vector<tjs_uint8> ttext; ttext.insert(ttext.begin(), text, text + 4096 + 16);
// 	tjs_uint8 *pout = out;
// 	tjs_int rr = TVPTLG5DecompressSlide_c(&tmp[0], in, insize, &ttext[0], initialr);
	
	tjs_int r = initialr;
	tjs_uint flags = 0;
	const tjs_uint8 *inlim = in + insize;
	while (in < inlim) {
		if (((flags >>= 1) & 256) == 0) {
			flags = in[0] | 0xff00;
			in++;
			if (flags == 0xff00 && r < (4096 - 8) && in < (inlim - 8)) {	// copy 8byte
				uint8x8_t c = vld1_u8(in);
				vst1_u8(out, c);;
				vst1_u8(&text[r], c);;
				r += 8;
				in += 8;
				out += 8;
				flags = 0;
				continue;
			}
		}
		if (flags & 1) {
			tjs_uint16 in16 = *(tjs_uint16*)in;
			tjs_uint mpos = in16 & 0xFFF;
			tjs_uint mlen = (in16 >> 12) + 3;
			in += 2;
			if (mlen == 18)
				mlen += *in++;
 			if (mlen > 15 && (mpos - r > 15 || r - mpos > 15)) {
				if ((mpos + mlen) < 4096 && (r + mlen) < 4096) {
					tjs_int count = mlen >> 4;
					while (count--) {
						uint8x16_t c = vld1q_u8(&text[mpos]);
						vst1q_u8(out, c);
						vst1q_u8(&text[r], c);
						mpos += 16; r += 16; out += 16;
					}
					mlen &= 0x0f;	// ำเค๊
					while (mlen--) {
						out[0] = text[r++] = text[mpos++]; out++;
					}
					continue;
				}
#if 0
				while (mlen) {
					uint8x16_t c = vld1q_u8(&text[mpos]);
					vst1q_u8(out, c);
					vst1q_u8(&text[r], c); // direct write to text is OK due to the extra 16 bytes
					tjs_int next = mlen < 16 ? mlen : 16;
					if (mpos + next > 4095) {
						next = 4096 - mpos;
						mpos = 0;
					} else {
						mpos += next;
					}
					out += next;
					r += next;
					mlen -= next;
					if (r > 4095) {
						r &= 0x0fff;
						vst1q_u8(&text[r - 16], c);
					}
				}
				continue;
#endif
			}
			while (mlen--) {
				out[0] = text[r++] = text[mpos++]; out++;
				mpos &= 0x0fff;
				r &= 0x0fff;
			}
		} else {
			unsigned char c = in[0]; in++;
			out[0] = c; out++;
			text[r++] = c;
			r &= 0x0fff;
		}
	}

	// test
// 	assert(rr == r);
// 	for (int i = 0; i < out - pout; ++i) {
// 		assert(tmp[i] == pout[i]);
// 	}

	return r;
}

//#include <cpu-features.h>

static tjs_uint32 *testbuff = NULL;
static tjs_uint32 *testdata1 = NULL;
static tjs_uint32 *testdata2 = NULL;
static tjs_uint32 *testdest1 = NULL;
static tjs_uint32 *testdest2 = NULL;
static tjs_uint32 *testtable = NULL;
static tjs_uint8  *testrule = NULL;
#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
#define FUNC_API extern "C"
#else
#define FUNC_API
#endif
FUNC_API int TVPShowSimpleMessageBox(const char * text, const char * caption, unsigned int nButton, const char **btnText); // C-style
tjs_uint32 TVPGetRoughTickCount32();

static void ShowInMessageBox(const char *text) {
	if (!text || !*text) return;
	const char *btnText = "OK";
	TVPShowSimpleMessageBox(text, "Log", 1, &btnText);
}

static void InitTestData() {
    if(!testtable) {
        testtable = (tjs_uint32*)malloc(256 * sizeof(tjs_uint32));
        for(int x = 0; x < 256; ++x) {
            testtable[x] = rand() & 0xFF;
        }
        testrule = (tjs_uint8*)malloc(256 * 256);
        for(int x = 0; x < 256 * 256; ++x) {
            testrule[x] = rand() & 0xFF;
        }
		testbuff = (tjs_uint32*)malloc((256 * 256 * 4 + 2) * sizeof(tjs_uint32));
		testdest1 = testbuff;
		testdest2 = testdest1 + 256 * 256;
		testdata1 = testdest2 + 256 * 256;
        testdata2 = testdata1 + 256 * 256;
    }
	int obfu = 65531;
    for(int x = 0; x < 256; ++x) {
        for(int y = 0; y < 256; ++y) {
            typedef struct {
                unsigned char a;
                unsigned char r;
                unsigned char g;
                unsigned char b;
            } clr;
            clr *clr1 = (clr*)(testdata1 + 256 * y + x),
                *clr2 = (clr*)(testdata2 + 256 * y + x);
            clr1->a = 255 - x; clr2->a = 255 - y;
            clr1->r = x; clr2->r = y;
            clr1->g = y; clr2->g = 255 - x;
            clr1->b = 255 - y; clr2->b = x;
			if (y == 0) {
				clr1->a = obfu;
				obfu = obfu * 3 + 29;
				clr2->a = obfu;
				obfu = obfu * 3 + 29;
				clr1->r = obfu;
				obfu = obfu * 3 + 29;
				clr2->r = obfu;
				obfu = obfu * 3 + 29;
				clr1->g = obfu;
				obfu = obfu * 3 + 29;
				clr2->g = obfu;
				obfu = obfu * 3 + 29;
				clr1->b = obfu;
				obfu = obfu * 3 + 29;
				clr2->b = obfu;
				obfu = obfu * 3 + 29;
        }
    }
    }
    memcpy(testdest1, testdata2, 256 * 256 * 4);
    memcpy(testdest2, testdata2, 256 * 256 * 4);
}

#if defined(TEST_ARM_NEON_CODE) || defined(LOG_NEON_TEST)
static void CheckTestData(const char *pszFuncName)
{
	typedef union{
		struct {
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
		unsigned long u32;
	} clr; clr clr1, clr2;
	for (int i = 0; i < 256 * 256; ++i) {
		clr1.u32 = testdest1[i];
		clr2.u32 = testdest2[i];
		if (clr1.a <= 1 && clr2.a <= 1) continue;
		if (abs(clr1.a - clr2.a) > 2 ||
			abs(clr1.r - clr2.r) > 2 ||
			abs(clr1.g - clr2.g) > 2 ||
			abs(clr1.b - clr2.b) > 2)
		{
			char tmp[256];
			sprintf(tmp, "test fail on function %s", pszFuncName);
#ifdef _MSC_VER
			cv::Mat test1(256, 256, CV_8UC4, testdest1, 1024);
			cv::Mat test2(256, 256, CV_8UC4, testdest2, 1024);
#endif
			ShowInMessageBox(tmp);
#if !defined(WIN32) && 0
			const char bmphdr[] = "\x42\x4D\x36\x00\x04\x00\x00\x00\x00\x00\x36\x00\x00\x00\x28\x00\x00\x00\x00\x01\x00\x00\x00\x01\x00\x00\x01\x00\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x12\x0B\x00\x00\x12\x0B\x00\x00\x00\x00\x00\x00\x00\x00\x00";
			FILE* f = fopen("/sdcard/testdest1.bmp", "wb");
			fwrite(bmphdr, sizeof(bmphdr), 1, f);
			fwrite(testdest1, 256 * 256, 4, f);
			fclose(f);
			f = fopen("/sdcard/testdest2.bmp", "wb");
			fwrite(bmphdr, sizeof(bmphdr), 1, f);
			fwrite(testdest2, 256 * 256, 4, f);
			fclose(f);
#endif
			return;
		}
	}
	//SDL_Log("cheking %s pass", pszFuncName);
}
#endif
static void CheckTestData_RGB(const char *pszFuncName)
{
	typedef union{
		struct {
			unsigned char r;
			unsigned char g;
			unsigned char b;
			unsigned char a;
		};
		unsigned long u32;
	} clr; clr clr1, clr2;
	for (int i = 0; i < 256 * 256; ++i) {
		clr1.u32 = testdest1[i];
		clr2.u32 = testdest2[i];
		if (abs(clr1.r - clr2.r) > 2 ||
			abs(clr1.g - clr2.g) > 2 ||
			abs(clr1.b - clr2.b) > 2)
		{
			char tmp[256];
			sprintf(tmp, "test fail on function %s", pszFuncName);
#ifdef _MSC_VER
			cv::Mat test1(256, 256, CV_8UC4, testdest1, 1024);
			cv::Mat test2(256, 256, CV_8UC4, testdest2, 1024);
#endif
			ShowInMessageBox(tmp);
			//assert(!pszFuncName);
			return;
		}
	}
	//SDL_Log("cheking %s pass", pszFuncName);
}

static void testTLG6_chroma()
{
	tjs_uint8 tmpbuff[(32 + 256) * 4 * 2 + 16];
	tjs_uint8 *block_src_ref = (tjs_uint8*)((((intptr_t)tmpbuff) + 7) & ~7);
	tjs_uint8 *block_src = block_src_ref + 32 * 4;
	tjs_uint32 *testdest1 = (tjs_uint32*)(block_src + 32 * 4);
	tjs_uint32 *testdest2 = testdest1 + 256;
	tjs_uint8 *psrc[4] = {
		block_src,
		block_src + 1,
		block_src + 3,
		block_src + 2,
	};
	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLine_NEON((tjs_uint32 *)block_src_ref, testdest1, 64, /*0,*/ 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
			TVPTLG6DecodeLineGeneric_c((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
			if (memcmp(testdest1, testdest2, 8 * 4) != 0) {
				ShowInMessageBox("test fail on function TVPTLG6DecodeLineGeneric");
				assert(0);
			}
		}

		TVPTLG5ComposeColors3To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		TVPTLG5ComposeColors3To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		if (memcmp(testdest1, testdest2, 8 * 4) != 0) {
			ShowInMessageBox("test fail on function TVPTLG5ComposeColors3To4");
			assert(0);
		}
		TVPTLG5ComposeColors4To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		TVPTLG5ComposeColors4To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		if (memcmp(testdest1, testdest2, 8 * 4) != 0) {
			ShowInMessageBox("test fail on function TVPTLG5ComposeColors4To4");
			assert(0);
		}
	}
}

#ifdef LOG_NEON_TEST
#define SHOW_AND_CLEAR_LOG ShowInMessageBox(LogData); pLogData = LogData;
#else
#define SHOW_AND_CLEAR_LOG
#endif

#ifdef TEST_ARM_NEON_CODE

#define REGISTER_TVPGL_BLEND_FUNC_2(origf, f) \
	InitTestData();\
	origf##_c(testdest2, testdata1, 256 * 256);\
	f = f##_NEON;\
	f##_NEON(testdest1, testdata1, 256 * 256);\
	CheckTestData(#f);
#define REGISTER_TVPGL_BLEND_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, testdata1, 256 * 256, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	CheckTestData(#f);
#define REGISTER_TVPGL_STRECH_FUNC_2(origf, f) \
	InitTestData();\
	origf##_c(testdest2, 16 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 16 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f);
#define REGISTER_TVPGL_STRECH_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, 16 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, 16 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	CheckTestData(#f);
#define REGISTER_TVPGL_LINTRANS_FUNC_2(origf, f) \
	InitTestData();\
	origf##_c(testdest2, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64);\
	f = f##_NEON;\
	f##_NEON(testdest1, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64);\
	CheckTestData(#f);
#define REGISTER_TVPGL_LINTRANS_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64, __VA_ARGS__);\
    CheckTestData(#f);
#define REGISTER_TVPGL_UNIVTRANS_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
    CheckTestData_RGB(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, __VA_ARGS__);\
    CheckTestData(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC_RGB(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, __VA_ARGS__);\
    CheckTestData_RGB(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC_TYPE(origf, f, DT, ...) \
    InitTestData();\
    origf##_c((DT)testdest2, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON((DT)testdest1, __VA_ARGS__);\
    CheckTestData(#f);
#else
#ifdef LOG_NEON_TEST

static tjs_uint32 lastTick1, lastTick2;
static tjs_int tickC, tickNEON;
static unsigned int LogDataSize = 1024;
static char *LogData, *pLogData;

static void AddLog(const char *format, ...) {
	va_list args;
	va_start(args, format);
	char buf[256];
	vsnprintf(buf, 256 - 3, format, args);
	char *p = buf;
	if (!LogData) {
		LogData = (char*)TJS_malloc(LogDataSize);
		pLogData = LogData;
	}

	while (*p) {
		if (LogData + LogDataSize - pLogData <= 2) {
			int used = pLogData - LogData;
			LogDataSize += 1024;
			LogData = (char*)TJS_realloc(LogData, LogDataSize);
			pLogData = LogData + used;
		}
		*pLogData++ = *p++;
	}
	*pLogData++ = '\n';
	*pLogData = '\0';


	va_end(args);
}
#ifdef _MSC_VER
#define TEST_COUNT 0
#else
#define TEST_COUNT 200
#endif
#include "tjsCommHead.h"
#include "GraphicsLoaderIntf.h"
#include "UtilStreams.h"
#include "LayerBitmapIntf.h"
#include "LayerBitmapImpl.h"
#define TVP_clNone				((tjs_uint32)(0x1fffffff))
void TVPLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_int keyidx, tTVPGraphicLoadMode mode);
static void logTLG6_chroma() {
	if (!TEST_COUNT) return;
	tjs_uint8 tmpbuff[(32 + 256) * 4 * 2 + 16];
	tjs_uint8 *block_src_ref = (tjs_uint8*)((((intptr_t)tmpbuff) + 7) & ~7);
	tjs_uint8 *block_src = block_src_ref + 32 * 4;
	tjs_uint32 *testdest1 = (tjs_uint32*)(block_src + 32 * 4);
	tjs_uint32 *testdest2 = testdest1 + 256;
	tjs_uint8 *psrc[4] = {
		block_src,
		block_src + 1,
		block_src + 3,
		block_src + 2,
	};
	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 4; ++n)
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLineGeneric_c((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
		}
		tickC += TVPGetRoughTickCount32() - lastTick1;
		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 4; ++n)
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLine_NEON((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
		}
		tickNEON += TVPGetRoughTickCount32() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG6DecodeLineGeneric", tickC, tickNEON, (float)tickNEON / tickC * 100);

	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}

		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors3To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		tickC += TVPGetRoughTickCount32() - lastTick1;
		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors3To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		tickNEON += TVPGetRoughTickCount32() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG5ComposeColors3To4", tickC, tickNEON, (float)tickNEON / tickC * 100);

	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}

		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors4To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		tickC += TVPGetRoughTickCount32() - lastTick1;
		lastTick1 = TVPGetRoughTickCount32();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors4To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		tickNEON += TVPGetRoughTickCount32() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG5ComposeColors4To4", tickC, tickNEON, (float)tickNEON / tickC * 100);

	FILE *fp = fopen("/sdcard/KR2/test.tlg", "rb");
	if (fp) {
		fseek(fp, 0, SEEK_END);
		size_t n = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		tTVPMemoryStream memio; memio.SetSize(n);
		fread(memio.GetInternalBuffer(), 1, n, fp);
		fclose(fp);
		static tTVPBitmap *testbmp = nullptr;
		TVPTLG5ComposeColors3To4 = TVPTLG5ComposeColors3To4_NEON;
		TVPTLG5ComposeColors4To4 = TVPTLG5ComposeColors4To4_NEON;
		TVPTLG5DecompressSlide = TVPTLG5DecompressSlide_c;
		lastTick1 = TVPGetRoughTickCount32();
		for (int i = 0; i < 32; ++i) {
			memio.SetPosition(0);
			TVPLoadTLG(nullptr, nullptr, [](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int {
				if (!testbmp) testbmp = new tTVPBitmap(w, h, 32);
				return testbmp->GetPitch();
			}, [](void *callbackdata, tjs_int y)->void* {
				return testbmp->GetScanLine(y);
			}, [](void *callbackdata, const ttstr & name, const ttstr & value){}, &memio, TVP_clNone, glmNormal);
		}
		tickC = TVPGetRoughTickCount32() - lastTick1;
		delete testbmp; testbmp = nullptr;
		TVPTLG5DecompressSlide = TVPTLG5DecompressSlide_NEON;
		lastTick1 = TVPGetRoughTickCount32();
		for (int i = 0; i < 32; ++i) {
			memio.SetPosition(0);
			TVPLoadTLG(nullptr, nullptr, [](void *callbackdata, tjs_uint w, tjs_uint h, tTVPGraphicPixelFormat fmt)->int {
				if (!testbmp) testbmp = new tTVPBitmap(w, h, 32);
				return testbmp->GetPitch();
			}, [](void *callbackdata, tjs_int y)->void* {
				return testbmp->GetScanLine(y);
			}, [](void *callbackdata, const ttstr & name, const ttstr & value){}, &memio, TVP_clNone, glmNormal);
		}
		tickNEON = TVPGetRoughTickCount32() - lastTick1;
		delete testbmp; testbmp = nullptr;
		AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPLoadTLG5", tickC, tickNEON, (float)tickNEON / tickC * 100);
	}
}

#define REGISTER_TVPGL_BLEND_FUNC_2(origf, f) \
    InitTestData();\
    origf##_c(testdest2, testdata1, 256 * 256);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256);\
	CheckTestData(#f); if (TEST_COUNT) {\
    InitTestData();\
    lastTick1 = TVPGetRoughTickCount32();\
	for (int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, testdata1, 256 * 256); \
    lastTick2 = TVPGetRoughTickCount32();\
	for (int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, 256 * 256); \
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }

#define REGISTER_TVPGL_BLEND_FUNC(origf, f, ...) \
    InitTestData();\
    origf##_c(testdest2, testdata1, 256 * 256, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
    InitTestData();\
    lastTick1 = TVPGetRoughTickCount32();\
	for (int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, testdata1, 256 * 256, __VA_ARGS__); \
    lastTick2 = TVPGetRoughTickCount32();\
    for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }

#define REGISTER_TVPGL_STRECH_FUNC_2(origf, f, ...) \
	InitTestData();\
	origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16); \
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_STRECH_FUNC(origf, f, ...) \
	InitTestData();\
	origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_STRECH_FUNC_0(origf, f) \
	InitTestData();\
	origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_LINTRANS_FUNC_2(origf, f) \
	InitTestData();\
	origf##_c(testdest2, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for (int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_LINTRANS_FUNC(origf, f, ...) \
	InitTestData();\
	origf##_c(testdest2, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_UNIVTRANS_FUNC(origf, f, ...) \
	InitTestData();\
    origf##_c(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC(origf, f, ...) \
	InitTestData();\
	origf##_c(testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, __VA_ARGS__);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC_RGB(origf, f, ...) \
	InitTestData();\
	origf##_c(testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, __VA_ARGS__);\
	CheckTestData_RGB(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetRoughTickCount32();\
	for(int i = 0; i < TEST_COUNT; ++i) origf##_c(testdest2, __VA_ARGS__);\
	lastTick2 = TVPGetRoughTickCount32(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC_TYPE(origf, f, DT, ...) \
	InitTestData();\
	origf##_c((DT)testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON((DT)testdest1, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData(); \
	lastTick1 = TVPGetRoughTickCount32(); \
	for (int i = 0; i < TEST_COUNT; ++i) origf##_c((DT)testdest2, __VA_ARGS__); \
	lastTick2 = TVPGetRoughTickCount32(); \
	for (int i = 0; i < TEST_COUNT; ++i) f##_NEON((DT)testdest1, __VA_ARGS__);\
	tickC = lastTick2 - lastTick1; tickNEON = TVPGetRoughTickCount32() - lastTick2; \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, tickC, tickNEON, (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#else
#define REGISTER_TVPGL_BLEND_FUNC_2(origf, f, ...)  f = f##_NEON;
#define REGISTER_TVPGL_BLEND_FUNC(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_STRECH_FUNC_2(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_STRECH_FUNC(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_LINTRANS_FUNC_2(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_LINTRANS_FUNC(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_UNIVTRANS_FUNC(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_CUSTOM_FUNC(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_CUSTOM_FUNC_RGB(origf, f, ...) f = f##_NEON;
#define REGISTER_TVPGL_CUSTOM_FUNC_TYPE(origf, f, ...) f = f##_NEON;
#endif
#endif
#define REGISTER_TVPGL_ONLY(origf, f) origf = f;

#include "Protect.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "tvpgl_arm_route.h"
#ifdef __cplusplus
};
#endif

FUNC_API void calcBezierPatch_c(float* result, /*const */float* arr/*16*/, /*const */float* a3);
FUNC_API void calcBezierPatch_NEON(float* result, float* arr/*16*/, float* p);

FUNC_API void TVPGL_ASM_Init()
{
    if ((TVPCPUFeatures & TVP_CPU_FAMILY_MASK) == TVP_CPU_FAMILY_ARM && (TVPCPUFeatures & TVP_CPU_HAS_NEON))
	{
		TVPInitTVPGL();
#ifdef LOG_NEON_TEST
#if 0
		do {	// test calcBezierPatch
			float arr[32];
			float resultC[2], resultNEON[2],
				pt[2] = {
				((rand() & 1) ? -1 : 1) * ((float)rand() / rand()),
				((rand() & 1) ? -1 : 1) * ((float)rand() / rand())
			};
			for (int i = 0; i < 32; ++i) {
				arr[i] = ((rand() & 1) ? -1 : 1) * ((float)rand() / rand());
			}
			calcBezierPatch_c(resultC, arr, pt);
			calcBezierPatch_NEON(resultNEON, arr, pt);
			if (resultC[0] != resultNEON[0] || resultC[1] != resultNEON[1]) {
				ShowInMessageBox("test calcBezierPatch fail");
			}
			if (!TEST_COUNT) break;
			for (int i = 0; i < 4; ++i) {
				lastTick1 = TVPGetRoughTickCount32();
				for (int i = 0; i < 160000; ++i) calcBezierPatch_c(resultC, arr, pt);
				lastTick2 = TVPGetRoughTickCount32();
				for (int i = 0; i < 160000; ++i) calcBezierPatch_NEON(resultNEON, arr, pt);
				AddLog("calcBezierPatch: %d ms, NEON: %d ms(%g%%)", (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetRoughTickCount32() - lastTick2), (float)tickNEON / tickC * 100);
			}
			SHOW_AND_CLEAR_LOG;
		} while (0);
#endif
#undef TEST_COUNT
#define TEST_COUNT 1000
// 		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
// 		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
// 		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
// 		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
// 		REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
// 		REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
// 		REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
// 		REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
//		SHOW_AND_CLEAR_LOG;
#undef TEST_COUNT
#define TEST_COUNT 200
		testTLG6_chroma();
		logTLG6_chroma();
#endif
#if 1

		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAlphaBlend, TVPAlphaBlend, testdata1, 256 * 256);
        REGISTER_TVPGL_ONLY(TVPAlphaBlend_HDA, TVPAlphaBlend_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAlphaBlend_o, TVPAlphaBlend_o, testdata1, 256 * 256, 100);
		REGISTER_TVPGL_ONLY(TVPAlphaBlend_HDA_o, TVPAlphaBlend_o_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaBlend_d, TVPAlphaBlend_d, testdata1, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaBlend_a, TVPAlphaBlend_a, testdata1, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaBlend_do, TVPAlphaBlend_do, testdata1, 256 * 256, 100);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaBlend_ao, TVPAlphaBlend_ao, testdata1, 256 * 256, 100);

        REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaColorMat, TVPAlphaColorMat, 0x98765432, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAdditiveAlphaBlend, TVPAdditiveAlphaBlend, testdata1, 256 * 256);
		REGISTER_TVPGL_ONLY(TVPAdditiveAlphaBlend_HDA, TVPAdditiveAlphaBlend_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAdditiveAlphaBlend_o, TVPAdditiveAlphaBlend_o, testdata1, 256 * 256, 100);
		REGISTER_TVPGL_ONLY(TVPAdditiveAlphaBlend_HDA_o, TVPAdditiveAlphaBlend_o_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAdditiveAlphaBlend_a, TVPAdditiveAlphaBlend_a, testdata1, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPAdditiveAlphaBlend_ao, TVPAdditiveAlphaBlend_ao, testdata1, 256 * 256, 100);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPConvertAlphaToAdditiveAlpha, TVPConvertAlphaToAdditiveAlpha, 256 * 256);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaColorMat, TVPAlphaColorMat, 0x98765432, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPStretchAlphaBlend_HDA, TVPStretchAlphaBlend, 16 * 256, testdata1, 0, 1 << 16);
        REGISTER_TVPGL_ONLY(TVPStretchAlphaBlend_HDA, TVPStretchAlphaBlend_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPStretchAlphaBlend_o, TVPStretchAlphaBlend_o, 16 * 256, testdata1, 0, 1 << 16, 100);
        REGISTER_TVPGL_ONLY(TVPStretchAlphaBlend_HDA_o, TVPStretchAlphaBlend_o_NEON);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_a, TVPStretchAlphaBlend_a);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_ao, TVPStretchAlphaBlend_ao, 100);

		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAdditiveAlphaBlend_HDA, TVPStretchAdditiveAlphaBlend);
		REGISTER_TVPGL_ONLY(TVPStretchAdditiveAlphaBlend_HDA, TVPStretchAdditiveAlphaBlend_NEON);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAdditiveAlphaBlend_HDA_o, TVPStretchAdditiveAlphaBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPStretchAdditiveAlphaBlend_HDA_o, TVPStretchAdditiveAlphaBlend_o_NEON);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAdditiveAlphaBlend_a, TVPStretchAdditiveAlphaBlend_a);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAdditiveAlphaBlend_ao, TVPStretchAdditiveAlphaBlend_ao, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_HDA, TVPLinTransAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPLinTransAlphaBlend_HDA, TVPLinTransAlphaBlend_NEON);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_HDA_o, TVPLinTransAlphaBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPLinTransAlphaBlend_HDA_o, TVPLinTransAlphaBlend_o_NEON);
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_d, TVPLinTransAlphaBlend_d); // performance issue !
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_a, TVPLinTransAlphaBlend_a);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_do, TVPLinTransAlphaBlend_do, 100);
		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_ao, TVPLinTransAlphaBlend_ao, 100);

		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAdditiveAlphaBlend_HDA, TVPLinTransAdditiveAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPLinTransAdditiveAlphaBlend_HDA, TVPLinTransAdditiveAlphaBlend_NEON);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAdditiveAlphaBlend_HDA_o, TVPLinTransAdditiveAlphaBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPLinTransAdditiveAlphaBlend_HDA_o, TVPLinTransAdditiveAlphaBlend_o_NEON);
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAdditiveAlphaBlend_a, TVPLinTransAdditiveAlphaBlend_a);
		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAdditiveAlphaBlend_ao, TVPLinTransAdditiveAlphaBlend_ao, 100);

		SHOW_AND_CLEAR_LOG;

// 		REGISTER_TVPGL_CUSTOM_FUNC(TVPInterpStretchCopy, TVPInterpStretchCopy,
// 			127 * 256, testdata1, testdata2, 127, 0, 1 << 16); // performance issue !
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPInterpLinTransCopy, TVPInterpLinTransCopy);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchAdditiveAlphaBlend, TVPInterpStretchAdditiveAlphaBlend,
			16 * 256, testdata1, testdata2, 127, 0, 1 << 16);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchAdditiveAlphaBlend_o, TVPInterpStretchAdditiveAlphaBlend_o,
			16 * 256, testdata1, testdata2, 127, 0, 1 << 16, 100);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpLinTransAdditiveAlphaBlend, TVPInterpLinTransAdditiveAlphaBlend,
			8 * 256, testdata1, 0, 0, 1 << 16, 1 << 16, 64);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpLinTransAdditiveAlphaBlend_o, TVPInterpLinTransAdditiveAlphaBlend_o,
			8 * 256, testdata1, 0, 0, 1 << 16, 1 << 16, 64, 100);

		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchConstAlphaBlend, TVPInterpStretchConstAlphaBlend,
			16 * 256, testdata1, testdata2, 127, 0, 1 << 16, 100);
		REGISTER_TVPGL_LINTRANS_FUNC(TVPInterpLinTransConstAlphaBlend, TVPInterpLinTransConstAlphaBlend, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyOpaqueImage, TVPCopyOpaqueImage);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPStretchCopyOpaqueImage, TVPStretchCopyOpaqueImage, 127 * 256, testdata1, 0, 1 << 16);
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransCopyOpaqueImage, TVPLinTransCopyOpaqueImage); // performance issue !
		REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_HDA, TVPConstAlphaBlend, 100);
		REGISTER_TVPGL_ONLY(TVPConstAlphaBlend_HDA, TVPConstAlphaBlend_NEON);
		REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_d, TVPConstAlphaBlend_d, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_a, TVPConstAlphaBlend_a, 100);

		REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_HDA, TVPStretchConstAlphaBlend, 100);
		REGISTER_TVPGL_ONLY(TVPStretchConstAlphaBlend_HDA, TVPStretchConstAlphaBlend_NEON);
		REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_d, TVPStretchConstAlphaBlend_d, 100);
		REGISTER_TVPGL_ONLY(TVPStretchConstAlphaBlend_d, TVPStretchConstAlphaBlend_d_NEON);
		REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_a, TVPStretchConstAlphaBlend_a, 100);

		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_HDA, TVPLinTransConstAlphaBlend, 100); // performance issue !
		REGISTER_TVPGL_ONLY(TVPLinTransConstAlphaBlend_HDA, TVPLinTransConstAlphaBlend_NEON); // performance issue !
		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_d, TVPLinTransConstAlphaBlend_d, 100); // performance issue !
		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_a, TVPLinTransConstAlphaBlend_a, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPConstAlphaBlend_SD, TVPConstAlphaBlend_SD, testdata1, testdata2, 256 * 256, 100);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPConstAlphaBlend_SD_a, TVPConstAlphaBlend_SD_a, testdata1, testdata2, 256 * 256, 100);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPConstAlphaBlend_SD_d, TVPConstAlphaBlend_SD_d, testdata1, testdata2, 256 * 256, 100);

        // TVPInitUnivTransBlendTable
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPUnivTransBlend, TVPUnivTransBlend, testdata1, testdata2, testrule, testtable, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPUnivTransBlend_d, TVPUnivTransBlend_d, testdata1, testdata2, testrule, testtable, 256 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPUnivTransBlend_a, TVPUnivTransBlend_a, testdata1, testdata2, testrule, testtable, 256 * 256);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch, TVPUnivTransBlend_switch, 240, 32);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch_d, TVPUnivTransBlend_switch_d, 240, 32);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch_a, TVPUnivTransBlend_switch_a, 240, 32);

        REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_HDA, TVPApplyColorMap, testrule, 256 * 256, 0x55d20688);
        REGISTER_TVPGL_ONLY(TVPApplyColorMap_HDA, TVPApplyColorMap_NEON);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_HDA_o, TVPApplyColorMap_o, testrule, 256 * 256, 0x55d20688, 100);
        REGISTER_TVPGL_ONLY(TVPApplyColorMap_HDA_o, TVPApplyColorMap_o_NEON);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_d, TVPApplyColorMap_d, testrule, 256 * 256, 0x55d20688);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_a, TVPApplyColorMap_a, testrule, 256 * 256, 0x55d20688);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_do, TVPApplyColorMap_do, testrule, 256 * 256, 0x55d20688, 100);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPApplyColorMap_ao, TVPApplyColorMap_ao, testrule, 256 * 256, 0x55d20688, 100);

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPConstColorAlphaBlend, TVPConstColorAlphaBlend, 256 * 256, 0x55d20688, 100);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPConstColorAlphaBlend_d, TVPConstColorAlphaBlend_d, 256 * 256, 0x55d20688, 100);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPConstColorAlphaBlend_a, TVPConstColorAlphaBlend_a, 256 * 256, 0x55d20688, 100);

        REGISTER_TVPGL_CUSTOM_FUNC(TVPRemoveConstOpacity, TVPRemoveConstOpacity, 256 * 256, 100);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPRemoveOpacity, TVPRemoveOpacity, testrule, 255 * 256);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPRemoveOpacity_o, TVPRemoveOpacity_o, testrule, 255 * 256, 100);

		REGISTER_TVPGL_BLEND_FUNC_2(TVPAddBlend, TVPAddBlend);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAddBlend_HDA, TVPAddBlend_HDA);
        REGISTER_TVPGL_BLEND_FUNC(TVPAddBlend_HDA_o, TVPAddBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPAddBlend_HDA_o, TVPAddBlend_o_NEON);

		REGISTER_TVPGL_BLEND_FUNC_2(TVPSubBlend, TVPSubBlend);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPSubBlend_HDA, TVPSubBlend_HDA);
        REGISTER_TVPGL_BLEND_FUNC(TVPSubBlend_HDA_o, TVPSubBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPSubBlend_HDA_o, TVPSubBlend_o_NEON);

		REGISTER_TVPGL_BLEND_FUNC_2(TVPMulBlend_HDA, TVPMulBlend_HDA);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPMulBlend, TVPMulBlend);
		REGISTER_TVPGL_BLEND_FUNC(TVPMulBlend_HDA_o, TVPMulBlend_HDA_o, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPMulBlend_o, TVPMulBlend_o, 100);

		SHOW_AND_CLEAR_LOG;

//         REGISTER_TVPGL_BLEND_FUNC_2(TVPColorDodgeBlend_HDA, TVPColorDodgeBlend); // performance issue
//         REGISTER_TVPGL_ONLY(TVPColorDodgeBlend_HDA, TVPColorDodgeBlend_NEON);
//         REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o, TVPColorDodgeBlend_o, 100);
//         REGISTER_TVPGL_ONLY(TVPColorDodgeBlend_HDA_o, TVPColorDodgeBlend_o_NEON);
        REGISTER_TVPGL_BLEND_FUNC_2(TVPDarkenBlend_HDA, TVPDarkenBlend);
        REGISTER_TVPGL_ONLY(TVPDarkenBlend_HDA, TVPDarkenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPDarkenBlend_HDA_o, TVPDarkenBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPDarkenBlend_HDA_o, TVPDarkenBlend_o_NEON);
        REGISTER_TVPGL_BLEND_FUNC_2(TVPLightenBlend_HDA, TVPLightenBlend);
        REGISTER_TVPGL_ONLY(TVPLightenBlend_HDA, TVPLightenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPLightenBlend_HDA_o, TVPLightenBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPLightenBlend_HDA_o, TVPLightenBlend_o_NEON);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPScreenBlend_HDA, TVPScreenBlend);
        REGISTER_TVPGL_ONLY(TVPScreenBlend_HDA, TVPScreenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPScreenBlend_HDA_o, TVPScreenBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPScreenBlend_HDA_o, TVPScreenBlend_o_NEON);

		SHOW_AND_CLEAR_LOG;

//         TVPFastLinearInterpH2F, TVPFastLinearInterpH2F_c;
//         TVPFastLinearInterpH2B, TVPFastLinearInterpH2B_c;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPFastLinearInterpV2, TVPFastLinearInterpV2,
            256 * 256, testdata1, testdata2);

        //TVPStretchColorCopy, TVPStretchColorCopy_c;

		//TVPMakeAlphaFromKey, TVPMakeAlphaFromKey_c;
        
//		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor, TVPCopyColor);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPBindMaskToMain, TVPBindMaskToMain, testrule, 256 * 256);

		// NEON's TVPFillARGB is slower than plain C
//         REGISTER_TVPGL_CUSTOM_FUNC(TVPFillARGB, TVPFillARGB, 256 * 256, 0x55d20688);
//  		REGISTER_TVPGL_ONLY(TVPFillARGB_NC, TVPFillARGB_NEON);

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPFillColor, TVPFillColor, 256 * 256, 0x55d20688);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPFillMask, TVPFillMask, 256 * 256, 0x55d20688);
        REGISTER_TVPGL_CUSTOM_FUNC_TYPE(TVPAddSubVertSum16, TVPAddSubVertSum16, tjs_uint16*, testdata1, testdata2, 128 * 256);
        REGISTER_TVPGL_CUSTOM_FUNC_TYPE(TVPAddSubVertSum16_d, TVPAddSubVertSum16_d, tjs_uint16*, testdata1, testdata2, 128 * 256);

//         TVPAddSubVertSum32, TVPAddSubVertSum32_c;
//         TVPAddSubVertSum32_d, TVPAddSubVertSum32_d_c;
//         TVPDoBoxBlurAvg16, TVPDoBoxBlurAvg16_c;
//         TVPDoBoxBlurAvg16_d, TVPDoBoxBlurAvg16_d_c;
//         TVPDoBoxBlurAvg32, TVPDoBoxBlurAvg32_c;
//         TVPDoBoxBlurAvg32_d, TVPDoBoxBlurAvg32_d_c;
//         TVPSwapLine8, TVPSwapLine8_c;
//         TVPSwapLine32, TVPSwapLine32_c;
//         TVPReverse8, TVPReverse8_c;
//         TVPReverse32, TVPReverse32_c;
        REGISTER_TVPGL_CUSTOM_FUNC(TVPDoGrayScale, TVPDoGrayScale, 256 * 256);
//         TVPInitGammaAdjustTempData, TVPInitGammaAdjustTempData_c;
//         TVPUninitGammaAdjustTempData, TVPUninitGammaAdjustTempData_c;
//         TVPAdjustGamma, TVPAdjustGamma_c;
//         TVPAdjustGamma_a, TVPAdjustGamma_a_c;
//         TVPChBlurMulCopy65, TVPChBlurMulCopy65_c;
//         TVPChBlurAddMulCopy65, TVPChBlurAddMulCopy65_c;
//         TVPChBlurCopy65, TVPChBlurCopy65_c;
//         TVPBLExpand1BitTo8BitPal, TVPBLExpand1BitTo8BitPal_c;
//         TVPBLExpand1BitTo8Bit, TVPBLExpand1BitTo8Bit_c;
//         TVPBLExpand1BitTo32BitPal, TVPBLExpand1BitTo32BitPal_c;
//         TVPBLExpand4BitTo8BitPal, TVPBLExpand4BitTo8BitPal_c;
//         TVPBLExpand4BitTo8Bit, TVPBLExpand4BitTo8Bit_c;
//         TVPBLExpand4BitTo32BitPal, TVPBLExpand4BitTo32BitPal_c;
//         TVPBLExpand8BitTo8BitPal, TVPBLExpand8BitTo8BitPal_c;uni
//         TVPBLExpand8BitTo32BitPal, TVPBLExpand8BitTo32BitPal_c;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPExpand8BitTo32BitGray, TVPExpand8BitTo32BitGray, testrule, 256 * 256);
//         TVPBLConvert15BitTo8Bit, TVPBLConvert15BitTo8Bit;
        REGISTER_TVPGL_CUSTOM_FUNC(TVPBLConvert15BitTo32Bit, TVPBLConvert15BitTo32Bit, (const tjs_uint16*)testrule, 128 * 256);
//         TVPBLConvert24BitTo8Bit, TVPBLConvert24BitTo8Bit;
        REGISTER_TVPGL_ONLY(TVPBLConvert24BitTo32Bit, TVPConvert24BitTo32Bit_NEON);
		REGISTER_TVPGL_CUSTOM_FUNC(TVPConvert24BitTo32Bit, TVPConvert24BitTo32Bit, testrule, 256 * 256 / 3);
		REGISTER_TVPGL_ONLY(TVPConvert32BitTo24Bit, TVPConvert32BitTo24Bit);
//         TVPBLConvert32BitTo8Bit, TVPBLConvert32BitTo8Bit;
//         TVPBLConvert32BitTo32Bit_NoneAlpha, TVPBLConvert32BitTo32Bit_NoneAlpha;
//         TVPBLConvert32BitTo32Bit_MulAddAlpha, TVPBLConvert32BitTo32Bit_MulAddAlpha;
//         TVPBLConvert32BitTo32Bit_AddAlpha, TVPBLConvert32BitTo32Bit_AddAlpha;
//         TVPDither32BitTo16Bit565, TVPDither32BitTo16Bit565;
//         TVPDither32BitTo16Bit555, TVPDither32BitTo16Bit555;
//         TVPDither32BitTo8Bit, TVPDither32BitTo8Bit;
//         TVPTLG5DecompressSlide, TVPTLG5DecompressSlide;
//         TVPTLG6DecodeGolombValuesForFirst, TVPTLG6DecodeGolombValuesForFirst;
//         TVPTLG6DecodeGolombValues, TVPTLG6DecodeGolombValues;

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsAlphaBlend_HDA, TVPPsAlphaBlend);
		REGISTER_TVPGL_ONLY(TVPPsAlphaBlend_HDA, TVPPsAlphaBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsAlphaBlend_HDA_o, TVPPsAlphaBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsAlphaBlend_HDA_o, TVPPsAlphaBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsAddBlend_HDA, TVPPsAddBlend);
		REGISTER_TVPGL_ONLY(TVPPsAddBlend_HDA, TVPPsAddBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsAddBlend_HDA_o, TVPPsAddBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsAddBlend_HDA_o, TVPPsAddBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsSubBlend_HDA, TVPPsSubBlend);
		REGISTER_TVPGL_ONLY(TVPPsSubBlend_HDA, TVPPsSubBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsSubBlend_HDA_o, TVPPsSubBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsSubBlend_HDA_o, TVPPsSubBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsMulBlend_HDA, TVPPsMulBlend);
		REGISTER_TVPGL_ONLY(TVPPsMulBlend_HDA, TVPPsMulBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsMulBlend_HDA_o, TVPPsMulBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsMulBlend_HDA_o, TVPPsMulBlend_o_NEON);

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsScreenBlend_HDA, TVPPsScreenBlend);
		REGISTER_TVPGL_ONLY(TVPPsScreenBlend_HDA, TVPPsScreenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsScreenBlend_HDA_o, TVPPsScreenBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsScreenBlend_HDA_o, TVPPsScreenBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsOverlayBlend_HDA, TVPPsOverlayBlend);
		REGISTER_TVPGL_ONLY(TVPPsOverlayBlend, TVPPsOverlayBlend_NEON);
		REGISTER_TVPGL_ONLY(TVPPsOverlayBlend_HDA, TVPPsOverlayBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsOverlayBlend_HDA_o, TVPPsOverlayBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsOverlayBlend_o, TVPPsOverlayBlend_o_NEON);
		REGISTER_TVPGL_ONLY(TVPPsOverlayBlend_HDA_o, TVPPsOverlayBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsHardLightBlend_HDA, TVPPsHardLightBlend);
		REGISTER_TVPGL_ONLY(TVPPsHardLightBlend, TVPPsHardLightBlend_NEON);
		REGISTER_TVPGL_ONLY(TVPPsHardLightBlend_HDA, TVPPsHardLightBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsHardLightBlend_HDA_o, TVPPsHardLightBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsHardLightBlend_o, TVPPsHardLightBlend_o_NEON);
		REGISTER_TVPGL_ONLY(TVPPsHardLightBlend_HDA_o, TVPPsHardLightBlend_o_NEON);

//         TVPPsSoftLightBlend = TVPPsSoftLightBlend_c;
//         TVPPsSoftLightBlend_o = TVPPsSoftLightBlend_o_c;
//         TVPPsSoftLightBlend_HDA = TVPPsSoftLightBlend_HDA_c;
//         TVPPsSoftLightBlend_HDA_o = TVPPsSoftLightBlend_HDA_o_c;
//         TVPPsColorDodgeBlend = TVPPsColorDodgeBlend_c;
//         TVPPsColorDodgeBlend_o = TVPPsColorDodgeBlend_o_c;
//         TVPPsColorDodgeBlend_HDA = TVPPsColorDodgeBlend_HDA_c;
//         TVPPsColorDodgeBlend_HDA_o = TVPPsColorDodgeBlend_HDA_o_c;
//         TVPPsColorDodge5Blend = TVPPsColorDodge5Blend_c;
//         TVPPsColorDodge5Blend_o = TVPPsColorDodge5Blend_o_c;
//         TVPPsColorDodge5Blend_HDA = TVPPsColorDodge5Blend_HDA_c;
//         TVPPsColorDodge5Blend_HDA_o = TVPPsColorDodge5Blend_HDA_o_c;
//         TVPPsColorBurnBlend = TVPPsColorBurnBlend_c;
//         TVPPsColorBurnBlend_o = TVPPsColorBurnBlend_o_c;
//         TVPPsColorBurnBlend_HDA = TVPPsColorBurnBlend_HDA_c;
//         TVPPsColorBurnBlend_HDA_o = TVPPsColorBurnBlend_HDA_o_c;

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsLightenBlend_HDA, TVPPsLightenBlend);
		REGISTER_TVPGL_ONLY(TVPPsLightenBlend_HDA, TVPPsLightenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsLightenBlend_HDA_o, TVPPsLightenBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsLightenBlend_HDA_o, TVPPsLightenBlend_o_NEON);

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsDarkenBlend_HDA, TVPPsDarkenBlend);
		REGISTER_TVPGL_ONLY(TVPPsDarkenBlend_HDA, TVPPsDarkenBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsDarkenBlend_HDA_o, TVPPsDarkenBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsDarkenBlend_HDA_o, TVPPsDarkenBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsDiffBlend_HDA, TVPPsDiffBlend);
		REGISTER_TVPGL_ONLY(TVPPsDiffBlend_HDA, TVPPsDiffBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsDiffBlend_HDA_o, TVPPsDiffBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsDiffBlend_HDA_o, TVPPsDiffBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsDiff5Blend_HDA, TVPPsDiff5Blend);
		REGISTER_TVPGL_ONLY(TVPPsDiff5Blend_HDA, TVPPsDiff5Blend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsDiff5Blend_HDA_o, TVPPsDiff5Blend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsDiff5Blend_HDA_o, TVPPsDiff5Blend_o_NEON);

		REGISTER_TVPGL_BLEND_FUNC_2(TVPPsExclusionBlend_HDA, TVPPsExclusionBlend);
		REGISTER_TVPGL_ONLY(TVPPsExclusionBlend_HDA, TVPPsExclusionBlend_NEON);
		REGISTER_TVPGL_BLEND_FUNC(TVPPsExclusionBlend_HDA_o, TVPPsExclusionBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsExclusionBlend_HDA_o, TVPPsExclusionBlend_o_NEON);

		REGISTER_TVPGL_ONLY(TVPTLG6DecodeLine, TVPTLG6DecodeLine_NEON);
		REGISTER_TVPGL_ONLY(TVPTLG5ComposeColors3To4, TVPTLG5ComposeColors3To4_NEON);
		REGISTER_TVPGL_ONLY(TVPTLG5ComposeColors4To4, TVPTLG5ComposeColors4To4_NEON);
		REGISTER_TVPGL_ONLY(TVPTLG5DecompressSlide, TVPTLG5DecompressSlide_NEON);

		REGISTER_TVPGL_ONLY(TVPReverseRGB, TVPReverseRGB_NEON);
		REGISTER_TVPGL_ONLY(TVPUpscale65_255, TVPUpscale65_255_NEON);

		SHOW_AND_CLEAR_LOG;
#endif
#ifdef DEBUG_ARM_NEON
		free(testbuff);
#ifdef _DEBUG
        TVPInitTVPGL();
#endif
#endif
	}
}

FUNC_API void TVPGL_ASM_Test() {
#ifdef LOG_NEON_TEST
	TVPCPUFeatures |= TVP_CPU_FAMILY_ARM | TVP_CPU_HAS_NEON;
	TVPGL_ASM_Init();
#endif
}