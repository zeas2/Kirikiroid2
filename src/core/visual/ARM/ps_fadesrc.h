// s = s * a
{
    uint16x8_t s_r16 = vmulq_u16(vmovl_u8(s.val[2]), a);
    uint16x8_t s_g16 = vmulq_u16(vmovl_u8(s.val[1]), a);
    uint16x8_t s_b16 = vmulq_u16(vmovl_u8(s.val[0]), a);

    s.val[2] = vshrn_n_u16(s_r16, 8);
    s.val[1] = vshrn_n_u16(s_g16, 8);
    s.val[0] = vshrn_n_u16(s_b16, 8);
}