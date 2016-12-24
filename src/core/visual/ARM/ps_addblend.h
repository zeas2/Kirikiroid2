// s = sat(s, d)
{
    s.val[2] = vqadd_u8(s.val[2], d.val[2]);
    s.val[1] = vqadd_u8(s.val[1], d.val[1]);
    s.val[0] = vqadd_u8(s.val[0], d.val[0]);
}
#include "ps_alphablend.h"