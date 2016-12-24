static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest,
#ifdef STRECH_FUNC
    tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep
#elif defined(LINEAR_TRANS_FUNC)
    tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch
#else
    const tjs_uint32 *src, tjs_int len
#endif
#ifdef BLEND_WITH_OPACITY
    , tjs_int opa
#endif
    )
{
    tjs_uint32* pEndDst = dest + len;
    tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
    if(PreFragLen > len) PreFragLen = len;
    if(PreFragLen) {
        C_FUNC_NAME(dest,
#ifdef STRECH_FUNC
            PreFragLen, src, srcstart, srcstep
#elif defined(LINEAR_TRANS_FUNC)
            PreFragLen, src, sx, sy, stepx, stepy, srcpitch
#else
            src, PreFragLen
#endif
#ifdef BLEND_WITH_OPACITY
            , opa
#endif
            );
        dest += PreFragLen;
#ifdef STRECH_FUNC
        srcstart += PreFragLen * srcstep;
#elif defined(LINEAR_TRANS_FUNC)
        sx += stepx * PreFragLen;
        sy += stepy * PreFragLen;
#else
        src += PreFragLen;
#endif
    }
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
    unsigned char strechbuff[32 + 16];
    tjs_uint32 *strechsrc = (tjs_uint32*)((((intptr_t)strechbuff) + 15) & ~15);
#endif
#ifdef BLEND_WITH_OPACITY
    uint8x8_t opa8 = vdup_n_u8(opa);
#endif
    while(dest < pVecEndDst) {
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
        for(int i = 0; i < 8; ++i) {
#ifdef STRECH_FUNC
            strechsrc[i] = src[(srcstart + srcstep * i) >> 16];
#elif defined(LINEAR_TRANS_FUNC)
            strechsrc[i] = *((const tjs_uint32*)((const tjs_uint8*)src + ((sy + stepy * i) >> 16)*srcpitch) + ((sx + stepx * i) >> 16));
#endif
        }
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char *)strechsrc);
#else
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char *)src);
#endif
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
#ifdef BLEND_WITH_OPACITY
        {
            uint16x8_t s_a16 = vmull_u8(s_argb8.val[3], opa8);
            uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], opa8);
            uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], opa8);
            uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], opa8);
            s_argb8.val[3] = vshrn_n_u16(s_a16, 8);
            s_argb8.val[2] = vshrn_n_u16(s_r16, 8);
            s_argb8.val[1] = vshrn_n_u16(s_g16, 8);
            s_argb8.val[0] = vshrn_n_u16(s_b16, 8);
        }
#endif
#ifdef BLEND_WITH_ADDALPHA
        {
            //Da = Sa + Da - SaDa
            uint16x8_t d_a16 = vmull_u8(s_argb8.val[3], d_argb8.val[3]);
            uint16x8_t t = vaddl_u8(s_argb8.val[3], d_argb8.val[3]);
            s_argb8.val[3] = vmvn_u8(s_argb8.val[3]); // 1 - a
            d_a16 = vsubq_u16(t, vshrq_n_u16(d_a16, 8));
            d_argb8.val[3] = vmovn_u16(vsubq_u16(d_a16, vshrq_n_u16(d_a16, 8)));
        }
#else
        s_argb8.val[3] = vmvn_u8(s_argb8.val[3]); // 1 - a
#endif
        // s + d * (1 - sa)
        uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], s_argb8.val[3]);
        uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], s_argb8.val[3]);
        uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], s_argb8.val[3]);

        // 8-bit to do saturated add
        d_argb8.val[2] = vqadd_u8(vshrn_n_u16(d_r16, 8), s_argb8.val[2]);
        d_argb8.val[1] = vqadd_u8(vshrn_n_u16(d_g16, 8), s_argb8.val[1]);
        d_argb8.val[0] = vqadd_u8(vshrn_n_u16(d_b16, 8), s_argb8.val[0]);
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);

#ifdef STRECH_FUNC
        srcstart += srcstep * 8;
#elif defined(LINEAR_TRANS_FUNC)
        sx += stepx * 8;
        sy += stepy * 8;
#else
        src += 8;
#endif
        dest += 8;
    }
    
    if(dest < pEndDst) {
        C_FUNC_NAME(dest,
#ifdef STRECH_FUNC
            pEndDst - dest, src, srcstart, srcstep
#elif defined(LINEAR_TRANS_FUNC)
            pEndDst - dest, src, sx, sy, stepx, stepy, srcpitch
#else
            src, pEndDst - dest
#endif
#ifdef BLEND_WITH_OPACITY
            , opa
#endif
            );
    }
}