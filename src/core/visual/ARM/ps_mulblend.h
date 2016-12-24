{
    uint16x8_t d_r16 = vmull_u8(s.val[2], d.val[2]);
    uint16x8_t d_g16 = vmull_u8(s.val[1], d.val[1]);
    uint16x8_t d_b16 = vmull_u8(s.val[0], d.val[0]);
    s.val[2] = vshrn_n_u16(d_r16, 8);
    s.val[1] = vshrn_n_u16(d_g16, 8);
    s.val[0] = vshrn_n_u16(d_b16, 8);
}
#include "ps_alphablend.h"