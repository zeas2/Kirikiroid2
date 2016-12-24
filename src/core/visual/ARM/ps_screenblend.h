// c = ((s+d-(s*d)/255)-d)*a + d = (s-(s*d)/255)*a + d
{
    uint16x8_t d_r16 = vmull_u8(s.val[2], d.val[2]);
    uint16x8_t d_g16 = vmull_u8(s.val[1], d.val[1]);
    uint16x8_t d_b16 = vmull_u8(s.val[0], d.val[0]);
    d_r16 = vsubq_u16(vmovl_u8(s.val[2]), vshrq_n_u16(d_r16, 8));
    d_g16 = vsubq_u16(vmovl_u8(s.val[1]), vshrq_n_u16(d_g16, 8));
    d_b16 = vsubq_u16(vmovl_u8(s.val[0]), vshrq_n_u16(d_b16, 8));
    d_r16 = vmulq_u16(d_r16, a);
    d_g16 = vmulq_u16(d_g16, a);
    d_b16 = vmulq_u16(d_b16, a);
    s.val[2] = vadd_u8(d.val[2], vshrn_n_u16(d_r16, 8));
    s.val[1] = vadd_u8(d.val[1], vshrn_n_u16(d_g16, 8));
    s.val[0] = vadd_u8(d.val[0], vshrn_n_u16(d_b16, 8));
}
