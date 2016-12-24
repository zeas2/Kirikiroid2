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
#if defined(TEST_ARM_NEON_CODE)
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

#ifndef Region_AlphaBlend

#define FUNC_NAME TVPAlphaBlend
#define C_FUNC_NAME TVPAlphaBlend_HDA_c
#include "alphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAlphaBlend_o
#define C_FUNC_NAME TVPAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "alphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

// static void TVPAlphaBlend_d_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
// 	tjs_uint32* pEndDst = dest + len;
// 	{
// 		tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
// 		if (PreFragLen > len) PreFragLen = len;
// 		if (PreFragLen) {
// 			TVPAlphaBlend_d_c(dest, src, PreFragLen);
// 			dest += PreFragLen;
// 			src += PreFragLen;
// 		}
// 	}
// 	tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7) - 7;
// 	unsigned char tmpbuff[32 + 16];
// 	unsigned char *tmpa = __builtin_assume_aligned((unsigned char*)((((intptr_t)tmpbuff) + 15) & ~15), 16);
// 	unsigned short *tmpsa = __builtin_assume_aligned((unsigned short*)(tmpa + 8), 8);
// 	while (dest < pVecEndDst) {
// 		uint8x8x4_t s_argb8 = vld4_u8((unsigned char *)src);
// 		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
// 
// 		//( 255 - (255-a)*(255-b)/ 255 ); 
// 		uint16x8_t isd_a16 = vmull_u8(vmvn_u8(s_argb8.val[3]), vmvn_u8(d_argb8.val[3]));
// 		d_argb8.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
// 
// 		uint16x8_t sopa = vorrq_u16(vshll_n_u8(s_argb8.val[3], 8), vmovl_u8(d_argb8.val[3]));
// 		vst1q_u16(tmpsa, sopa);
// 		tmpa[0] = TVPOpacityOnOpacityTable[tmpsa[0]];
// 		tmpa[1] = TVPOpacityOnOpacityTable[tmpsa[1]];
// 		tmpa[2] = TVPOpacityOnOpacityTable[tmpsa[2]];
// 		tmpa[3] = TVPOpacityOnOpacityTable[tmpsa[3]];
// 		tmpa[4] = TVPOpacityOnOpacityTable[tmpsa[4]];
// 		tmpa[5] = TVPOpacityOnOpacityTable[tmpsa[5]];
// 		tmpa[6] = TVPOpacityOnOpacityTable[tmpsa[6]];
// 		tmpa[7] = TVPOpacityOnOpacityTable[tmpsa[7]];
// 		uint16x8_t s_a16 = vmovl_u8(vld1_u8(tmpa));
// 
// 		// d + (s - d) * sa
// 		uint16x8_t d_r16 = vsubl_u8(s_argb8.val[2], d_argb8.val[2]);
// 		uint16x8_t d_g16 = vsubl_u8(s_argb8.val[1], d_argb8.val[1]);
// 		uint16x8_t d_b16 = vsubl_u8(s_argb8.val[0], d_argb8.val[0]);
// 
// 		d_r16 = vmulq_u16(d_r16, s_a16);
// 		d_g16 = vmulq_u16(d_g16, s_a16);
// 		d_b16 = vmulq_u16(d_b16, s_a16);
// 
// 		d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
// 		d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
// 		d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));
// 
// 		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
// 
// 		src += 8;
// 		dest += 8;
// 	}
// 
// 	if (dest < pEndDst) {
// 		TVPAlphaBlend_d_c(dest, src, pEndDst - dest);
// 	}
// }

#define FUNC_NAME TVPAlphaBlend_d
#define C_FUNC_NAME TVPAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAlphaBlend_a
#define C_FUNC_NAME TVPAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAlphaBlend_do
#define C_FUNC_NAME TVPAlphaBlend_do_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAlphaBlend_ao
#define C_FUNC_NAME TVPAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#endif

static void TVPAlphaColorMat_NEON(tjs_uint32 *dest, const tjs_uint32 color, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPAlphaColorMat_c(dest, color, PreFragLen);
            dest += PreFragLen;
        }
    }

	tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	uint8x8_t d_argb8[3];
    d_argb8[0] = vdup_n_u8(color & 0xff);
    d_argb8[1] = vdup_n_u8((color >> 8) & 0xff);
    d_argb8[2] = vdup_n_u8((color >> 16) & 0xff);
	while(dest < pVecEndDst) {
		uint8x8x4_t s_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
		uint16x8_t s_a16 = vmovl_u8(s_argb8.val[3]);

		// d + (s - d) * sa
        uint16x8_t d_r16 = vsubl_u8(s_argb8.val[2], d_argb8[2]);
        uint16x8_t d_g16 = vsubl_u8(s_argb8.val[1], d_argb8[1]);
        uint16x8_t d_b16 = vsubl_u8(s_argb8.val[0], d_argb8[0]);
		d_r16 = vmulq_u16(d_r16, s_a16);
		d_g16 = vmulq_u16(d_g16, s_a16);
		d_b16 = vmulq_u16(d_b16, s_a16);

		// 8-bit to do saturated add
        s_argb8.val[2] = vadd_u8(d_argb8[2], vshrn_n_u16(d_r16, 8));
        s_argb8.val[1] = vadd_u8(d_argb8[1], vshrn_n_u16(d_g16, 8));
        s_argb8.val[0] = vadd_u8(d_argb8[0], vshrn_n_u16(d_b16, 8));
		s_argb8.val[3] = vdup_n_u8(0xFF);
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s_argb8);
		dest += 8;
	}

    if(dest < pEndDst) {
        TVPAlphaColorMat_c(dest, color, pEndDst - dest);
    }
}

#ifndef Region_AdditiveAlphaBlend

#define FUNC_NAME TVPAdditiveAlphaBlend
#define C_FUNC_NAME TVPAdditiveAlphaBlend_HDA_c
#include "addalphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAdditiveAlphaBlend_o
#define C_FUNC_NAME TVPAdditiveAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "addalphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAdditiveAlphaBlend_a
#define C_FUNC_NAME TVPAdditiveAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAdditiveAlphaBlend_ao
#define C_FUNC_NAME TVPAdditiveAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#endif

static void TVPConvertAlphaToAdditiveAlpha_NEON(tjs_uint32 *dest, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPConvertAlphaToAdditiveAlpha_c(dest, PreFragLen);
            dest += PreFragLen;
        }
    }

	tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	while(dest < pVecEndDst) {
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
		uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], d_argb8.val[3]);
		uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], d_argb8.val[3]);
		uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], d_argb8.val[3]);
		d_argb8.val[2] = vshrn_n_u16(d_r16, 8);
		d_argb8.val[1] = vshrn_n_u16(d_g16, 8);
		d_argb8.val[0] = vshrn_n_u16(d_b16, 8);
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
		dest += 8;
	}
	if(dest < pEndDst) {
        TVPConvertAlphaToAdditiveAlpha_c(dest, pEndDst - dest);
	}
}

#ifndef Region_StretchAlphaBlend
#define STRECH_FUNC
#define FUNC_NAME TVPStretchAlphaBlend
#define C_FUNC_NAME TVPStretchAlphaBlend_HDA_c
#include "alphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAlphaBlend_o
#define C_FUNC_NAME TVPStretchAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "alphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAlphaBlend_d
#define C_FUNC_NAME TVPStretchAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAlphaBlend_a
#define C_FUNC_NAME TVPStretchAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAlphaBlend_do
#define C_FUNC_NAME TVPStretchAlphaBlend_do_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAlphaBlend_ao
#define C_FUNC_NAME TVPStretchAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef STRECH_FUNC
#endif

#ifndef Region_StretchAddAlphaBlend
#define STRECH_FUNC
#define FUNC_NAME TVPStretchAdditiveAlphaBlend
#define C_FUNC_NAME TVPStretchAdditiveAlphaBlend_HDA_c
#include "addalphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPInterpStretchAdditiveAlphaBlend
#define C_FUNC_NAME TVPInterpStretchAdditiveAlphaBlend_c
#define BLEND_WITH_ADDALPHA
#include "InterpTransBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPInterpStretchAdditiveAlphaBlend_o
#define C_FUNC_NAME TVPInterpStretchAdditiveAlphaBlend_o_c
#define BLEND_WITH_OPACITY
#include "InterpTransBlend.h"
#undef BLEND_WITH_OPACITY
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAdditiveAlphaBlend_o
#define C_FUNC_NAME TVPStretchAdditiveAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "addalphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAdditiveAlphaBlend_a
#define C_FUNC_NAME TVPStretchAdditiveAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchAdditiveAlphaBlend_ao
#define C_FUNC_NAME TVPStretchAdditiveAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#undef STRECH_FUNC
#endif

#ifndef Region_LinTransAlphaBlend
#define LINEAR_TRANS_FUNC
#define FUNC_NAME TVPLinTransAlphaBlend
#define C_FUNC_NAME TVPLinTransAlphaBlend_HDA_c
#include "alphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAlphaBlend_o
#define C_FUNC_NAME TVPLinTransAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "alphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAlphaBlend_d
#define C_FUNC_NAME TVPLinTransAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAlphaBlend_a
#define C_FUNC_NAME TVPLinTransAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAlphaBlend_do
#define C_FUNC_NAME TVPLinTransAlphaBlend_do_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_DEST_ALPHA
#include "alphablend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAlphaBlend_ao
#define C_FUNC_NAME TVPLinTransAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "alphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#undef LINEAR_TRANS_FUNC
#endif

#ifndef Region_LinTransAddAlphaBlend
#define LINEAR_TRANS_FUNC

#define FUNC_NAME TVPLinTransAdditiveAlphaBlend
#define C_FUNC_NAME TVPLinTransAdditiveAlphaBlend_HDA_c
#include "addalphablend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAdditiveAlphaBlend_o
#define C_FUNC_NAME TVPLinTransAdditiveAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "addalphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAdditiveAlphaBlend_a
#define C_FUNC_NAME TVPLinTransAdditiveAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransAdditiveAlphaBlend_ao
#define C_FUNC_NAME TVPLinTransAdditiveAlphaBlend_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "addalphablend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPInterpLinTransAdditiveAlphaBlend
#define C_FUNC_NAME TVPInterpLinTransAdditiveAlphaBlend_c
#include "InterpTransBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPInterpLinTransAdditiveAlphaBlend_o
#define C_FUNC_NAME TVPInterpLinTransAdditiveAlphaBlend_o_c
#define BLEND_WITH_OPACITY
#include "InterpTransBlend.h"
#undef BLEND_WITH_OPACITY
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#undef LINEAR_TRANS_FUNC
#endif

