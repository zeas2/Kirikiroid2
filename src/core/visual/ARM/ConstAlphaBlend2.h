static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int len
    , tjs_int opa)
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            C_FUNC_NAME(dest, src1, src2,
                PreFragLen
                , opa);
            dest += PreFragLen;
            src1 += PreFragLen;
            src2 += PreFragLen;
        }
    }
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#ifdef BLEND_WITH_DEST_ALPHA
    unsigned char tmpbuff[32 + 8];
    unsigned short *tmpa = (unsigned short*)((((intptr_t)tmpbuff) + 15) & ~15);
    tjs_int o = opa;
    if(o > 127) o ++; /* adjust for error */
    uint8x8_t opa8 = vdup_n_u8(o);
    uint8x8_t iopa8 = vdup_n_u8(256 - o);
#endif
    while(dest < pVecEndDst) {
        uint8x8x4_t s1 = vld4_u8((unsigned char *)src1); 
#ifdef BLEND_WITH_DEST_ALPHA
        uint16x8_t s1_a16 = vmull_u8(s1.val[3], iopa8);
#endif
        uint8x8x4_t s2 = vld4_u8((unsigned char *)src2);
#if defined(BLEND_WITH_DEST_ALPHA)
        uint16x8_t o16 = vmull_u8(s2.val[3], opa8);

        o16 = vsriq_n_u16(o16, s1_a16, 8); // addr
        vst1q_u16(tmpa, o16);
        for(int i = 0; i < 8; ++i) {
            unsigned int addr = tmpa[i];
            tmpa[i] = TVPOpacityOnOpacityTable[addr];
        }
        o16 = vld1q_u16(tmpa);

        //uint16x8_t s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), s_a16);
        uint16x8_t d_r16 = vmulq_u16(vsubl_u8(s2.val[2], s1.val[2]), o16);
        uint16x8_t d_g16 = vmulq_u16(vsubl_u8(s2.val[1], s1.val[1]), o16);
        uint16x8_t d_b16 = vmulq_u16(vsubl_u8(s2.val[0], s1.val[0]), o16);
#else // BLEND_WITH_DEST_ALPHA
        // s1 * o + s2 * (1 - o) => s1 + (s2 - s1) * o
#ifdef BLEND_WITH_ADDALPHA
        uint16x8_t d_a16 = vmulq_n_u16(vsubl_u8(s2.val[3], s1.val[3]), opa);
#endif
        uint16x8_t d_r16 = vmulq_n_u16(vsubl_u8(s2.val[2], s1.val[2]), opa);
        uint16x8_t d_g16 = vmulq_n_u16(vsubl_u8(s2.val[1], s1.val[1]), opa);
        uint16x8_t d_b16 = vmulq_n_u16(vsubl_u8(s2.val[0], s1.val[0]), opa);
#endif //BLEND_WITH_DEST_ALPHA

        // s * o >> 8
#ifdef BLEND_WITH_ADDALPHA
        s2.val[3] = vadd_u8(s1.val[3], vshrn_n_u16(d_a16, 8));
#else
        //s2.val[3] = vdup_n_u8(0);
#endif
        s2.val[2] = vadd_u8(s1.val[2], vshrn_n_u16(d_r16, 8));
        s2.val[1] = vadd_u8(s1.val[1], vshrn_n_u16(d_g16, 8));
        s2.val[0] = vadd_u8(s1.val[0], vshrn_n_u16(d_b16, 8));
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s2);

        src1 += 8;
        src2 += 8;
        dest += 8;
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, src1, src2,
            pEndDst - dest
            , opa);
    }
}