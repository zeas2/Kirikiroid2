static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest, const tjs_uint32 *src, tjs_int len
#ifdef BLEND_WITH_OPACITY
    , tjs_int opa
#endif
    )
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            C_FUNC_NAME(dest, src, PreFragLen
#ifdef BLEND_WITH_OPACITY
                , opa
#endif
                );
            dest += PreFragLen;
            src += PreFragLen;
        }
    }

#ifdef BLEND_WITH_OPACITY
    uint8x8_t opa8 = vdup_n_u8(opa);
#endif
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char*)src);
#ifdef BLEND_WITH_OPACITY
        s_argb8.val[2] = vmvn_u8(s_argb8.val[2]);
        s_argb8.val[1] = vmvn_u8(s_argb8.val[1]);
        s_argb8.val[0] = vmvn_u8(s_argb8.val[0]);
        uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], opa8);
        uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], opa8);
        uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], opa8);
        s_argb8.val[2] = vmvn_u8(vshrn_n_u16(s_r16, 8));
        s_argb8.val[1] = vmvn_u8(vshrn_n_u16(s_g16, 8));
        s_argb8.val[0] = vmvn_u8(vshrn_n_u16(s_b16, 8));
#endif
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        uint16x8_t d_r16 = vmull_u8(s_argb8.val[2], d_argb8.val[2]);
        uint16x8_t d_g16 = vmull_u8(s_argb8.val[1], d_argb8.val[1]);
        uint16x8_t d_b16 = vmull_u8(s_argb8.val[0], d_argb8.val[0]);
        d_argb8.val[2] = vshrn_n_u16(d_r16, 8);
        d_argb8.val[1] = vshrn_n_u16(d_g16, 8);
        d_argb8.val[0] = vshrn_n_u16(d_b16, 8);
#ifdef NON_HDA
		d_argb8.val[3] = vdup_n_u8(0);
#endif

		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
        src += 8;
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, src, pEndDst - dest
#ifdef BLEND_WITH_OPACITY
            , opa
#endif
            );
    }
}