static void TVPCopyOpaqueImage_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
    if(PreFragLen > len) PreFragLen = len;
    if(PreFragLen) {
        TVPCopyOpaqueImage_c(dest, src, PreFragLen);
        dest += PreFragLen;
        src += PreFragLen;
    }
	
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			__builtin_prefetch(src, 0, 0);
			uint8x16x4_t s0 = vld4q_u8(__builtin_assume_aligned((uint8_t *)(src), 4));
			s0.val[3] = vdupq_n_u8(0xFF);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)(dest), 8), s0);
			src += 16;
			dest += 16;

// 			__builtin_prefetch(src, 0, 0);
// 			uint8x16x4_t s1 = vld4q_u8(__builtin_assume_aligned((uint8_t *)(src), 4));
// 			s1.val[3] = vdupq_n_u8(0xFF);
// 			vst4q_u8(__builtin_assume_aligned((uint8_t *)(dest + 16), 8), s1);
// 			src += 16;
// 			dest += 16;
		}
	} else {
		while (dest < pVecEndDst) {
			__builtin_prefetch(src, 0, 0);
			uint8x16x4_t s0 = vld4q_u8(__builtin_assume_aligned((uint8_t *)(src), 8));
			s0.val[3] = vdupq_n_u8(0xFF);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)(dest), 8), s0);
			src += 16;
			dest += 16;

// 			__builtin_prefetch(src, 0, 0);
// 			uint8x16x4_t s1 = vld4q_u8(__builtin_assume_aligned((uint8_t *)(src), 8));
// 			s1.val[3] = vdupq_n_u8(0xFF);
// 			vst4q_u8(__builtin_assume_aligned((uint8_t *)(dest + 16), 8), s1);
// 			src += 16;
// 			dest += 16;
		}
	}

    if(dest < pEndDst) {
        TVPCopyOpaqueImage_c(dest, src, pEndDst - dest);
    }
}

#ifndef Region_ConstAlphaBlend

#define FUNC_NAME TVPConstAlphaBlend
#define C_FUNC_NAME TVPConstAlphaBlend_HDA_c
#include "ConstAlphaBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstAlphaBlend_d
#define C_FUNC_NAME TVPConstAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstAlphaBlend_a
#define C_FUNC_NAME TVPConstAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define STRECH_FUNC
#define FUNC_NAME TVPStretchConstAlphaBlend
#define C_FUNC_NAME TVPStretchConstAlphaBlend_HDA_c
#include "ConstAlphaBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchConstAlphaBlend_d
#define C_FUNC_NAME TVPStretchConstAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPStretchConstAlphaBlend_a
#define C_FUNC_NAME TVPStretchConstAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef STRECH_FUNC

#define LINEAR_TRANS_FUNC

#define FUNC_NAME TVPLinTransConstAlphaBlend
#define C_FUNC_NAME TVPLinTransConstAlphaBlend_HDA_c
#include "ConstAlphaBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransConstAlphaBlend_d
#define C_FUNC_NAME TVPLinTransConstAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPLinTransConstAlphaBlend_a
#define C_FUNC_NAME TVPLinTransConstAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "ConstAlphaBlend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#undef LINEAR_TRANS_FUNC

#define FUNC_NAME TVPConstAlphaBlend_SD
#define C_FUNC_NAME TVPConstAlphaBlend_SD_c
#include "ConstAlphaBlend2.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstAlphaBlend_SD_a
#define C_FUNC_NAME TVPConstAlphaBlend_SD_a_c
#define BLEND_WITH_ADDALPHA
#include "ConstAlphaBlend2.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstAlphaBlend_SD_d
#define C_FUNC_NAME TVPConstAlphaBlend_SD_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ConstAlphaBlend2.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#endif

static void TVPStretchCopyOpaqueImage_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPStretchCopyOpaqueImage_c(dest, PreFragLen, src, srcstart, srcstep);
            dest += PreFragLen;
            srcstart += PreFragLen * srcstep;
        }
    }

    unsigned char strechbuff[64 + 16];
	tjs_uint32 *strechsrc = __builtin_assume_aligned((tjs_uint32*)((((intptr_t)strechbuff) + 7) & ~7), 8);
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    while(dest < pVecEndDst) {
        for(int i = 0; i < 16; ++i) {
            strechsrc[i] = src[(srcstart) >> 16];
            srcstart += srcstep;
        }
        uint8x16x4_t s = vld4q_u8((uint8_t *)strechsrc);
        s.val[3] = vdupq_n_u8(0xFF);
		vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        dest += 16;
    }

    if(dest < pEndDst) {
        TVPStretchCopyOpaqueImage_c(dest, pEndDst - dest, src, srcstart, srcstep);
    }
}

void TVPLinTransCopyOpaqueImage_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPLinTransCopyOpaqueImage_c(dest, PreFragLen, src, sx, sy, stepx, stepy, srcpitch);
            dest += PreFragLen;
            sx += stepx * PreFragLen;
            sy += stepy * PreFragLen;
        }
    }

    unsigned char strechbuff[64 + 16];
	tjs_uint32 *strechsrc = __builtin_assume_aligned((tjs_uint32*)((((intptr_t)strechbuff) + 7) & ~7), 8);
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    while(dest < pVecEndDst) {
        for(int i = 0; i < 16; ++i) {
            strechsrc[i] = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
            sx += stepx;
            sy += stepy;
        }
        uint8x16x4_t s = vld4q_u8((uint8_t *)strechsrc);
        s.val[3] = vdupq_n_u8(0xFF);
		vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        dest += 16;
    }

    if(dest < pEndDst) {
        TVPLinTransCopyOpaqueImage_c(dest, pEndDst - dest, src, sx, sy, stepx, stepy, srcpitch);
    }
}

#ifndef Region_UnivTransBlend

#define STRECH_FUNC
#define FUNC_NAME TVPInterpStretchConstAlphaBlend
#define C_FUNC_NAME TVPInterpStretchConstAlphaBlend_c
#define BLEND_WITH_OPACITY
#include "InterpTransBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef STRECH_FUNC

#define LINEAR_TRANS_FUNC
#define FUNC_NAME TVPInterpLinTransConstAlphaBlend
#define C_FUNC_NAME TVPInterpLinTransConstAlphaBlend_c
#include "InterpTransBlend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef LINEAR_TRANS_FUNC

#define UNIV_TRANS

#define FUNC_NAME TVPUnivTransBlend
#define C_FUNC_NAME TVPUnivTransBlend_c
#include "alphablend2.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPUnivTransBlend_d
#define C_FUNC_NAME TVPUnivTransBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "alphablend2.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPUnivTransBlend_a
#define C_FUNC_NAME TVPUnivTransBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "alphablend2.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define UNIV_TRANS_SWITCH
#define FUNC_NAME TVPUnivTransBlend_switch
#define C_FUNC_NAME TVPUnivTransBlend_switch_c
#include "alphablend2.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPUnivTransBlend_switch_d
#define C_FUNC_NAME TVPUnivTransBlend_switch_d_c
#define BLEND_WITH_DEST_ALPHA
#include "alphablend2.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPUnivTransBlend_switch_a
#define C_FUNC_NAME TVPUnivTransBlend_switch_a_c
#define BLEND_WITH_ADDALPHA
#include "alphablend2.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#undef UNIV_TRANS_SWITCH
#undef UNIV_TRANS
#endif

#ifndef Region_ApplyColorMap

#define FUNC_NAME TVPApplyColorMap
#define C_FUNC_NAME TVPApplyColorMap_HDA_c
#include "ApplyColorMap.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPApplyColorMap_o
#define C_FUNC_NAME TVPApplyColorMap_HDA_o_c
#define BLEND_WITH_OPACITY
#include "ApplyColorMap.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPApplyColorMap_d
#define C_FUNC_NAME TVPApplyColorMap_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ApplyColorMap.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPApplyColorMap_a
#define C_FUNC_NAME TVPApplyColorMap_a_c
#define BLEND_WITH_ADDALPHA
#include "ApplyColorMap.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPApplyColorMap_do
#define C_FUNC_NAME TVPApplyColorMap_do_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_DEST_ALPHA
#include "ApplyColorMap.h"
#undef BLEND_WITH_OPACITY
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPApplyColorMap_ao
#define C_FUNC_NAME TVPApplyColorMap_ao_c
#define BLEND_WITH_OPACITY
#define BLEND_WITH_ADDALPHA
#include "ApplyColorMap.h"
#undef BLEND_WITH_OPACITY
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#endif

#ifndef Region_ConstColorAlphaBlend

#define FUNC_NAME TVPConstColorAlphaBlend
#define C_FUNC_NAME TVPConstColorAlphaBlend_c
#include "ConstColorAlphaBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstColorAlphaBlend_d
#define C_FUNC_NAME TVPConstColorAlphaBlend_d_c
#define BLEND_WITH_DEST_ALPHA
#include "ConstColorAlphaBlend.h"
#undef BLEND_WITH_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPConstColorAlphaBlend_a
#define C_FUNC_NAME TVPConstColorAlphaBlend_a_c
#define BLEND_WITH_ADDALPHA
#include "ConstColorAlphaBlend.h"
#undef BLEND_WITH_ADDALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME

#endif

static void TVPRemoveConstOpacity_NEON(tjs_uint32 *dest, tjs_int len, tjs_int strength)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPRemoveConstOpacity_c(dest, PreFragLen, strength);
            dest += PreFragLen;
        }
    }

    uint8x8_t istrength = vdup_n_u8(255 - strength);
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
		uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        d.val[3] = vshrn_n_u16(vmull_u8(d.val[3], istrength), 8);
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
        dest += 8;
    }

    if(dest < pEndDst) {
        TVPRemoveConstOpacity_c(dest, pEndDst - dest, strength);
    }
}

static void TVPRemoveOpacity_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPRemoveOpacity_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	if (!(((intptr_t)src) & 7)) {
		while (dest < pVecEndDst) {
			uint8x8_t s = vld1_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[3] = vshrn_n_u16(vmull_u8(d.val[3], vmvn_u8(s)), 8);
			vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 8;
			src += 8;
		}
	}

    if(dest < pEndDst) {
        TVPRemoveOpacity_c(dest, src, pEndDst - dest);
    }
}

