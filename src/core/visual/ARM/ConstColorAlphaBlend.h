static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest, tjs_int len, tjs_uint32 color, tjs_int opa)
{
    /* this function always holds desitination alpha channel */
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            C_FUNC_NAME(dest, PreFragLen, color, opa);
            dest += PreFragLen;
        }
    }

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#ifdef BLEND_WITH_DEST_ALPHA
    unsigned char tmpbuff[32 + 8];
	unsigned short *tmpa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 15) & ~15), 16);
	unsigned char  *tmpd = __builtin_assume_aligned((unsigned char *)(tmpa + 8), 16);
	uint16x8_t hopa16 = vdupq_n_u16(opa << 8);
    uint8x8_t s_r8 = vdup_n_u8((color >> 16) & 0xFF);
    uint8x8_t s_g8 = vdup_n_u8((color >>  8) & 0xFF);
    uint8x8_t s_b8 = vdup_n_u8((color >>  0) & 0xFF);
#elif defined(BLEND_WITH_ADDALPHA)
    uint8x8_t s_r8 = vdup_n_u8((((color >> 16) & 0xFF) * opa) >> 8);
    uint8x8_t s_g8 = vdup_n_u8((((color >>  8) & 0xFF) * opa) >> 8);
    uint8x8_t s_b8 = vdup_n_u8((((color >>  0) & 0xFF) * opa) >> 8);
    uint8x8_t s_a8 = vdup_n_u8(opa);
#else
    uint16x8_t s_r16 = vdupq_n_u16(((color >> 16) & 0xFF) * opa);
    uint16x8_t s_g16 = vdupq_n_u16(((color >>  8) & 0xFF) * opa);
    uint16x8_t s_b16 = vdupq_n_u16(((color >>  0) & 0xFF) * opa);
#endif
    uint8x8_t s_ia8 = vdup_n_u8(opa ^ 0xFF);
    while(dest < pVecEndDst) {
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));

#ifdef BLEND_WITH_ADDALPHA
        uint16x8_t tmp = vmull_u8(d_argb8.val[3], s_a8);
        uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], s_ia8);
        uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], s_ia8);
        uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], s_ia8);
        tmp = vsubq_u16(vaddl_u8(d_argb8.val[3], s_a8), vshrq_n_u16(tmp, 8));
        d_argb8.val[3] = vsub_u8(vmovn_u16(tmp), vshrn_n_u16(tmp, 8));
        d_argb8.val[2] = vqadd_u8(vshrn_n_u16(d_r16, 8), s_r8);
        d_argb8.val[1] = vqadd_u8(vshrn_n_u16(d_g16, 8), s_g8);
        d_argb8.val[0] = vqadd_u8(vshrn_n_u16(d_b16, 8), s_b8);
#elif defined(BLEND_WITH_DEST_ALPHA)
		uint16x8_t isd_a16 = vmull_u8(s_ia8, vmvn_u8(d_argb8.val[3]));
#if 1
		uint16x8_t s_a16 = vorrq_u16(hopa16, vmovl_u8(d_argb8.val[3]));
		d_argb8.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8)); //(255-((255-dopa)*(255-opa)>>8))
		vst1q_u16(tmpa, s_a16);
		tmpd[0] = TVPOpacityOnOpacityTable[tmpa[0]];
		tmpd[1] = TVPOpacityOnOpacityTable[tmpa[1]];
		tmpd[2] = TVPOpacityOnOpacityTable[tmpa[2]];
		tmpd[3] = TVPOpacityOnOpacityTable[tmpa[3]];
		tmpd[4] = TVPOpacityOnOpacityTable[tmpa[4]];
		tmpd[5] = TVPOpacityOnOpacityTable[tmpa[5]];
		tmpd[6] = TVPOpacityOnOpacityTable[tmpa[6]];
		tmpd[7] = TVPOpacityOnOpacityTable[tmpa[7]];
		s_a16 = vmovl_u8(vld1_u8(tmpd));
#else
        uint16x8_t d_a16 = vmovl_u8(d_argb8.val[3]);
        uint16x8_t sd_a16 = vmulq_u16(opa16, d_a16);
        uint16x8_t sopa = vshlq_n_u16(vaddq_u16(opa16, d_a16), 8);
        d_argb8.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8)); //(255-((255-dopa)*(255-opa)>>8))
        vst1q_u16(tmpa, vshrq_n_u16(vsubq_u16(sopa, sd_a16), 8));
        tmpa[0] = TVPRecipTableForOpacityOnOpacity[tmpa[0]];
        tmpa[1] = TVPRecipTableForOpacityOnOpacity[tmpa[1]];
        tmpa[2] = TVPRecipTableForOpacityOnOpacity[tmpa[2]];
        tmpa[3] = TVPRecipTableForOpacityOnOpacity[tmpa[3]];
        tmpa[4] = TVPRecipTableForOpacityOnOpacity[tmpa[4]];
        tmpa[5] = TVPRecipTableForOpacityOnOpacity[tmpa[5]];
        tmpa[6] = TVPRecipTableForOpacityOnOpacity[tmpa[6]];
        tmpa[7] = TVPRecipTableForOpacityOnOpacity[tmpa[7]];
        uint16x8_t s_a16 = vmulq_u16(vld1q_u16(tmpa), opa16);
		s_a16 = vshrq_n_u16(s_a16, 8);
#endif
        uint16x8_t d_r16 = vsubl_u8(s_r8, d_argb8.val[2]);
        uint16x8_t d_g16 = vsubl_u8(s_g8, d_argb8.val[1]);
        uint16x8_t d_b16 = vsubl_u8(s_b8, d_argb8.val[0]);
        d_r16 = vmulq_u16(d_r16, s_a16);
        d_g16 = vmulq_u16(d_g16, s_a16);
        d_b16 = vmulq_u16(d_b16, s_a16);
        d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));
#else
        uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], s_ia8);
        uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], s_ia8);
        uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], s_ia8);
        d_argb8.val[2] = vshrn_n_u16(vaddq_u16(d_r16, s_r16), 8);
        d_argb8.val[1] = vshrn_n_u16(vaddq_u16(d_g16, s_g16), 8);
        d_argb8.val[0] = vshrn_n_u16(vaddq_u16(d_b16, s_b16), 8);
#endif
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
        dest += 8;
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, pEndDst - dest, color, opa);
    }
}