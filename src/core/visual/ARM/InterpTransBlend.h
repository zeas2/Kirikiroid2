static void _CAT_NAME(FUNC_NAME, _NEON)(
tjs_uint32 *dest, tjs_int len
#ifdef LINEAR_TRANS_FUNC
    , const tjs_uint32 *src, tjs_int sx, tjs_int sy, tjs_int stepx, tjs_int stepy, tjs_int srcpitch
#else //STRECH_FUNC
    , const tjs_uint32 *src1, const tjs_uint32 *src2, tjs_int _blend_y, tjs_int srcstart, tjs_int srcstep
#endif // LINEAR_TRANS_FUNC
#ifdef BLEND_WITH_OPACITY
    , tjs_int _opa
#endif
    )
{
    tjs_uint32* pEndDst = dest + len;
    {
        tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
        if(PreFragLen > len) PreFragLen = len;
        if(PreFragLen) {
            C_FUNC_NAME(dest, PreFragLen
#ifdef LINEAR_TRANS_FUNC
                , src, sx, sy, stepx, stepy, srcpitch
#else //STRECH_FUNC
                , src1, src2, _blend_y, srcstart, srcstep
#endif // LINEAR_TRANS_FUNC
#ifdef BLEND_WITH_OPACITY
                , _opa
#endif
                );
            dest += PreFragLen;
#ifdef LINEAR_TRANS_FUNC
            sx += stepx * PreFragLen;
            sy += stepy * PreFragLen;
#else //STRECH_FUNC
            srcstart += PreFragLen * srcstep;
#endif // LINEAR_TRANS_FUNC
        }
    }

#ifdef LINEAR_TRANS_FUNC
    unsigned char tmpbuff[4 * 8 * 4 + 2 * 8 * 2 + 16];
    tjs_uint32 *tmp0_0 = (tjs_uint32*)((((intptr_t)tmpbuff) + 15) & ~15);
    tjs_uint32 *tmp0_1 = tmp0_0 + 8;
    tjs_uint32 *tmp1_0 = tmp0_1 + 8;
    tjs_uint32 *tmp1_1 = tmp1_0 + 8;
    uint16_t *blend_x = (uint16_t *)(tmp1_1 + 8);
    uint16_t *blend_y = (blend_x + 8);
#else //STRECH_FUNC
    tjs_int blend_y = _blend_y + (_blend_y >> 7); /* adjust blend ratio */

    unsigned char tmpbuff[4 * 8 * 3 + 16];
    tjs_uint32 *tmp1_0 = (tjs_uint32*)((((intptr_t)tmpbuff) + 15) & ~15);
    tjs_uint32 *tmp1_1 = tmp1_0 + 8;
    uint16_t *blend_x = (uint16_t *)(tmp1_1 + 8);
#endif // LINEAR_TRANS_FUNC

    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;

#ifdef BLEND_WITH_ADDALPHA
#ifdef BLEND_WITH_OPACITY
    uint8x8_t opa8 = vdup_n_u8(_opa);
#endif
#elif defined(BLEND_WITH_OPACITY)
    tjs_int opa = _opa + (_opa >> 7); /* adjust opa */
#endif
    while(dest < pVecEndDst) {
        uint8x8x4_t s_argb8;
#ifdef STRECH_FUNC
        tjs_int start = srcstart;
#endif
        {
            for(int i = 0; i < 8; ++i) {
#ifdef LINEAR_TRANS_FUNC
                const tjs_uint32 *p0, *p1;
                int bld_x, bld_y;
                bld_x = (sx & 0xffff) >> 8;
                bld_x += bld_x >> 7;
                bld_y = (sy & 0xffff) >> 8;
                bld_y += bld_y >> 7;
                blend_x[i] = bld_x;
                blend_y[i] = bld_y;

                p0 = (const tjs_uint32*)((const tjs_uint8*)src + ((sy>>16)  )*srcpitch) + (sx>>16);
                p1 = (const tjs_uint32*)((const tjs_uint8*)p0 + srcpitch);

                tmp0_0[i] = p0[0];
                tmp0_1[i] = p0[1];
                tmp1_0[i] = p1[0];
                tmp1_1[i] = p1[1];

                sx += stepx;
                sy += stepy;
#else //STRECH_FUNC
                int addr = start >> 16;
                tmp1_0[i] = src2[addr];
                tmp1_1[i] = src2[addr + 1];
                blend_x[i] = (start & 0xffff) >> 8;
                start += srcstep;
#endif // LINEAR_TRANS_FUNC
            }
            // TVPBlendARGB(src2[sp], src2[sp+1], blend_x)
            uint8x8x4_t b = vld4_u8((unsigned char *)tmp1_0);
            uint8x8x4_t a = vld4_u8((unsigned char *)tmp1_1);
            uint16x8_t ratio = vld1q_u16(blend_x); // qreg = 5
            // TVPBlendARGB: a * ratio + b * (1 - ratio) => b + (a - b) * ratio
            uint16x8_t s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
            uint16x8_t s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
            uint16x8_t s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
            uint16x8_t s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio); // qreg = 9

            s_argb8.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
            s_argb8.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
            s_argb8.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
            s_argb8.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 11

#ifdef LINEAR_TRANS_FUNC
            b = vld4_u8((unsigned char *)tmp0_0);
            a = vld4_u8((unsigned char *)tmp0_1);
#else //STRECH_FUNC
            start = srcstart;
            for(int i = 0; i < 8; ++i) {
                int addr = (start) >> 16;
                tmp1_0[i] = src1[addr];
                tmp1_1[i] = src1[addr + 1];
                start += srcstep;
            }
            // TVPBlendARGB(src1[sp], src1[sp+1], blend_x)
            b = vld4_u8((unsigned char *)tmp1_0);
            a = vld4_u8((unsigned char *)tmp1_1);
#endif // LINEAR_TRANS_FUNC
            s_a16 = vmulq_u16(vsubl_u8(a.val[3], b.val[3]), ratio);
            s_r16 = vmulq_u16(vsubl_u8(a.val[2], b.val[2]), ratio);
            s_g16 = vmulq_u16(vsubl_u8(a.val[1], b.val[1]), ratio);
            s_b16 = vmulq_u16(vsubl_u8(a.val[0], b.val[0]), ratio);
            uint8x8x4_t s2;
            s2.val[3] = vadd_u8(b.val[3], vshrn_n_u16(s_a16, 8));
            s2.val[2] = vadd_u8(b.val[2], vshrn_n_u16(s_r16, 8));
            s2.val[1] = vadd_u8(b.val[1], vshrn_n_u16(s_g16, 8));
            s2.val[0] = vadd_u8(b.val[0], vshrn_n_u16(s_b16, 8)); // qreg = 13

            // TVPBlendARGB
#ifdef LINEAR_TRANS_FUNC
            ratio = vld1q_u16(blend_y);
            s_a16 = vmulq_u16(vsubl_u8(s_argb8.val[3], s2.val[3]), ratio);
            s_r16 = vmulq_u16(vsubl_u8(s_argb8.val[2], s2.val[2]), ratio);
            s_g16 = vmulq_u16(vsubl_u8(s_argb8.val[1], s2.val[1]), ratio);
            s_b16 = vmulq_u16(vsubl_u8(s_argb8.val[0], s2.val[0]), ratio);
#else //STRECH_FUNC
            s_a16 = vmulq_n_u16(vsubl_u8(s_argb8.val[3], s2.val[3]), blend_y);
            s_r16 = vmulq_n_u16(vsubl_u8(s_argb8.val[2], s2.val[2]), blend_y);
            s_g16 = vmulq_n_u16(vsubl_u8(s_argb8.val[1], s2.val[1]), blend_y);
            s_b16 = vmulq_n_u16(vsubl_u8(s_argb8.val[0], s2.val[0]), blend_y);
#endif // LINEAR_TRANS_FUNC
            s_argb8.val[3] = vadd_u8(s2.val[3], vshrn_n_u16(s_a16, 8));
            s_argb8.val[2] = vadd_u8(s2.val[2], vshrn_n_u16(s_r16, 8));
            s_argb8.val[1] = vadd_u8(s2.val[1], vshrn_n_u16(s_g16, 8));
            s_argb8.val[0] = vadd_u8(s2.val[0], vshrn_n_u16(s_b16, 8));
#ifdef BLEND_WITH_OPACITY
#ifdef BLEND_WITH_ADDALPHA
            {
                s_a16 = vmull_u8(s_argb8.val[3], opa8);
                s_r16 = vmull_u8(s_argb8.val[2], opa8);
                s_g16 = vmull_u8(s_argb8.val[1], opa8);
                s_b16 = vmull_u8(s_argb8.val[0], opa8);
                s_argb8.val[3] = vshrn_n_u16(s_a16, 8);
                s_argb8.val[2] = vshrn_n_u16(s_r16, 8);
                s_argb8.val[1] = vshrn_n_u16(s_g16, 8);
                s_argb8.val[0] = vshrn_n_u16(s_b16, 8);
            }
#endif
#endif
        }
#ifdef COPY_FUNC
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), s_argb8);
#else
		uint8x8x4_t d_argb8 = vld4_u8(__builtin_assume_aligned((uint8_t *)dest, 8));