static void TVPRemoveOpacity_o_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len, tjs_int _strength)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPRemoveOpacity_o_c(dest, src, PreFragLen, _strength);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

	uint16x8_t strength = vdupq_n_u16(_strength > 127 ? _strength + 1 : _strength);/* adjust for error */
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	if (!(((intptr_t)src) & 7)) {
		while (dest < pVecEndDst) {
			//d.rgb | (d.a * (65535 - s * str) >> 8)
			uint16x8_t s16 = vmulq_u16(vmovl_u8(vld1_u8(__builtin_assume_aligned(src, 8))), strength); // s * str(8pix)
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned(__builtin_assume_aligned((uint8_t *)dest, 8), 8)); // d (8pix)
			s16 = vmvnq_u16(s16); // 65535 - s
			s16 = vmull_u8(vshrn_n_u16(s16, 8), d.val[3]); // da * (65535 - s * str)
			d.val[3] = vshrn_n_u16(s16, 8);
			vst4_u8(__builtin_assume_aligned(__builtin_assume_aligned((uint8_t *)dest, 8), 8), d);
			dest += 8;
			src += 8;
		}
	}

    if(dest < pEndDst) {
        TVPRemoveOpacity_o_c(dest, src, pEndDst - dest, _strength);
    }
}

#ifndef Region_AddBlend
#define OP_FUNC vqaddq_u8
#define FUNC_NAME TVPAddBlend
#define C_FUNC_NAME TVPAddBlend_c
#include "AddBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPAddBlend_HDA
#define C_FUNC_NAME TVPAddBlend_HDA_c
#define HOLD_DEST_ALPHA
#include "AddBlend.h"
#undef HOLD_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef OP_FUNC

#define OP_FUNC vqadd_u8
#define FUNC_NAME TVPAddBlend_o
#define C_FUNC_NAME TVPAddBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "AddBlend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef OP_FUNC

#endif

#ifndef Region_SubBlend
#define SUB_FUNC
#define OP_FUNC vqsubq_u8
#define FUNC_NAME TVPSubBlend
#define C_FUNC_NAME TVPSubBlend_c
#include "AddBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPSubBlend_HDA
#define C_FUNC_NAME TVPSubBlend_HDA_c
#define HOLD_DEST_ALPHA
#include "AddBlend.h"
#undef HOLD_DEST_ALPHA
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef OP_FUNC

#define OP_FUNC vqsub_u8
#define FUNC_NAME TVPSubBlend_o
#define C_FUNC_NAME TVPSubBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "AddBlend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef OP_FUNC
#undef SUB_FUNC
#endif

#ifndef Region_MulBlend
#define FUNC_NAME TVPMulBlend_HDA
#define C_FUNC_NAME TVPMulBlend_HDA_c
#include "MulBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPMulBlend_HDA_o
#define C_FUNC_NAME TVPMulBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "MulBlend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME

#define NON_HDA
#define FUNC_NAME TVPMulBlend
#define C_FUNC_NAME TVPMulBlend_c
#include "MulBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPMulBlend_o
#define C_FUNC_NAME TVPMulBlend_o_c
#define BLEND_WITH_OPACITY
#include "MulBlend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef NON_HDA
#endif

static void TVPColorDodgeBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPColorDodgeBlend_HDA_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    unsigned char tmpbuff[16 + 16 + 8];
	unsigned short *tmpa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 7) & ~7), 8);
	unsigned char* tmpb = __builtin_assume_aligned((unsigned char*)(tmpa + 8), 8);

	tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7) - 7;
    while(dest < pVecEndDst) {
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char*)src);
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

        // d = d * 255 / (255 - s)
        s_argb8.val[2] = vmvn_u8(s_argb8.val[2]);
        s_argb8.val[1] = vmvn_u8(s_argb8.val[1]);
        s_argb8.val[0] = vmvn_u8(s_argb8.val[0]);

        uint16x8_t tmp = vsubl_u8(s_argb8.val[2], d_argb8.val[2]);
        uint8x8_t mask = vshrn_n_u16(tmp, 8); // 00 or FF
        vst1_u8(tmpb, s_argb8.val[2]);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[0]], tmp, 0);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[1]], tmp, 1);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[2]], tmp, 2);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[3]], tmp, 3);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[4]], tmp, 4);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[5]], tmp, 5);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[6]], tmp, 6);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[7]], tmp, 7);
//         for(int i = 0; i < 8; ++i) {
//             tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
//         }
//        tmp = vld1q_u16(tmpa);
		tmp = vmulq_u16(vmovl_u8(d_argb8.val[2]), tmp);
        d_argb8.val[2] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

        tmp = vsubl_u8(s_argb8.val[1], d_argb8.val[1]);
        mask = vshrn_n_u16(tmp, 8);
		vst1_u8(tmpb, s_argb8.val[1]);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[0]], tmp, 0);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[1]], tmp, 1);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[2]], tmp, 2);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[3]], tmp, 3);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[4]], tmp, 4);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[5]], tmp, 5);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[6]], tmp, 6);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[7]], tmp, 7);
//         for(int i = 0; i < 8; ++i) {
//             tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
//         }
//         tmp = vld1q_u16(tmpa);
        tmp = vmulq_u16(vmovl_u8(d_argb8.val[1]), tmp);
        d_argb8.val[1] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

        tmp = vsubl_u8(s_argb8.val[0], d_argb8.val[0]);
        mask = vshrn_n_u16(tmp, 8);
		vst1_u8(tmpb, s_argb8.val[0]);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[0]], tmp, 0);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[1]], tmp, 1);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[2]], tmp, 2);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[3]], tmp, 3);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[4]], tmp, 4);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[5]], tmp, 5);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[6]], tmp, 6);
		tmp = vsetq_lane_u16(TVPRecipTableForOpacityOnOpacity[tmpb[7]], tmp, 7);
//         for(int i = 0; i < 8; ++i) {
//             tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
//         }
//         tmp = vld1q_u16(tmpa);
        tmp = vmulq_u16(vmovl_u8(d_argb8.val[0]), tmp);
        d_argb8.val[0] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPColorDodgeBlend_HDA_c(dest, src, pEndDst - dest);
    }
}

static void TVPColorDodgeBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPColorDodgeBlend_HDA_o_c(dest, src, PreFragLen, opa);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    unsigned char tmpbuff[16 + 16 + 8];
	unsigned short *tmpa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 7) & ~7), 8);
	unsigned char* tmpb = __builtin_assume_aligned((unsigned char*)(tmpa + 8), 8);

    uint8x8_t opa8 = vdup_n_u8(opa);

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char*)src);
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

        // d = d * 255 / (255 - s * opa / 256)
        uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], opa8);
        uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], opa8);
        uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], opa8);

        s_argb8.val[2] = vmvn_u8(vshrn_n_u16(s_r16, 8));
        s_argb8.val[1] = vmvn_u8(vshrn_n_u16(s_g16, 8));
        s_argb8.val[0] = vmvn_u8(vshrn_n_u16(s_b16, 8));

        uint16x8_t tmp = vsubl_u8(s_argb8.val[2], d_argb8.val[2]);
        uint8x8_t mask = vshrn_n_u16(tmp, 8);
        vst1_u8(tmpb, s_argb8.val[2]);
        for(int i = 0; i < 8; ++i) {
            tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
        }
        tmp = vld1q_u16(tmpa);
        tmp = vmulq_u16(vmovl_u8(d_argb8.val[2]), tmp);
        d_argb8.val[2] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

        tmp = vsubl_u8(s_argb8.val[1], d_argb8.val[1]);
        mask = vshrn_n_u16(tmp, 8);
        vst1_u8(tmpb, s_argb8.val[1]);
        for(int i = 0; i < 8; ++i) {
            tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
        }
        tmp = vld1q_u16(tmpa);
        tmp = vmulq_u16(vmovl_u8(d_argb8.val[1]), tmp);
        d_argb8.val[1] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

        tmp = vsubl_u8(s_argb8.val[0], d_argb8.val[0]);
        mask = vshrn_n_u16(tmp, 8);
        vst1_u8(tmpb, s_argb8.val[0]);
        for(int i = 0; i < 8; ++i) {
            tmpa[i] = TVPRecipTableForOpacityOnOpacity[tmpb[i]];
        }
        tmp = vld1q_u16(tmpa);
        tmp = vmulq_u16(vmovl_u8(d_argb8.val[0]), tmp);
        d_argb8.val[0] = vorr_u8(vshrn_n_u16(tmp, 8), mask);

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPColorDodgeBlend_HDA_o_c(dest, src, pEndDst - dest, opa);
    }
}

static void TVPDarkenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPDarkenBlend_HDA_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)src, 4));
			uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[0] = vminq_u8(s.val[0], d.val[0]);
			d.val[1] = vminq_u8(s.val[1], d.val[1]);
			d.val[2] = vminq_u8(s.val[2], d.val[2]);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 16;
			dest += 16;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[0] = vminq_u8(s.val[0], d.val[0]);
			d.val[1] = vminq_u8(s.val[1], d.val[1]);
			d.val[2] = vminq_u8(s.val[2], d.val[2]);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 16;
			dest += 16;
		}
	}

    if(dest < pEndDst) {
        TVPDarkenBlend_HDA_c(dest, src, pEndDst - dest);
    }
}

static void TVPDarkenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPDarkenBlend_HDA_o_c(dest, src, PreFragLen, opa);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    uint16x8_t opa16 = vdupq_n_u16(opa);
    uint8x8_t revopa8 = vdup_n_u8(~opa);
    while(dest < pVecEndDst) {
		uint8x8x4_t s_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

        s_argb8.val[2] = vmin_u8(s_argb8.val[2], d_argb8.val[2]);
        s_argb8.val[1] = vmin_u8(s_argb8.val[1], d_argb8.val[1]);
        s_argb8.val[0] = vmin_u8(s_argb8.val[0], d_argb8.val[0]);

        // d + (s - d) * o
        uint16x8_t d_r16 = vmulq_u16(vsubl_u8(s_argb8.val[2], d_argb8.val[2]), opa16);
        uint16x8_t d_g16 = vmulq_u16(vsubl_u8(s_argb8.val[1], d_argb8.val[1]), opa16);
        uint16x8_t d_b16 = vmulq_u16(vsubl_u8(s_argb8.val[0], d_argb8.val[0]), opa16);

        d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPDarkenBlend_HDA_o_c(dest, src, pEndDst - dest, opa);
    }
}

