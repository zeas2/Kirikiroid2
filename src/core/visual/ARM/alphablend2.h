static void _CAT_NAME(FUNC_NAME, _NEON)(tjs_uint32 *dest, const tjs_uint32 *src1, const tjs_uint32 *src2,
#if defined(UNIV_TRANS)
    const tjs_uint8 *rule, const tjs_uint32 *table, tjs_int len
#ifdef UNIV_TRANS_SWITCH
    , tjs_int src1lv, tjs_int src2lv
#endif
#endif
    )
{
    tjs_uint32* pEndDst = dest + len;
    tjs_int PreFragLen = (tjs_uint32*)((((intptr_t)dest) + 7)&~7) - dest;
    if(PreFragLen > len) PreFragLen = len;
    if(PreFragLen) {
        C_FUNC_NAME(dest, src1, src2,
#if defined(UNIV_TRANS)
            rule, table, PreFragLen
#ifdef UNIV_TRANS_SWITCH
            , src1lv, src2lv
#endif
#endif
            );
        dest += PreFragLen;
        src1 += PreFragLen;
        src2 += PreFragLen;
#ifdef UNIV_TRANS
        rule += PreFragLen;
#endif
    }
    tjs_uint32* pVecEndDst = (tjs_uint32*)(((intptr_t)pEndDst)&~7)-7;
#ifdef BLEND_WITH_DEST_ALPHA
    unsigned char tmpbuff[32 + 16];
	unsigned short *tmpa = __builtin_assume_aligned((unsigned short*)((((intptr_t)tmpbuff) + 15) & ~15), 16);
	unsigned char  *tmpd = __builtin_assume_aligned((unsigned char *)(tmpa + 8), 16);
    uint16x8_t const256 = vdupq_n_u16(256);
#endif
#ifdef UNIV_TRANS
#ifdef UNIV_TRANS_SWITCH
	unsigned char opabuff[16 + 16];
	unsigned short *tmpo = __builtin_assume_aligned((unsigned short*)((((intptr_t)opabuff) + 15) & ~15), 16);
#endif
#endif
    while(dest < pVecEndDst) {
#ifdef UNIV_TRANS
		uint16x8_t o16;
#ifdef UNIV_TRANS_SWITCH
		for (int i = 0; i < 8; ++i) {
            tjs_int opa = *rule++;
            if(opa >= src1lv) {
                tmpo[i] = 0;
            } else if(opa < src2lv) {
                tmpo[i] = 255;
            } else {
                tmpo[i] = table[opa];
            }
			o16 = vld1q_u16(tmpo);
		}
#else
		o16 = vsetq_lane_u16(table[*rule++], o16, 0);
		o16 = vsetq_lane_u16(table[*rule++], o16, 1);
		o16 = vsetq_lane_u16(table[*rule++], o16, 2);
		o16 = vsetq_lane_u16(table[*rule++], o16, 3);
		o16 = vsetq_lane_u16(table[*rule++], o16, 4);
		o16 = vsetq_lane_u16(table[*rule++], o16, 5);
		o16 = vsetq_lane_u16(table[*rule++], o16, 6);
		o16 = vsetq_lane_u16(table[*rule++], o16, 7);
#endif
#endif
        uint8x8x4_t s1 = vld4_u8((unsigned char *)src1); 
#ifdef BLEND_WITH_DEST_ALPHA
        uint16x8_t s1_a16 = vmulq_u16(vmovl_u8(s1.val[3]), vsubq_u16(const256, o16)); // a1*(256-opa)
#endif
        uint8x8x4_t s2 = vld4_u8((unsigned char *)src2);
#ifdef BLEND_WITH_DEST_ALPHA
        uint16x8_t d_a16 = vmulq_u16(vsubl_u8(s2.val[3], s1.val[3]), o16);
        o16 = vmulq_u16(vmovl_u8(s2.val[3]), o16);
        o16 = vsriq_n_u16(o16, s1_a16, 8); // addr
        s2.val[3] = vadd_u8(s1.val[3], vshrn_n_u16(d_a16, 8));
        vst1q_u16(tmpa, o16);
#ifdef UNIV_TRANS_SWITCH
		tmpd[0] = TVPNegativeMulTable[tmpa[0]];
		tmpd[1] = TVPNegativeMulTable[tmpa[1]];
		tmpd[2] = TVPNegativeMulTable[tmpa[2]];
		tmpd[3] = TVPNegativeMulTable[tmpa[3]];
		tmpd[4] = TVPNegativeMulTable[tmpa[4]];
		tmpd[5] = TVPNegativeMulTable[tmpa[5]];
		tmpd[6] = TVPNegativeMulTable[tmpa[6]];
		tmpd[7] = TVPNegativeMulTable[tmpa[7]];
#endif
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[0]], o16, 0);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[1]], o16, 1);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[2]], o16, 2);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[3]], o16, 3);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[4]], o16, 4);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[5]], o16, 5);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[6]], o16, 6);
// 		o16 = vsetq_lane_u16(TVPOpacityOnOpacityTable[tmpa[7]], o16, 7);

		tmpa[0] = TVPOpacityOnOpacityTable[tmpa[0]];
		tmpa[1] = TVPOpacityOnOpacityTable[tmpa[1]];
		tmpa[2] = TVPOpacityOnOpacityTable[tmpa[2]];
		tmpa[3] = TVPOpacityOnOpacityTable[tmpa[3]];
		tmpa[4] = TVPOpacityOnOpacityTable[tmpa[4]];
		tmpa[5] = TVPOpacityOnOpacityTable[tmpa[5]];
		tmpa[6] = TVPOpacityOnOpacityTable[tmpa[6]];
		tmpa[7] = TVPOpacityOnOpacityTable[tmpa[7]];
        o16 = vld1q_u16(tmpa);
#endif
        // s1 * o + s2 * (1 - o) => s1 + (s2 - s1) * o
#ifdef BLEND_WITH_ADDALPHA
        uint16x8_t d_a16 = vmulq_u16(vsubl_u8(s2.val[3], s1.val[3]), o16);
#endif
        uint16x8_t d_r16 = vmulq_u16(vsubl_u8(s2.val[2], s1.val[2]), o16);
        uint16x8_t d_g16 = vmulq_u16(vsubl_u8(s2.val[1], s1.val[1]), o16);
        uint16x8_t d_b16 = vmulq_u16(vsubl_u8(s2.val[0], s1.val[0]), o16);

        // s * o >> 8
#ifdef BLEND_WITH_ADDALPHA
        s2.val[3] = vadd_u8(s1.val[3], vshrn_n_u16(d_a16, 8));
#else
        //s2.val[3] = vdup_n_u8(0);
#endif
#ifdef BLEND_WITH_DEST_ALPHA
#ifdef UNIV_TRANS_SWITCH
        s2.val[3] = vld1_u8(tmpd);
#endif
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
#if defined(UNIV_TRANS)
            rule, table, pEndDst - dest
#ifdef UNIV_TRANS_SWITCH
            , src1lv, src2lv
#endif
#endif
            );
    }
}