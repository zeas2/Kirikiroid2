// d + (s - d) * sa
{
    uint16x8_t d_r16 = vsubl_u8(s.val[2], d.val[2]);
    uint16x8_t d_g16 = vsubl_u8(s.val[1], d.val[1]);
    uint16x8_t d_b16 = vsubl_u8(s.val[0], d.val[0]);
    d_r16 = vmulq_u16(d_r16, a);
    d_g16 = vmulq_u16(d_g16, a);
    d_b16 = vmulq_u16(d_b16, a);

    s.val[2] = vadd_u8(d.val[2], vshrn_n_u16(d_r16, 8));
    s.val[1] = vadd_u8(d.val[1], vshrn_n_u16(d_g16, 8));
    s.val[0] = vadd_u8(d.val[0], vshrn_n_u16(d_b16, 8));
}