static void TVPLightenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPLightenBlend_HDA_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)src, 4));
			uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[0] = vmaxq_u8(s.val[0], d.val[0]);
			d.val[1] = vmaxq_u8(s.val[1], d.val[1]);
			d.val[2] = vmaxq_u8(s.val[2], d.val[2]);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 16;
			dest += 16;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[0] = vmaxq_u8(s.val[0], d.val[0]);
			d.val[1] = vmaxq_u8(s.val[1], d.val[1]);
			d.val[2] = vmaxq_u8(s.val[2], d.val[2]);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 16;
			dest += 16;
		}
	}

    if(dest < pEndDst) {
        TVPLightenBlend_HDA_c(dest, src, pEndDst - dest);
    }
}

static void TVPLightenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPLightenBlend_HDA_o_c(dest, src, PreFragLen, opa);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    uint16x8_t opa16 = vdupq_n_u16(opa);
    while(dest < pVecEndDst) {
		uint8x8x4_t s_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

        s_argb8.val[2] = vmax_u8(s_argb8.val[2], d_argb8.val[2]);
        s_argb8.val[1] = vmax_u8(s_argb8.val[1], d_argb8.val[1]);
        s_argb8.val[0] = vmax_u8(s_argb8.val[0], d_argb8.val[0]);

        // d + (s - d) * o
        uint16x8_t d_r16 = vmulq_u16(vsubl_u8(s_argb8.val[2], d_argb8.val[2]), opa16);
        uint16x8_t d_g16 = vmulq_u16(vsubl_u8(s_argb8.val[1], d_argb8.val[1]), opa16);
        uint16x8_t d_b16 = vmulq_u16(vsubl_u8(s_argb8.val[0], d_argb8.val[0]), opa16);

        d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPLightenBlend_HDA_o_c(dest, src, pEndDst - dest, opa);
    }
}

static void TVPScreenBlend_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPScreenBlend_HDA_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
		uint8x8x4_t s_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

        uint16x8_t d_r16 = vmull_u8(vmvn_u8(s_argb8.val[2]), vmvn_u8(d_argb8.val[2]));
        uint16x8_t d_g16 = vmull_u8(vmvn_u8(s_argb8.val[1]), vmvn_u8(d_argb8.val[1]));
        uint16x8_t d_b16 = vmull_u8(vmvn_u8(s_argb8.val[0]), vmvn_u8(d_argb8.val[0]));
        d_argb8.val[2] = vmvn_u8(vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vmvn_u8(vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vmvn_u8(vshrn_n_u16(d_b16, 8));

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPScreenBlend_HDA_c(dest, src, pEndDst - dest);
    }
}

static void TVPScreenBlend_o_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len, tjs_int opa) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPScreenBlend_HDA_o_c(dest, src, PreFragLen, opa);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    uint8x8_t opa8 = vdup_n_u8(opa);
    while(dest < pVecEndDst) {
		uint8x8x4_t s_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));

        uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], opa8);
        uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], opa8);
        uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], opa8);
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        s_argb8.val[2] = vshrn_n_u16(s_r16, 8);
        s_argb8.val[1] = vshrn_n_u16(s_g16, 8);
        s_argb8.val[0] = vshrn_n_u16(s_b16, 8);

        uint16x8_t d_r16 = vmull_u8(vmvn_u8(s_argb8.val[2]), vmvn_u8(d_argb8.val[2]));
        uint16x8_t d_g16 = vmull_u8(vmvn_u8(s_argb8.val[1]), vmvn_u8(d_argb8.val[1]));
        uint16x8_t d_b16 = vmull_u8(vmvn_u8(s_argb8.val[0]), vmvn_u8(d_argb8.val[0]));
        d_argb8.val[2] = vmvn_u8(vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vmvn_u8(vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vmvn_u8(vshrn_n_u16(d_b16, 8));

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        TVPScreenBlend_HDA_o_c(dest, src, pEndDst - dest, opa);
    }
}

#define STRECH_FUNC
#define COPY_FUNC
#define FUNC_NAME TVPInterpStretchCopy
#define C_FUNC_NAME TVPInterpStretchCopy_c
#include "InterpTransBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef COPY_FUNC
#undef STRECH_FUNC

#define LINEAR_TRANS_FUNC
#define COPY_FUNC
#define FUNC_NAME TVPInterpLinTransCopy
#define C_FUNC_NAME TVPInterpLinTransCopy_c
#include "InterpTransBlend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef COPY_FUNC
#undef LINEAR_TRANS_FUNC

static void TVPFastLinearInterpV2_NEON(tjs_uint32 *dest, tjs_int len, const tjs_uint32 *src0, const tjs_uint32 *src1)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPFastLinearInterpV2_c(dest, PreFragLen, src0, src1);
            dest += PreFragLen;
            src0 += PreFragLen;
            src1 += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-3;
	if ((((intptr_t)src0) & 7) && (((intptr_t)src1) & 7)) {
		while (dest < pVecEndDst) {
			uint8x16_t s0 = vld1q_u8(__builtin_assume_aligned((uint8_t *)src0, 8));
			uint8x16_t s1 = vld1q_u8(__builtin_assume_aligned((uint8_t *)src1, 8));

			vst1q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), vhaddq_u8(s0, s1));
			dest += 4;
			src0 += 4;
			src1 += 4;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16_t s0 = vld1q_u8(__builtin_assume_aligned((uint8_t *)src0, 4));
			uint8x16_t s1 = vld1q_u8(__builtin_assume_aligned((uint8_t *)src1, 4));

			vst1q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), vhaddq_u8(s0, s1));
			dest += 4;
			src0 += 4;
			src1 += 4;
		}
	}

    if(dest < pEndDst) {
        TVPFastLinearInterpV2_c(dest, pEndDst - dest, src0, src1);
    }
}

static void TVPCopyMask_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPCopyMask_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
#if 1
			//__builtin_prefetch(src, 0, 0);
			uint8x8x4_t s = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[3] = s.val[3];
			vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 8;
			dest += 8;
#else
			__asm__ __volatile__(
				"vld4.u8 {d0,d1,d2,d3}, [%1:128]		\n\t" // d
				"vld4.u8 {d3,d4,d5,d6}, [%0]!		\n\t" // s
				"vmov.u8 d3, d6						\n\t"
				"vst4.u8 {d0,d1,d2,d3}, [%1:128]!		\n\t" // d
				:
				: "rw"(src), "rw"(dest)
				);
#endif
		}
	} else {
		while (dest < pVecEndDst) {
#if 1
			//__builtin_prefetch(src, 0, 0);
			uint8x8x4_t s = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			d.val[3] = s.val[3];
			vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			src += 8;
			dest += 8;
#else
			__asm__ __volatile__(
				"vld4.u8 {d0,d1,d2,d3}, [%1:128]		\n\t" // d
				"vld4.u8 {d3,d4,d5,d6}, [%0:128]!		\n\t" // s
				"vmov.u8 d3, d6						\n\t"
				"vst4.u8 {d0,d1,d2,d3}, [%1:128]!		\n\t" // d
				:
				: "rw"(src), "rw"(dest)
				);
#endif
		}
	}

    if(dest < pEndDst) {
        TVPCopyMask_c(dest, src, pEndDst - dest);
    }
}

static void TVPCopyColor_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPCopyColor_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			//__builtin_prefetch(src, 0, 0);
			uint8x8x4_t s = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 4));
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			s.val[3] = d.val[3];
			vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
			src += 8;
			dest += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			//__builtin_prefetch(src, 0, 0);
			uint8x8x4_t s = vld4_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
			s.val[3] = d.val[3];
			vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
			src += 8;
			dest += 8;
		}
	}

    if(dest < pEndDst) {
        TVPCopyColor_c(dest, src, pEndDst - dest);
    }
}

static void TVPBindMaskToMain_NEON(tjs_uint32 *main, const tjs_uint8 *mask, tjs_int len)
{
    tjs_uint32* pEndDst = main + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)main) + 7)&~7) - main;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPBindMaskToMain_c(main, mask, PreFragLen);
            main += PreFragLen;
            mask += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
	if (((intptr_t)mask) & 7) {
		while (main < pVecEndDst) {
			__builtin_prefetch(mask, 0, 0);
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)main, 8));
			s.val[3] = vld1q_u8(mask);
			vst4q_u8(__builtin_assume_aligned((uint8_t *)main, 8), s);
			main += 16;
			mask += 16;
		}
	} else {
		while (main < pVecEndDst) {
			__builtin_prefetch(mask, 0, 0);
			uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)main, 8));
			s.val[3] = vld1q_u8(__builtin_assume_aligned((uint8_t *)mask, 8));
			vst4q_u8(__builtin_assume_aligned((uint8_t *)main, 8), s);
			main += 16;
			mask += 16;
		}
	}

    if(main < pEndDst) {
        TVPBindMaskToMain_c(main, mask, pEndDst - main);
    }
}

static void TVPFillARGB_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 value)
{
	tjs_uint32* pEndDst = dest + len;
	while((((intptr_t)dest)&~7) && dest < pEndDst) {
		*dest++ = value;
	}

	uint32x4_t v = vdupq_n_u32(value);
	tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-3;
	while(dest < pVecEndDst) {
		vst1q_u32(__builtin_assume_aligned(dest, 8), v);
		dest += 4;
	}
	while(dest < pEndDst) {
		*dest++ = value;
	}
}

static void TVPFillColor_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 color)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPFillColor_c(dest, PreFragLen, color);
            dest += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    uint8x16x4_t s;
    s.val[0] = vdupq_n_u8(color & 0xff);
    s.val[1] = vdupq_n_u8((color >> 8) & 0xff);
    s.val[2] = vdupq_n_u8((color >> 16) & 0xff);
    while(dest < pVecEndDst) {
		uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        s.val[3] = d.val[3];
		vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        dest += 16;
    }

    if(dest < pEndDst) {
        TVPFillColor_c(dest, pEndDst - dest, color);
    }
}

static void TVPFillMask_NEON(tjs_uint32 *dest, tjs_int len, tjs_uint32 mask)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPFillMask_c(dest, PreFragLen, mask);
            dest += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    while(dest < pVecEndDst) {
		uint8x16x4_t s = vld4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        s.val[3] = vdupq_n_u8(mask);
		vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        dest += 16;
    }

    if(dest < pEndDst) {
        TVPFillMask_c(dest, pEndDst - dest, mask);
    }
}

