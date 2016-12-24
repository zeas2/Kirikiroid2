// s <- d - (1 - s)
{
    s.val[2] = vqsub_u8(d.val[2], vmvn_u8(s.val[2]));
    s.val[1] = vqsub_u8(d.val[1], vmvn_u8(s.val[1]));
    s.val[0] = vqsub_u8(d.val[0], vmvn_u8(s.val[0]));
}
#include "ps_alphablend.h"