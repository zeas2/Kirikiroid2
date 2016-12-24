{
    s.val[2] = vmax_u8(s.val[2], d.val[2]);
    s.val[1] = vmax_u8(s.val[1], d.val[1]);
    s.val[0] = vmax_u8(s.val[0], d.val[0]);
}
#include "ps_alphablend.h"