static void TVPAddSubVertSum16_NEON(tjs_uint16 *dest, const tjs_uint32 *addline, const tjs_uint32 *subline, tjs_int len)
{
    tjs_uint16* pEndDst = dest + len * 4;
    {
        tjs_int PreFragLen = ((tjs_uint16*)((((intptr_t)dest) + 7)&~7) - dest) / 4;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPAddSubVertSum16_c(dest, addline, subline, PreFragLen);
            dest += PreFragLen * 4;
            addline += PreFragLen;
            subline += PreFragLen;
        }
    }

    tjs_uint16* pVecEndDst = (tjs_uint16*)(((intptr_t)pEndDst)&~7)-7;
	if ((((intptr_t)addline) & 7) && (((intptr_t)subline) & 7)) {
		while (dest < pVecEndDst) {
			uint8x8x4_t add = vld4_u8((unsigned char *)addline);
			uint8x8x4_t sub = vld4_u8((unsigned char *)subline);
			uint16x8x4_t d = vld4q_u16(__builtin_assume_aligned(dest, 8));
			d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));
			d.val[2] = vaddq_u16(d.val[2], vsubl_u8(add.val[2], sub.val[2]));
			d.val[1] = vaddq_u16(d.val[1], vsubl_u8(add.val[1], sub.val[1]));
			d.val[0] = vaddq_u16(d.val[0], vsubl_u8(add.val[0], sub.val[0]));
			vst4q_u16(__builtin_assume_aligned(dest, 8), d);
			dest += 8 * 4;
			addline += 8;
			subline += 8;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x8x4_t add = vld4_u8(__builtin_assume_aligned((uint8_t *)addline, 8));
			uint8x8x4_t sub = vld4_u8(__builtin_assume_aligned((uint8_t *)subline, 8));
			uint16x8x4_t d = vld4q_u16(__builtin_assume_aligned(dest, 8));
			d.val[3] = vaddq_u16(d.val[3], vsubl_u8(add.val[3], sub.val[3]));
			d.val[2] = vaddq_u16(d.val[2], vsubl_u8(add.val[2], sub.val[2]));
			d.val[1] = vaddq_u16(d.val[1], vsubl_u8(add.val[1], sub.val[1]));
			d.val[0] = vaddq_u16(d.val[0], vsubl_u8(add.val[0], sub.val[0]));
			vst4q_u16(__builtin_assume_aligned(dest, 8), d);
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
    {
        tjs_int PreFragLen = ((tjs_uint16*)((((intptr_t)dest) + 7)&~7) - dest) / 4;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPAddSubVertSum16_d_c(dest, addline, subline, PreFragLen);
            dest += PreFragLen * 4;
            addline += PreFragLen;
            subline += PreFragLen;
        }
    }

    tjs_uint16* pVecEndDst = (tjs_uint16*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
        uint8x8x4_t add = vld4_u8((unsigned char *)addline);
        uint8x8x4_t sub = vld4_u8((unsigned char *)subline);
		uint16x8x4_t d = vld4q_u16(__builtin_assume_aligned(dest, 8));

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

		vst4q_u16(__builtin_assume_aligned(dest, 8), d);
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
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
			TVPDoBoxBlurAvg16_c(dest, _sum, add, sub, n, PreFragLen);
            dest += PreFragLen;
            add += PreFragLen;
            sub += PreFragLen;
        }
    }

	static const int32_t c_shl_n[4] = { 0, 8, 16, 24 };

    uint32x4_t rcp = vdupq_n_u32((1<<16) / n);
	int32x4_t shl_n = vld1q_s32(c_shl_n);
    uint16x4_t half_n = vdup_n_u16(n >> 1);

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
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

static void TVPExpand8BitTo32BitGray_NEON(tjs_uint32 *dest, const tjs_uint8 *src, tjs_int len)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPExpand8BitTo32BitGray_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    uint8x16x4_t d;
    d.val[3] = vdupq_n_u8(0xFF);
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			d.val[2] = vld1q_u8(src);
			d.val[1] = d.val[2];
			d.val[0] = d.val[2];
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16;
		}
	} else {
		while (dest < pVecEndDst) {
			d.val[2] = vld1q_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			d.val[1] = d.val[2];
			d.val[0] = d.val[2];
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16;
		}
	}

    if(dest < pEndDst) {
        TVPExpand8BitTo32BitGray_c(dest, src, pEndDst - dest);
    }
}

static void TVPReverseRGB_NEON(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPReverseRGB_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			uint8x16x4_t d = vld4q_u8((uint8_t*)src);
			uint8x16_t t = d.val[0];
			d.val[0] = d.val[2];
			d.val[2] = t;
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16x4_t d = vld4q_u8(__builtin_assume_aligned((uint8_t *)src, 8));
			uint8x16_t t = d.val[0];
			d.val[0] = d.val[2];
			d.val[2] = t;
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16;
		}
	}

    if(dest < pEndDst) {
        TVPReverseRGB_c(dest, src, pEndDst - dest);
    }
}

