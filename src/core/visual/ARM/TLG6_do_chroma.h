case (N<<1):
    do {
        tjs_uint32 clr = *in;
		clr = FILTER;
		uint8x8_t v = vreinterpret_u8_u32(vdup_n_u32(clr));
		uint32x2_t u = vdup_n_u32(*prevline++);
#ifdef DEBUG_ARM_NEON
		uint8x8_t m = med_NEON(p, u, up);
#else
		med_NEON(p, u, up);
#endif
        p = vreinterpret_u32_u8(vadd_u8(m, v));
        *curline ++ = vget_lane_u32(p, 0);
        up = u;
        in += step;
    } while(--w);
    break;
case (N<<1)+1:
    do {
        tjs_uint32 clr = *in;
		clr = FILTER;
		uint8x8_t v = vreinterpret_u8_u32(vdup_n_u32(clr));
		up = vdup_n_u32(*prevline++);
		p = vreinterpret_u32_u8(vadd_u8(vrhadd_u8(vreinterpret_u8_u32(p), vreinterpret_u8_u32(up)), v));
        *curline ++ = vget_lane_u32(p, 0);
        in += step;
    } while(--w);
    break;