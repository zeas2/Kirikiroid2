static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest,
#ifdef STRECH_FUNC
    tjs_int len, const tjs_uint32 *src, tjs_int srcstart, tjs_int srcstep
#elif defined(LINEAR_TRANS_FUNC)
    tjs_int len, const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch
#else
    const tjs_uint32 *src, tjs_int len
#endif
    , tjs_int copa)
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
            ,copa);
        dest += PreFragLen;
#ifdef STRECH_FUNC
        srcstart += srcstep * PreFragLen;
#elif defined(LINEAR_TRANS_FUNC)
        sx += stepx * PreFragLen;
        sy += stepy * PreFragLen;
#else
        src += PreFragLen;
#endif
    }
    tjs_int opa = copa;
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
    unsigned char strechbuff[32 + 16];
	tjs_uint32 *strechsrc = __builtin_assume_aligned((tjs_uint32*)((((intptr_t)strechbuff) + 15) & ~15), 16);
    if(opa > 128) opa ++; /* adjust for error */
#endif


#ifdef BLEND_WITH_DEST_ALPHA
    unsigned char tmpbuff[32 + 8];
	unsigned short *tmpa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 15) & ~15), 16);
	unsigned char *tmpd = __builtin_assume_aligned((unsigned char*)(tmpa + 8), 16);
    //uint16x8_t opa16 = vdupq_n_u16(opa);
	uint16x8_t hopa16 = vdupq_n_u16(copa << 8);
	uint8x8_t iopa8 = vdup_n_u8(~opa);
#endif
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
    while(dest < pVecEndDst) {
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
        for(int i = 0; i < 8; ++i) {
#ifdef STRECH_FUNC
            strechsrc[i] = src[(srcstart) >> 16];
            srcstart += srcstep;
#elif defined(LINEAR_TRANS_FUNC)
            strechsrc[i] = *( (const tjs_uint32*)((const tjs_uint8*)src + (sy>>16)*srcpitch) + (sx>>16));
            sx += stepx;
            sy += stepy;
#endif
        }
        uint8x8x4_t s = vld4_u8((unsigned char *)strechsrc);
#else
        uint8x8x4_t s = vld4_u8((unsigned char *)src);
#endif
		uint8x8x4_t d = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
#ifdef BLEND_WITH_ADDALPHA
        s.val[3] = vdup_n_u8(opa);
        uint16x8_t d_a16 = vmull_u8(s.val[3], d.val[3]);
        //Da = Sa + Da - SaDa
        d_a16 = vsubq_u16(vaddl_u8(s.val[3], d.val[3]), vshrq_n_u16(d_a16, 8));
        d.val[3] = vmovn_u16(vsubq_u16(d_a16, vshrq_n_u16(d_a16, 8)));

        // Di = sat(Si, (1-Sa)*Di)
        s.val[3] = vmvn_u8(s.val[3]);
        uint16x8_t d_r16 = vmull_u8(d.val[2], s.val[3]);
        uint16x8_t d_g16 = vmull_u8(d.val[1], s.val[3]);
        uint16x8_t d_b16 = vmull_u8(d.val[0], s.val[3]);

        // 8-bit to do saturated add
        d.val[2] = vqadd_u8(s.val[2], vshrn_n_u16(d_r16, 8));
        d.val[1] = vqadd_u8(s.val[1], vshrn_n_u16(d_g16, 8));
        d.val[0] = vqadd_u8(s.val[0], vshrn_n_u16(d_b16, 8));
#else
#ifdef BLEND_WITH_DEST_ALPHA
        uint16x8_t isd_a16 = vmull_u8(iopa8, vmvn_u8(d.val[3]));
#if 1
		uint16x8_t s_a16 = vorrq_u16(hopa16, vmovl_u8(d.val[3]));
		d.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
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
        uint16x8_t d_a16 = vmovl_u8(d.val[3]);
        uint16x8_t sd_a16 = vmulq_u16(opa16, d_a16);
        uint16x8_t sopa = vshlq_n_u16(vaddq_u16(opa16, d_a16), 8);
        d.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
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

        // d = d + (s - d) * opa
        uint16x8_t d_r16 = vsubl_u8(s.val[2], d.val[2]);
        uint16x8_t d_g16 = vsubl_u8(s.val[1], d.val[1]);
        uint16x8_t d_b16 = vsubl_u8(s.val[0], d.val[0]);
        d_r16 = vmulq_u16(d_r16, s_a16);
        d_g16 = vmulq_u16(d_g16, s_a16);
        d_b16 = vmulq_u16(d_b16, s_a16);
#else
        // d = d + (s - d) * opa
        uint16x8_t d_r16 = vmulq_n_u16(vsubl_u8(s.val[2], d.val[2]), opa);
        uint16x8_t d_g16 = vmulq_n_u16(vsubl_u8(s.val[1], d.val[1]), opa);
        uint16x8_t d_b16 = vmulq_n_u16(vsubl_u8(s.val[0], d.val[0]), opa);
#endif // BLEND_WITH_DEST_ALPHA
        d.val[2] = vadd_u8(d.val[2], vshrn_n_u16(d_r16, 8));
        d.val[1] = vadd_u8(d.val[1], vshrn_n_u16(d_g16, 8));
        d.val[0] = vadd_u8(d.val[0], vshrn_n_u16(d_b16, 8));
#endif // BLEND_WITH_ADDALPHA
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d);
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
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
            ,copa);
    }
}