static void TVPUpscale65_255_NEON(tjs_uint8 *dest, tjs_int len) {
	// dest is already aligned by 16 bytes
	tjs_uint8* pEndDst = dest + len;
	tjs_uint8* pVecEndDst = (tjs_uint8*)(((intptr_t)pEndDst)&~7) - 15;

	while (dest < pVecEndDst) {
		uint8x16_t d = vld1q_u8(__builtin_assume_aligned((uint8_t *)dest, 16));
		d = vqshlq_n_u8(d, 2);
		vst1q_u8(__builtin_assume_aligned((uint8_t *)dest, 16), d);
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
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if (PreFragLen > len) PreFragLen = len;
        if (PreFragLen) {
            TVPBLConvert15BitTo32Bit_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7) - 7;

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
				uint16x8_t s = vshlq_n_u16(vld1q_u16(src), 1);
				d.val[0] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[1] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[2] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
				dest += 8;
				src += 8;
			}
		} else {
			while (dest < pVecEndDst) {
				uint16x8_t s = vshlq_n_u16(vld1q_u16(__builtin_assume_aligned(src, 8)), 1);
				d.val[0] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[1] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				s = vshlq_n_u16(s, 5);
				d.val[2] = vtbl4_u8(lut, vmovn_u16(vshrq_n_u16(s, 11)));
				vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
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
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPConvert24BitTo32Bit_c(dest, src, PreFragLen);
            dest += PreFragLen;
            src += PreFragLen * 3;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    uint8x16x4_t d;
    d.val[3] = vdupq_n_u8(0xFF);
	if (((intptr_t)src) & 7) {
		while (dest < pVecEndDst) {
			uint8x16x3_t s = vld3q_u8(src);
			d.val[2] = s.val[0];
			d.val[1] = s.val[1];
			d.val[0] = s.val[2];
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16 * 3;
		}
	} else {
		while (dest < pVecEndDst) {
			uint8x16x3_t s = vld3q_u8(__builtin_assume_aligned(src, 8));
			d.val[2] = s.val[0];
			d.val[1] = s.val[1];
			d.val[0] = s.val[2];
			vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
			dest += 16;
			src += 16 * 3;
		}
	}

    if(dest < pEndDst) {
        TVPConvert24BitTo32Bit_c(dest, src, pEndDst - dest);
    }
}

static void TVPDoGrayScale_NEON(tjs_uint32 *dest, tjs_int len) {
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            TVPDoGrayScale_c(dest, PreFragLen);
            dest += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
    uint8x8_t const_19 = vdup_n_u8(19), const_183 = vdup_n_u8(183), const_54 = vdup_n_u8(54);
    while(dest < pVecEndDst) {
        uint8x8x4_t s = vld4_u8((uint8_t*)dest);
        uint16x8_t r = vmull_u8(s.val[0], const_19);
        uint16x8_t g = vmull_u8(s.val[1], const_183);
        uint16x8_t b = vmull_u8(s.val[2], const_54);
        r = vaddq_u16(r, g);
        r = vaddq_u16(r, b);
        s.val[2] = s.val[1] = s.val[0] = vshrn_n_u16(r, 8);
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        dest += 8;
    }

    if(dest < pEndDst) {
        TVPDoGrayScale_c(dest, pEndDst - dest);
    }
}

#ifndef Region_PSBlend
#define TVPPS_OPERATION "ps_alphablend.h"
#define FUNC_NAME TVPPsAlphaBlend
#define C_FUNC_NAME TVPPsAlphaBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsAlphaBlend_o
#define C_FUNC_NAME TVPPsAlphaBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_addblend.h"
#define FUNC_NAME TVPPsAddBlend
#define C_FUNC_NAME TVPPsAddBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsAddBlend_o
#define C_FUNC_NAME TVPPsAddBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_subblend.h"
#define FUNC_NAME TVPPsSubBlend
#define C_FUNC_NAME TVPPsSubBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsSubBlend_o
#define C_FUNC_NAME TVPPsSubBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_mulblend.h"
#define FUNC_NAME TVPPsMulBlend
#define C_FUNC_NAME TVPPsMulBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsMulBlend_o
#define C_FUNC_NAME TVPPsMulBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_screenblend.h"
#define FUNC_NAME TVPPsScreenBlend
#define C_FUNC_NAME TVPPsScreenBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsScreenBlend_o
#define C_FUNC_NAME TVPPsScreenBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_overlayblend.h"
#define TVPPS_PREPROC uint8x8_t mask80 = vdup_n_u8(0x80), mask1 = vdup_n_u8(1), maskFE = vdup_n_u8(0xFE);
#define FUNC_NAME TVPPsOverlayBlend
#define C_FUNC_NAME TVPPsOverlayBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsOverlayBlend_o
#define C_FUNC_NAME TVPPsOverlayBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_hardlightblend.h"
#define FUNC_NAME TVPPsHardLightBlend
#define C_FUNC_NAME TVPPsHardLightBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsHardLightBlend_o
#define C_FUNC_NAME TVPPsHardLightBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_PREPROC
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_lightenblend.h"
#define FUNC_NAME TVPPsLightenBlend
#define C_FUNC_NAME TVPPsLightenBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsLightenBlend_o
#define C_FUNC_NAME TVPPsLightenBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_darkenblend.h"
#define FUNC_NAME TVPPsDarkenBlend
#define C_FUNC_NAME TVPPsDarkenBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsDarkenBlend_o
#define C_FUNC_NAME TVPPsDarkenBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_diffblend.h"
#define FUNC_NAME TVPPsDiffBlend
#define C_FUNC_NAME TVPPsDiffBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsDiffBlend_o
#define C_FUNC_NAME TVPPsDiffBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_diff5blend.h"
#define FUNC_NAME TVPPsDiff5Blend
#define C_FUNC_NAME TVPPsDiff5Blend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsDiff5Blend_o
#define C_FUNC_NAME TVPPsDiff5Blend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#define TVPPS_OPERATION "ps_exclusionblend.h"
#define FUNC_NAME TVPPsExclusionBlend
#define C_FUNC_NAME TVPPsExclusionBlend_HDA_c
#include "psblend.h"
#undef C_FUNC_NAME
#undef FUNC_NAME

#define FUNC_NAME TVPPsExclusionBlend_o
#define C_FUNC_NAME TVPPsExclusionBlend_HDA_o_c
#define BLEND_WITH_OPACITY
#include "psblend.h"
#undef BLEND_WITH_OPACITY
#undef C_FUNC_NAME
#undef FUNC_NAME
#undef TVPPS_OPERATION

#endif

#if TVP_TLG6_W_BLOCK_SIZE != 8
#error TVP_TLG6_W_BLOCK_SIZE must be 8 !
#endif

/*
 +---+---+
 |lt | t |        / min(l, t), if lt >= max(l, t);
 +---+---+  ret = | max(l, t), if lt >= min(l, t);
 | l |ret|        \ l + t - lt, otherwise;
 +---+---+
*/
#ifdef DEBUG_ARM_NEON
static inline uint8x8_t med_NEON(uint32x2_t l, uint32x2_t t, uint32x2_t lt)
{
	uint8x8_t max_l_t = vmax_u8(vreinterpret_u8_u32(l), vreinterpret_u8_u32(t));
	uint8x8_t min_l_t = vmin_u8(vreinterpret_u8_u32(l), vreinterpret_u8_u32(t));
	return vsub_u8(vadd_u8(max_l_t, min_l_t), vmax_u8(vmin_u8(max_l_t, vreinterpret_u8_u32(lt)), min_l_t));
}
#else
#define med_NEON(l, t, lt) \
	uint8x8_t max_l_t = vmax_u8(vreinterpret_u8_u32(l), vreinterpret_u8_u32(t));\
	uint8x8_t min_l_t = vmin_u8(vreinterpret_u8_u32(l), vreinterpret_u8_u32(t));\
	uint8x8_t m = vsub_u8(vadd_u8(max_l_t, min_l_t), vmax_u8(vmin_u8(max_l_t, vreinterpret_u8_u32(lt)), min_l_t));
#endif

void TVPTLG6DecodeLineGeneric_NEON(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int start_block, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
{
	/*
		chroma/luminosity decoding
		(this does reordering, color correlation filter, MED/AVG  at a time)
	*/
	uint32x2_t p, up;
	int step, i;

	if(start_block)
	{
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p = vdup_n_u32(curline[-1]);
		up = vdup_n_u32(prevline[-1]);
	}
	else
	{
		p = vdup_n_u32(initialp);
		up = vdup_n_u32(initialp);
	}

	in += skipblockbytes * start_block;
	step = (dir&1)?1:-1;

	for(i = start_block; i < block_limit; i ++)
	{
		int w = width - i*TVP_TLG6_W_BLOCK_SIZE, ww;
		if(w > TVP_TLG6_W_BLOCK_SIZE) w = TVP_TLG6_W_BLOCK_SIZE;
		ww = w;
		if(step==-1) in += ww-1;
		if(i&1) in += oddskip * ww;
		switch(filtertypes[i])
        {
#define IA	(char)(clr>>24)
#define IR	(char)(clr>>16)
#define IG  (char)(clr>>8 )
#define IB  (char)(clr    )
#define TLG6_SET_CLR(R, G, B) (0xff0000 & ((R)<<16)) + (0xff00 & ((G)<<8)) + (0xff & (B)) + ((IA) << 24)

            // 		TVP_TLG6_DO_CHROMA_DECODE( 0, IB, IG, IR); 
#define N 0
#define FILTER TLG6_SET_CLR(IB, IG, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N

            // 		TVP_TLG6_DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG); 
#define N 1
#define FILTER TLG6_SET_CLR(IB+IG, IG, IR+IG)

#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG); 
#define N 2
#define FILTER TLG6_SET_CLR(IB, IG+IB, IR+IB+IG)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR); 
#define N 3
#define FILTER TLG6_SET_CLR(IB+IR+IG, IG+IR, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG); 
#define N 4
#define FILTER TLG6_SET_CLR(IB+IR, IG+IB+IR, IR+IB+IR+IG)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR); 
#define N 5
#define FILTER TLG6_SET_CLR(IB+IR, IG+IB+IR, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 6, IB+IG, IG, IR); 
#define N 6
#define FILTER TLG6_SET_CLR(IB+IG, IG, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 7, IB, IG+IB, IR); 
#define N 7
#define FILTER TLG6_SET_CLR(IB, IG+IB, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 8, IB, IG, IR+IG); 
#define N 8
#define FILTER TLG6_SET_CLR(IB, IG, IR+IG)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB); 
#define N 9
#define FILTER TLG6_SET_CLR(IB+IG+IR+IB, IG+IR+IB, IR+IB)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR); 
#define N 10
#define FILTER TLG6_SET_CLR(IB+IR, IG+IR, IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB); 
#define N 11
#define FILTER TLG6_SET_CLR(IB, IG+IB, IR+IB)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB); 
#define N 12
#define FILTER TLG6_SET_CLR(IB, IG+IR+IB, IR+IB)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG); 
#define N 13
#define FILTER TLG6_SET_CLR(IB+IG, IG+IR+IB+IG, IR+IB+IG)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR); 
#define N 14
#define FILTER TLG6_SET_CLR(IR+IB+IG, IG+IR, IR+IB+IG+IR)
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N
            // 		TVP_TLG6_DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));
#define N 15
#define FILTER TLG6_SET_CLR(IB, IG+(IB<<1), IR+(IB<<1))
#include "TLG6_do_chroma.h"
#undef FILTER
#undef N

		default: return;
		}
		if(step == 1)
			in += skipblockbytes - ww;
		else
			in += skipblockbytes + 1;
		if(i&1) in -= oddskip * ww;
#undef IR
#undef IG
#undef IB
	}
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
        uint8x8x4_t up = vld4_u8(upper);
        rgba.val[0] = vadd_u8(pc.val[0], up.val[0]);
        rgba.val[1] = vadd_u8(pc.val[1], up.val[1]);
        rgba.val[2] = vadd_u8(pc.val[2], up.val[2]);
        vst4_u8(outp, rgba);
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
        uint8x8x4_t up = vld4_u8(upper);
        uint8x8x4_t rgba;
        rgba.val[0] = vadd_u8(pc.val[0], up.val[0]);
        rgba.val[1] = vadd_u8(pc.val[1], up.val[1]);
        rgba.val[2] = vadd_u8(pc.val[2], up.val[2]);
        rgba.val[3] = vadd_u8(pc.val[3], up.val[3]);
        vst4_u8(outp, rgba);
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
FUNC_API int TVPShowSimpleMessageBox(const char * text, const char * caption, unsigned int nButton, const char **btnText);
FUNC_API tjs_uint32 TVPGetTickTime();

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
        testdata1 = testbuff /*+ 1*/;
        testdata2 = testdata1 + 256 * 256;
        testdest1 = testdata2 + 256 * 256;
        testdest2 = testdest1 + 256 * 256;
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

#if defined(TEST_ARM_NEON_CODE)
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
		if( abs(clr1.a - clr2.a) > 1 ||
			abs(clr1.r - clr2.r) > 1 ||
			abs(clr1.g - clr2.g) > 1 ||
			abs(clr1.b - clr2.b) > 1 )
		{
			char tmp[256];
			sprintf(tmp, "test fail on function %s", pszFuncName);
#ifdef _MSC_VER
			cv::Mat test1(256, 256, CV_8UC4, testdest1, 1024);
			cv::Mat test2(256, 256, CV_8UC4, testdest2, 1024);
			ShowInMessageBox(tmp);
#endif
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
	for (int i = 0; i < 256 * 256; ++i) {
		if ((testdest2[i] & 0xFFFFFF) != (testdest1[i] & 0xFFFFFF)) {
			char tmp[256];
			sprintf(tmp, "test fail on function %s", pszFuncName);
			ShowInMessageBox(tmp);
			//assert(!pszFuncName);
			return;
		}
	}
	//SDL_Log("cheking %s pass", pszFuncName);
}

static void testTLG6_chroma()
{
	for (int i = 0; i < 32; ++i) {
		tjs_uint8 block_src_ref[32 * 4];
		tjs_uint8 block_src[32 * 4];
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		tjs_uint32 testdest1[256];
		tjs_uint32 testdest2[256];
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLineGeneric_NEON((tjs_uint32 *)block_src_ref, testdest1, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
			TVPTLG6DecodeLineGeneric_c((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
			if (memcmp(testdest1, testdest2, 8 * 4) != 0) {
				ShowInMessageBox("test fail on function TVPTLG6DecodeLineGeneric");
				assert(0);
			}
		}
		tjs_uint8 *psrc[4] = {
			block_src,
			block_src + 1,
			block_src + 3,
			block_src + 2,
		};

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
	origf(testdest2, testdata1, 256 * 256);\
	f = f##_NEON;\
	f##_NEON(testdest1, testdata1, 256 * 256);\
	CheckTestData(#f);
#define REGISTER_TVPGL_BLEND_FUNC(origf, f, ...) \
    InitTestData();\
    origf(testdest2, testdata1, 256 * 256, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	CheckTestData(#f);
#define REGISTER_TVPGL_STRECH_FUNC_2(origf, f) \
	InitTestData();\
	origf(testdest2, 16 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 16 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f);
#define REGISTER_TVPGL_STRECH_FUNC(origf, f, ...) \
    InitTestData();\
    origf(testdest2, 16 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, 16 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	CheckTestData(#f);
#define REGISTER_TVPGL_LINTRANS_FUNC_2(origf, f) \
	InitTestData();\
	origf(testdest2, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64);\
	f = f##_NEON;\
	f##_NEON(testdest1, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64);\
	CheckTestData(#f);
#define REGISTER_TVPGL_LINTRANS_FUNC(origf, f, ...) \
    InitTestData();\
    origf(testdest2, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, 8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64, __VA_ARGS__);\
    CheckTestData(#f);
#define REGISTER_TVPGL_UNIVTRANS_FUNC(origf, f) \
    InitTestData();\
    origf(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256);\
    CheckTestData_RGB(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC(origf, f, ...) \
    InitTestData();\
    origf(testdest2, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, __VA_ARGS__);\
    CheckTestData(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC_RGB(origf, f, ...) \
    InitTestData();\
    origf(testdest2, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, __VA_ARGS__);\
    CheckTestData_RGB(#f);
#define REGISTER_TVPGL_CUSTOM_FUNC_TYPE(origf, f, DT, ...) \
    InitTestData();\
    origf((DT)testdest2, __VA_ARGS__);\
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
		LogData = (char*)malloc(LogDataSize);
		pLogData = LogData;
	}

	while (*p) {
		if (LogData + LogDataSize - pLogData <= 2) {
			int used = pLogData - LogData;
			LogDataSize += 1024;
			LogData = (char*)realloc(LogData, LogDataSize);
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

static void logTLG6_chroma() {
	if (!TEST_COUNT) return;
	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		tjs_uint8 block_src_ref[32 * 4];
		tjs_uint8 block_src[32 * 4];
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		tjs_uint32 testdest1[256];
		tjs_uint32 testdest2[256];
		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 4; ++n)
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLineGeneric_c((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
		}
		tickC += TVPGetTickTime() - lastTick1;
		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 4; ++n)
		for (tjs_uint8 ft = 0; ft < 32; ++ft) {
			TVPTLG6DecodeLineGeneric_NEON((tjs_uint32 *)block_src_ref, testdest2, 64, 0, 1, &ft, 0, (tjs_uint32 *)block_src, 0, 0, 0);
		}
		tickNEON += TVPGetTickTime() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG6DecodeLineGeneric", tickC, tickNEON, (float)tickNEON / tickC * 100);

	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		tjs_uint8 block_src_ref[32 * 4];
		tjs_uint8 block_src[32 * 4];
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		tjs_uint32 testdest1[256];
		tjs_uint32 testdest2[256];

		tjs_uint8 *psrc[4] = {
			block_src,
			block_src + 1,
			block_src + 3,
			block_src + 2,
		};

		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors3To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		tickC += TVPGetTickTime() - lastTick1;
		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors3To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		tickNEON += TVPGetTickTime() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG5ComposeColors3To4", tickC, tickNEON, (float)tickNEON / tickC * 100);

	tickC = 0; tickNEON = 0;
	for (int i = 0; i < 32; ++i) {
		tjs_uint8 block_src_ref[32 * 4];
		tjs_uint8 block_src[32 * 4];
		for (int j = 0; j < 32 * 4; ++j) {
			block_src_ref[j] = 240 - i - j * 3;
			block_src[j] = 16 + i + j * 3;
		}
		tjs_uint32 testdest1[256];
		tjs_uint32 testdest2[256];

		tjs_uint8 *psrc[4] = {
			block_src,
			block_src + 1,
			block_src + 3,
			block_src + 2,
		};

		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors4To4_c((tjs_uint8*)testdest2, block_src_ref, psrc, 67);
		tickC += TVPGetTickTime() - lastTick1;
		lastTick1 = TVPGetTickTime();
		for (int n = 0; n < TEST_COUNT * 16; ++n) TVPTLG5ComposeColors4To4_NEON((tjs_uint8*)testdest1, block_src_ref, psrc, 67);
		tickNEON += TVPGetTickTime() - lastTick1;
	}
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", "TVPTLG5ComposeColors4To4", tickC, tickNEON, (float)tickNEON / tickC * 100);
}

#define REGISTER_TVPGL_BLEND_FUNC_2(origf, f) \
    InitTestData();\
    origf(testdest2, testdata1, 256 * 256);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256);\
	CheckTestData(#f); if (TEST_COUNT) {\
    InitTestData();\
    lastTick1 = TVPGetTickTime();\
	for (int i = 0; i < TEST_COUNT; ++i) origf(testdest2, testdata1, 256 * 256); \
    lastTick2 = TVPGetTickTime();\
	for (int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, 256 * 256); \
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }

#define REGISTER_TVPGL_BLEND_FUNC(origf, f, ...) \
    InitTestData();\
    origf(testdest2, testdata1, 256 * 256, __VA_ARGS__);\
    f = f##_NEON;\
    f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
    InitTestData();\
    lastTick1 = TVPGetTickTime();\
	for (int i = 0; i < TEST_COUNT; ++i) origf(testdest2, testdata1, 256 * 256, __VA_ARGS__); \
    lastTick2 = TVPGetTickTime();\
    for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, 256 * 256, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }

#define REGISTER_TVPGL_STRECH_FUNC_2(origf, f, ...) \
	InitTestData();\
	origf(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, 127 * 256, testdata1, 0, 1 << 16); \
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_STRECH_FUNC(origf, f, ...) \
	InitTestData();\
	origf(testdest2, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_STRECH_FUNC_0(origf, f) \
	InitTestData();\
	origf(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, 127 * 256, testdata1, 0, 1 << 16);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 1 << 16);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_LINTRANS_FUNC_2(origf, f) \
	InitTestData();\
	origf(testdest2, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for (int i = 0; i < TEST_COUNT; ++i) origf(testdest2, 127 * 256, testdata1, 0, 0, 1 << 16, 0, 256); \
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_LINTRANS_FUNC(origf, f, ...) \
	InitTestData();\
	origf(testdest2, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, 127 * 256, testdata1, 0, 0, 1<<16, 0, 256, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_UNIVTRANS_FUNC(origf, f, ...) \
	InitTestData();\
	origf(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256);\
	f = f##_NEON;\
	f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, testdata1, testdata2, testrule, testtable, 256 * 256);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, testdata1, testdata2, testrule, testtable, 256 * 256);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC(origf, f, ...) \
	InitTestData();\
	origf(testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, __VA_ARGS__);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC_RGB(origf, f, ...) \
	InitTestData();\
	origf(testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON(testdest1, __VA_ARGS__);\
	CheckTestData_RGB(#f); if (TEST_COUNT) {\
	InitTestData();\
	lastTick1 = TVPGetTickTime();\
	for(int i = 0; i < TEST_COUNT; ++i) origf(testdest2, __VA_ARGS__);\
	lastTick2 = TVPGetTickTime(); \
	for(int i = 0; i < TEST_COUNT; ++i) f##_NEON(testdest1, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
	f = f##_NEON; }
#define REGISTER_TVPGL_CUSTOM_FUNC_TYPE(origf, f, DT, ...) \
	InitTestData();\
	origf((DT)testdest2, __VA_ARGS__);\
	f = f##_NEON;\
	f##_NEON((DT)testdest1, __VA_ARGS__);\
	CheckTestData(#f); if (TEST_COUNT) {\
	InitTestData(); \
	lastTick1 = TVPGetTickTime(); \
	for (int i = 0; i < TEST_COUNT; ++i) origf((DT)testdest2, __VA_ARGS__); \
	lastTick2 = TVPGetTickTime(); \
	for (int i = 0; i < TEST_COUNT; ++i) f##_NEON((DT)testdest1, __VA_ARGS__);\
	AddLog("%s: %d ms, NEON: %d ms(%g%%)", #f, (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100); \
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
				lastTick1 = TVPGetTickTime();
				for (int i = 0; i < 160000; ++i) calcBezierPatch_c(resultC, arr, pt);
				lastTick2 = TVPGetTickTime();
				for (int i = 0; i < 160000; ++i) calcBezierPatch_NEON(resultNEON, arr, pt);
				AddLog("calcBezierPatch: %d ms, NEON: %d ms(%g%%)", (tickC = lastTick2 - lastTick1), (tickNEON = TVPGetTickTime() - lastTick2), (float)tickNEON / tickC * 100);
			}
			SHOW_AND_CLEAR_LOG;
		} while (0);
#endif
#undef TEST_COUNT
#define TEST_COUNT 1000
		usleep(1000000 * 3);

		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask_c, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask_c, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask_c, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask_c, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor_c, TVPCopyColor);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor_c, TVPCopyColor);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor_c, TVPCopyColor);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor_c, TVPCopyColor);
		SHOW_AND_CLEAR_LOG;

#undef TEST_COUNT
#define TEST_COUNT 200

		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_d_c, TVPUnivTransBlend_d);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_d_c, TVPUnivTransBlend_d);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_d_c, TVPUnivTransBlend_d);
		REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_d_c, TVPUnivTransBlend_d);
		REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o_c, TVPColorDodgeBlend_o, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o_c, TVPColorDodgeBlend_o, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o_c, TVPColorDodgeBlend_o, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o_c, TVPColorDodgeBlend_o, 100);
		SHOW_AND_CLEAR_LOG;
#endif
		// use NEON-optimized routines
        //_Initialize_Route_Ptr();
#if 1
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAlphaBlend_HDA, TVPAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPAlphaBlend_HDA, TVPAlphaBlend_NEON);
		REGISTER_TVPGL_BLEND_FUNC(TVPAlphaBlend_HDA_o, TVPAlphaBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPAlphaBlend_HDA_o, TVPAlphaBlend_o_NEON);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAlphaBlend_d, TVPAlphaBlend_d);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAlphaBlend_a, TVPAlphaBlend_a);
        REGISTER_TVPGL_BLEND_FUNC(TVPAlphaBlend_do, TVPAlphaBlend_do, 100);
        REGISTER_TVPGL_BLEND_FUNC(TVPAlphaBlend_ao, TVPAlphaBlend_ao, 100);

        REGISTER_TVPGL_CUSTOM_FUNC(TVPAlphaColorMat, TVPAlphaColorMat, 0x98765432, 256 * 256);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAdditiveAlphaBlend_HDA, TVPAdditiveAlphaBlend);
		REGISTER_TVPGL_ONLY(TVPAdditiveAlphaBlend_HDA, TVPAdditiveAlphaBlend_NEON);
		REGISTER_TVPGL_BLEND_FUNC(TVPAdditiveAlphaBlend_HDA_o, TVPAdditiveAlphaBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPAdditiveAlphaBlend_HDA_o, TVPAdditiveAlphaBlend_o_NEON);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPAdditiveAlphaBlend_a, TVPAdditiveAlphaBlend_a);
		REGISTER_TVPGL_BLEND_FUNC(TVPAdditiveAlphaBlend_ao, TVPAdditiveAlphaBlend_ao, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_CUSTOM_FUNC(TVPConvertAlphaToAdditiveAlpha, TVPConvertAlphaToAdditiveAlpha, 256 * 256);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_HDA, TVPStretchAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPStretchAlphaBlend_HDA, TVPStretchAlphaBlend_NEON);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_HDA_o, TVPStretchAlphaBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPStretchAlphaBlend_HDA_o, TVPStretchAlphaBlend_o_NEON);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_d, TVPStretchAlphaBlend_d);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAlphaBlend_a, TVPStretchAlphaBlend_a);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_do, TVPStretchAlphaBlend_do, 100);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAlphaBlend_ao, TVPStretchAlphaBlend_ao, 100);

		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAdditiveAlphaBlend_HDA, TVPStretchAdditiveAlphaBlend);
		REGISTER_TVPGL_ONLY(TVPStretchAdditiveAlphaBlend_HDA, TVPStretchAlphaBlend_NEON);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAdditiveAlphaBlend_HDA_o, TVPStretchAdditiveAlphaBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPStretchAdditiveAlphaBlend_HDA_o, TVPStretchAlphaBlend_o_NEON);
		REGISTER_TVPGL_STRECH_FUNC_2(TVPStretchAdditiveAlphaBlend_a, TVPStretchAdditiveAlphaBlend_a);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchAdditiveAlphaBlend_ao, TVPStretchAdditiveAlphaBlend_ao, 100);
        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchAdditiveAlphaBlend, TVPInterpStretchAdditiveAlphaBlend,
            16 * 256, testdata1, testdata2, 127, 0, 1<<16);
		REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchAdditiveAlphaBlend_o, TVPInterpStretchAdditiveAlphaBlend_o,
            16 * 256, testdata1, testdata2, 127, 0, 1<<16, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_HDA, TVPLinTransAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPLinTransAlphaBlend_HDA, TVPLinTransAlphaBlend_NEON);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_HDA_o, TVPLinTransAlphaBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPLinTransAlphaBlend_HDA_o, TVPLinTransAlphaBlend_o_NEON);
		//REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_d, TVPLinTransAlphaBlend_d); // performance issue !
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAlphaBlend_a, TVPLinTransAlphaBlend_a);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_do, TVPLinTransAlphaBlend_do, 100);
		REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAlphaBlend_ao, TVPLinTransAlphaBlend_ao, 100);

		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAdditiveAlphaBlend_HDA, TVPLinTransAdditiveAlphaBlend);
        REGISTER_TVPGL_ONLY(TVPLinTransAdditiveAlphaBlend_HDA, TVPLinTransAdditiveAlphaBlend_NEON);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAdditiveAlphaBlend_HDA_o, TVPLinTransAdditiveAlphaBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPLinTransAdditiveAlphaBlend_HDA_o, TVPLinTransAdditiveAlphaBlend_o_NEON);
		REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransAdditiveAlphaBlend_a, TVPLinTransAdditiveAlphaBlend_a);
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransAdditiveAlphaBlend_ao, TVPLinTransAdditiveAlphaBlend_ao, 100);
        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpLinTransAdditiveAlphaBlend, TVPInterpLinTransAdditiveAlphaBlend,
            8 * 256, testdata1, 0, 0, 1<<16, 1<<16, 64);
        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpLinTransAdditiveAlphaBlend_o, TVPInterpLinTransAdditiveAlphaBlend_o,
			8 * 256, testdata1, 0, 0, 1 << 16, 1 << 16, 64, 100);

		SHOW_AND_CLEAR_LOG;

		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyOpaqueImage, TVPCopyOpaqueImage);
        REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_HDA, TVPConstAlphaBlend, 100);
        REGISTER_TVPGL_ONLY(TVPConstAlphaBlend_HDA, TVPConstAlphaBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_d, TVPConstAlphaBlend_d, 100);
		REGISTER_TVPGL_BLEND_FUNC(TVPConstAlphaBlend_a, TVPConstAlphaBlend_a, 100);

		//REGISTER_TVPGL_STRECH_FUNC_0(TVPStretchCopyOpaqueImage, TVPStretchCopyOpaqueImage); // performance issue
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_HDA, TVPStretchConstAlphaBlend, 100);
        REGISTER_TVPGL_ONLY(TVPStretchConstAlphaBlend_HDA, TVPStretchConstAlphaBlend_NEON);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_d, TVPStretchConstAlphaBlend_d, 100);
        REGISTER_TVPGL_STRECH_FUNC(TVPStretchConstAlphaBlend_a, TVPStretchConstAlphaBlend_a, 100);
		//REGISTER_TVPGL_LINTRANS_FUNC_2(TVPLinTransCopyOpaqueImage, TVPLinTransCopyOpaqueImage); // performance issue !
        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPInterpStretchConstAlphaBlend, TVPInterpStretchConstAlphaBlend,
            16 * 256, testdata1, testdata2, 127, 0, 1<<16, 100);
        //REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_HDA, TVPLinTransConstAlphaBlend, 100); // performance issue !
        //REGISTER_TVPGL_ONLY(TVPLinTransConstAlphaBlend_HDA, TVPLinTransConstAlphaBlend_NEON); // performance issue !
        //REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_d, TVPLinTransConstAlphaBlend_d, 100); // performance issue !
        REGISTER_TVPGL_LINTRANS_FUNC(TVPLinTransConstAlphaBlend_a, TVPLinTransConstAlphaBlend_a, 100);
		//REGISTER_TVPGL_LINTRANS_FUNC(TVPInterpLinTransConstAlphaBlend, TVPInterpLinTransConstAlphaBlend, 100); // performance issue !

		SHOW_AND_CLEAR_LOG;

        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPConstAlphaBlend_SD, TVPConstAlphaBlend_SD, testdata1, testdata2, 256 * 256, 100);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPConstAlphaBlend_SD_a, TVPConstAlphaBlend_SD_a, testdata1, testdata2, 256 * 256, 100);
        REGISTER_TVPGL_CUSTOM_FUNC_RGB(TVPConstAlphaBlend_SD_d, TVPConstAlphaBlend_SD_d, testdata1, testdata2, 256 * 256, 100);

        // TVPInitUnivTransBlendTable
        REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend, TVPUnivTransBlend);
        REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_d, TVPUnivTransBlend_d);
        REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_a, TVPUnivTransBlend_a);