#ifdef BLEND_WITH_ADDALPHA
        // TVPAddAlphaBlend_n_a
        s_argb8.val[3] = vmvn_u8(s_argb8.val[3]); // 1 - a

        // s + d * (1 - sa)
        uint16x8_t d_r16 = vmull_u8(d_argb8.val[2], s_argb8.val[3]);
        uint16x8_t d_g16 = vmull_u8(d_argb8.val[1], s_argb8.val[3]);
        uint16x8_t d_b16 = vmull_u8(d_argb8.val[0], s_argb8.val[3]);

        // 8-bit to do saturated add
        d_argb8.val[2] = vqadd_u8(vshrn_n_u16(d_r16, 8), s_argb8.val[2]);
        d_argb8.val[1] = vqadd_u8(vshrn_n_u16(d_g16, 8), s_argb8.val[1]);
        d_argb8.val[0] = vqadd_u8(vshrn_n_u16(d_b16, 8), s_argb8.val[0]);
#else
        // TVPBlendARGB
        uint16x8_t d_a16 = vmulq_n_u16(vsubl_u8(s_argb8.val[3], d_argb8.val[3]), opa);
        uint16x8_t d_r16 = vmulq_n_u16(vsubl_u8(s_argb8.val[2], d_argb8.val[2]), opa);
        uint16x8_t d_g16 = vmulq_n_u16(vsubl_u8(s_argb8.val[1], d_argb8.val[1]), opa);
        uint16x8_t d_b16 = vmulq_n_u16(vsubl_u8(s_argb8.val[0], d_argb8.val[0]), opa);
        d_argb8.val[3] = vadd_u8(d_argb8.val[3], vshrn_n_u16(d_a16, 8));
        d_argb8.val[2] = vadd_u8(d_argb8.val[2], vshrn_n_u16(d_r16, 8));
        d_argb8.val[1] = vadd_u8(d_argb8.val[1], vshrn_n_u16(d_g16, 8));
        d_argb8.val[0] = vadd_u8(d_argb8.val[0], vshrn_n_u16(d_b16, 8));
#endif
		vst4_u8(__builtin_assume_aligned((uint8_t *)dest, 8), d_argb8);
#endif

#ifdef STRECH_FUNC
        srcstart = start;
#endif
        dest += 8;
    }

    if(dest < pEndDst) {
        C_FUNC_NAME(dest, pEndDst - dest
#ifdef LINEAR_TRANS_FUNC
            , src, sx, sy, stepx, stepy, srcpitch
#else //STRECH_FUNC
            , src1, src2, _blend_y, srcstart, srcstep
#endif // LINEAR_TRANS_FUNC
#ifdef BLEND_WITH_OPACITY
            , _opa
#endif
            );
    }
}