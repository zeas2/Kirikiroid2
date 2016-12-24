#ifdef _LOCAL_PROC_OVERLAY
sa = vmull_u8(vorr_u8(d.val[_I], mask1), s.val[_I]);
n = vtst_u8(s.val[_I], mask80); // n = d>=128
d1 = vand_u8(vand_u8(d.val[_I], n), maskFE), s1 = vand_u8(s.val[_I], n);
sa = vshrq_n_u16(sa, 7);
t = vshll_n_u8(vadd_u8(s1, d1), 1);
t = vsubw_u8(t, n);
t = vsubq_u16(t, sa);
s.val[_I] = vand_u8(vmovn_u16(t), n);
s.val[_I] = vorr_u8(s.val[_I], vand_u8(vmovn_u16(sa), vmvn_u8(n)));
#else
{
#define _LOCAL_PROC_OVERLAY
    uint16x8_t sa, t; uint8x8_t n, s1, d1;
#define _I 0
#include "ps_hardlightblend.h"
#undef _I
#define _I 1
#include "ps_hardlightblend.h"
#undef _I
#define _I 2
#include "ps_hardlightblend.h"
#undef _I
#undef _LOCAL_PROC_OVERLAY
}
#include "ps_alphablend.h"
#endif