//         REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch, TVPUnivTransBlend_switch, 240, 32);
//         REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch_d, TVPUnivTransBlend_switch_d, 240, 32);
//         REGISTER_TVPGL_UNIVTRANS_FUNC(TVPUnivTransBlend_switch_a, TVPUnivTransBlend_switch_a, 240, 32);

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
        REGISTER_TVPGL_BLEND_FUNC(TVPColorDodgeBlend_HDA_o, TVPColorDodgeBlend_o, 100);
        REGISTER_TVPGL_ONLY(TVPColorDodgeBlend_HDA_o, TVPColorDodgeBlend_o_NEON);
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

        REGISTER_TVPGL_CUSTOM_FUNC(TVPInterpStretchCopy, TVPInterpStretchCopy,
            127 * 256, testdata1, testdata2, 127, 0, 1<<16);

//         TVPFastLinearInterpH2F, TVPFastLinearInterpH2F_c;
//         TVPFastLinearInterpH2B, TVPFastLinearInterpH2B_c;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPFastLinearInterpV2, TVPFastLinearInterpV2,
            256 * 256, testdata1, testdata2);

        //TVPStretchColorCopy, TVPStretchColorCopy_c;

		//REGISTER_TVPGL_LINTRANS_FUNC_2(TVPInterpLinTransCopy, TVPInterpLinTransCopy); // performance issue !

        //TVPMakeAlphaFromKey, TVPMakeAlphaFromKey_c;
        
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyMask, TVPCopyMask);
		REGISTER_TVPGL_BLEND_FUNC_2(TVPCopyColor, TVPCopyColor);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPBindMaskToMain, TVPBindMaskToMain, testrule, 256 * 256);

		// NEON's TVPFillARGB is slower than plain C
