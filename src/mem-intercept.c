/* DannyNiu/NJF, 2023-07-27. Public Domain. */

#include "common.h"
#include "siphash.h"

uint8_t shk[16] = {0};
uint32_t mh[4] = {0};

long allocs = 0;
long frees = 0;
bool trace = false;

void *mem_intercept(void *a)
{
    siphash_t hx;
    uintptr_t p = (uintptr_t)a;
    uint32_t ph[4];

    SipHash_o128_Init(&hx, shk, 16);
    SipHash_c2_Update(&hx, &p, sizeof(p));
    SipHash_c2d4o128_Final(&hx, ph, 16);

    for(p=0; p<4; p++) mh[p] ^= ph[p];
    return a;
}
