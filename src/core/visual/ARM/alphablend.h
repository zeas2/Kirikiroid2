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
    {
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
    }
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
    unsigned char strechbuff[32 + 16];
    tjs_uint32 *strechsrc = __builtin_assume_aligned((tjs_uint32*)((((intptr_t)strechbuff) + 15) & ~15), 16);
#endif
#ifdef BLEND_WITH_OPACITY
    uint8x8_t opa8 = vdup_n_u8(opa);
#endif
#ifdef BLEND_WITH_DEST_ALPHA
    unsigned char tmpbuff[32 + 16];
    unsigned short *tmpsa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 15) & ~15), 16);
    unsigned char *tmpa = __builtin_assume_aligned((unsigned char*)(tmpsa + 8), 16);
#ifdef BLEND_WITH_OPACITY
    uint16x8_t soparev = vdupq_n_u16(0x00ff);
#endif
#endif
    while(dest < pVecEndDst) {
#if defined(STRECH_FUNC) || defined(LINEAR_TRANS_FUNC)
        for(int i = 0; i < 8; ++i) {
#ifdef STRECH_FUNC
            strechsrc[i] = src[(srcstart) >> 16];
            srcstart += srcstep;
#elif defined(LINEAR_TRANS_FUNC)
            strechsrc[i] = *( (const tjs_uint32*)((const tjs_uint8*)src + ((sy)>>16)*srcpitch) + ((sx)>>16));
            sx += stepx;
            sy += stepy;
#endif
        }
        uint8x8x4_t s_argb8 = vld4_u8((unsigned char *)strechsrc);
#else
		//__builtin_prefetch(src, 0, 0);
		uint8x8x4_t s_argb8 = vld4_u8((unsigned char *)src);
#endif
		//__builtin_prefetch(dest, 0, 0);
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
#ifdef BLEND_WITH_OPACITY
		uint16x8_t s_a16 = vmull_u8(s_argb8.val[3], opa8);
		s_a16 = vshrq_n_u16(s_a16, 8);
#else
		uint16x8_t s_a16 = vmovl_u8(s_argb8.val[3]);
#endif
#ifdef BLEND_WITH_ADDALPHA
        {
#ifdef BLEND_WITH_OPACITY
            s_argb8.val[3] = vmovn_u16(s_a16);
#endif
            uint16x8_t d_a16 = vmull_u8(s_argb8.val[3], d_argb8.val[3]);
            uint16x8_t s_r16 = vmull_u8(s_argb8.val[2], s_argb8.val[3]);
            uint16x8_t s_g16 = vmull_u8(s_argb8.val[1], s_argb8.val[3]);
            uint16x8_t s_b16 = vmull_u8(s_argb8.val[0], s_argb8.val[3]);
            //Da = Sa + Da - SaDa
            d_a16 = vsubq_u16(vaddl_u8(s_argb8.val[3], d_argb8.val[3]), vshrq_n_u16(d_a16, 8));
            d_argb8.val[3] = vmovn_u16(vsubq_u16(d_a16, vshrq_n_u16(d_a16, 8)));
            s_argb8.val[2] = vshrn_n_u16(s_r16, 8);
            s_argb8.val[1] = vshrn_n_u16(s_g16, 8);
            s_argb8.val[0] = vshrn_n_u16(s_b16, 8);
        }

        // Di = sat(Si, (1-Sa)*Di)
        s_argb8.val[3] = vmvn_u8(s_argb8.val[3]);

        uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], s_argb8.val[3]);
        uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], s_argb8.val[3]);
        uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], s_argb8.val[3]);

        // 8-bit to do saturated add
        d_argb8.val[2] = vqadd_u8(s_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vqadd_u8(s_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vqadd_u8(s_argb8.val[0], vshrn_n_u16(d_b16, 8));
#else // BLEND_WITH_ADDALPHA
#ifdef BLEND_WITH_DEST_ALPHA
        //( 255 - (255-a)*(255-b)/ 255 ); 
#ifdef BLEND_WITH_OPACITY
		uint16x8_t isd_a16 = vmulq_u16(veorq_u16(s_a16, soparev), vmovl_u8(vmvn_u8(d_argb8.val[3])));
#else
        uint16x8_t isd_a16 = vmull_u8(vmvn_u8(s_argb8.val[3]), vmvn_u8(d_argb8.val[3]));
#endif

#ifdef BLEND_WITH_OPACITY
		uint16x8_t sopa = vorrq_u16(vshlq_n_u16(s_a16, 8), vmovl_u8(d_argb8.val[3]));
#else
		uint16x8_t sopa = vorrq_u16(vshll_n_u8(s_argb8.val[3], 8), vmovl_u8(d_argb8.val[3]));
#endif
		d_argb8.val[3] = vmvn_u8(vshrn_n_u16(isd_a16, 8));
		vst1q_u16(tmpsa, sopa);
		tmpa[0] = TVPOpacityOnOpacityTable[tmpsa[0]];
        tmpa[1] = TVPOpacityOnOpacityTable[tmpsa[1]];
        tmpa[2] = TVPOpacityOnOpacityTable[tmpsa[2]];
        tmpa[3] = TVPOpacityOnOpacityTable[tmpsa[3]];
        tmpa[4] = TVPOpacityOnOpacityTable[tmpsa[4]];
        tmpa[5] = TVPOpacityOnOpacityTable[tmpsa[5]];
        tmpa[6] = TVPOpacityOnOpacityTable[tmpsa[6]];
        tmpa[7] = TVPOpacityOnOpacityTable[tmpsa[7]];
        s_a16 = vmovl_u8(vld1_u8(tmpa));
#endif

        // d + (s - d) * sa
        uint16x8_t d_r16 = vsubl_u8(s_argb8.val[2], d_argb8.val[2]);
        uint16x8_t d_g16 = vsubl_u8(s_argb8.val[1], d_argb8.val[1]);
        uint16x8_t d_b16 = vsubl_u8(s_argb8.val[0], d_argb8.val[0]);

        d_r16 = vmulq_u16(d_r16, s_a16);
        d_g16 = vmulq_u16(d_g16, s_a16);
        d_b16 = vmulq_u16(d_b16, s_a16);
        
        d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));
#endif //BLEND_WITH_ADDALPHA
        vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);

#ifdef STRECH_FUNC
#elif defined(LINEAR_TRANS_FUNC)
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