//         REGISTER_TVPGL_CUSTOM_FUNC(TVPFillARGB_c, TVPFillARGB, 256 * 256, 0x55d20688);
// 		REGISTER_TVPGL_ONLY(TVPFillARGB_NC, TVPFillARGB_NEON);

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
//         TVPBLExpand8BitTo8BitPal, TVPBLExpand8BitTo8BitPal_c;
//         TVPBLExpand8BitTo32BitPal, TVPBLExpand8BitTo32BitPal_c;

        REGISTER_TVPGL_CUSTOM_FUNC(TVPExpand8BitTo32BitGray, TVPExpand8BitTo32BitGray, testrule, 256 * 256);
//         TVPBLConvert15BitTo8Bit, TVPBLConvert15BitTo8Bit;
        REGISTER_TVPGL_CUSTOM_FUNC(TVPBLConvert15BitTo32Bit, TVPBLConvert15BitTo32Bit, (const tjs_uint16*)testrule, 128 * 256);
//         TVPBLConvert24BitTo8Bit, TVPBLConvert24BitTo8Bit;
        REGISTER_TVPGL_ONLY(TVPBLConvert24BitTo32Bit, TVPConvert24BitTo32Bit_NEON);
        REGISTER_TVPGL_CUSTOM_FUNC(TVPConvert24BitTo32Bit, TVPConvert24BitTo32Bit, testrule, 256 * 256 / 3);
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
        REGISTER_TVPGL_ONLY(TVPPsOverlayBlend_HDA, TVPPsOverlayBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsOverlayBlend_HDA_o, TVPPsOverlayBlend_o, 100);
		REGISTER_TVPGL_ONLY(TVPPsOverlayBlend_HDA_o, TVPPsOverlayBlend_o_NEON);

        REGISTER_TVPGL_BLEND_FUNC_2(TVPPsHardLightBlend_HDA, TVPPsHardLightBlend);
        REGISTER_TVPGL_ONLY(TVPPsHardLightBlend_HDA, TVPPsHardLightBlend_NEON);
        REGISTER_TVPGL_BLEND_FUNC(TVPPsHardLightBlend_HDA_o, TVPPsHardLightBlend_o, 100);
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

		REGISTER_TVPGL_ONLY(TVPTLG6DecodeLineGeneric, TVPTLG6DecodeLineGeneric_NEON);
		REGISTER_TVPGL_ONLY(TVPTLG5ComposeColors3To4, TVPTLG5ComposeColors3To4_NEON);
		REGISTER_TVPGL_ONLY(TVPTLG5ComposeColors4To4, TVPTLG5ComposeColors4To4_NEON);
//		REGISTER_TVPGL_ONLY(TVPTLG5DecompressSlide, TVPTLG5DecompressSlide_NEON); // TODO test performance

		REGISTER_TVPGL_ONLY(TVPReverseRGB, TVPReverseRGB_NEON);
		REGISTER_TVPGL_ONLY(TVPUpscale65_255, TVPUpscale65_255_NEON);

		SHOW_AND_CLEAR_LOG;
#endif
#ifdef DEBUG_ARM_NEON
#ifdef TEST_ARM_NEON_CODE
		testTLG6_chroma();
#endif
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