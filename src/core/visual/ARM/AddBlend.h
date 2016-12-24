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
#ifdef BLEND_WITH_OPACITY
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#else
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-15;
#endif
    while(dest < pVecEndDst) {
#ifdef BLEND_WITH_OPACITY
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char*)src);
#ifdef SUB_FUNC
        s_argb8.val[2] = vmvn_u8(s_argb8.val[2]);
        s_argb8.val[1] = vmvn_u8(s_argb8.val[1]);
        s_argb8.val[0] = vmvn_u8(s_argb8.val[0]);
#endif
        uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], opa8);
        uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], opa8);
        uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], opa8);
        s_argb8.val[2] = vshrn_n_u16(s_r16, 8);
        s_argb8.val[1] = vshrn_n_u16(s_g16, 8);
        s_argb8.val[0] = vshrn_n_u16(s_b16, 8);
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
        d_argb8.val[2] = OP_FUNC(d_argb8.val[2], s_argb8.val[2]);
        d_argb8.val[1] = OP_FUNC(d_argb8.val[1], s_argb8.val[1]);
        d_argb8.val[0] = OP_FUNC(d_argb8.val[0], s_argb8.val[0]);

        vst4_u8((unsigned char *)dest, d_argb8);
        dest += 8;
        src += 8;
#else // normal add-blend
        uint8x16x4_t s_argb8 = vld4q_u8((unsigned char*)src);
#ifdef SUB_FUNC
#ifndef HOLD_DEST_ALPHA
        s_argb8.val[3] = vmvnq_u8(s_argb8.val[3]);
#endif
        s_argb8.val[2] = vmvnq_u8(s_argb8.val[2]);
        s_argb8.val[1] = vmvnq_u8(s_argb8.val[1]);
        s_argb8.val[0] = vmvnq_u8(s_argb8.val[0]);
#endif
        uint8x16x4_t d_argb8 = vld4q_u8((unsigned char*)dest);
#ifndef HOLD_DEST_ALPHA
        d_argb8.val[3] = OP_FUNC(d_argb8.val[3], s_argb8.val[3]);
#endif
        d_argb8.val[2] = OP_FUNC(d_argb8.val[2], s_argb8.val[2]);
        d_argb8.val[1] = OP_FUNC(d_argb8.val[1], s_argb8.val[1]);
        d_argb8.val[0] = OP_FUNC(d_argb8.val[0], s_argb8.val[0]);
		vst4q_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);

        dest += 16;
        src += 16;
#endif
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, src, pEndDst - dest
#ifdef BLEND_WITH_OPACITY
            , opa
#endif
            );
    }
}