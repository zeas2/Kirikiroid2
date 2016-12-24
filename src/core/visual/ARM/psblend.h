static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest,
    const tjs_uint32 *src, tjs_int len
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
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#ifdef BLEND_WITH_OPACITY
    uint8x8_t opa8 = vdup_n_u8(opa);
#endif
#ifdef TVPPS_PREPROC
    TVPPS_PREPROC
#endif
    while(dest < pVecEndDst) {
        uint8x8x4_t s = vld4_u8((unsigned char *)src);
#ifdef BLEND_WITH_OPACITY
        uint16x8_t a = vmull_u8(s.val[3], opa8);
#else
        uint16x8_t a = vmovl_u8(s.val[3]);
#endif
		uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
#if defined(BLEND_WITH_OPACITY)
        a = vshrq_n_u16(a, 8);
#endif
#include TVPPS_OPERATION
        s.val[3] = d.val[3]; // hold alpha
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s);
        src += 8;
        dest += 8;
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, src, pEndDst - dest
#ifdef BLEND_WITH_OPACITY
            , opa
#endif
            );
    }
}