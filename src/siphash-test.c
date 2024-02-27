/* DannyNiu/NJF, 2023-03-02. Public Domain. */

#include "siphash.h"
#include "../tests/siphash-vectors.h"

#include <stdio.h>

int main(void)
{
    uint8_t in[64], out[16], k[16];
    int i, fails;

    siphash_t x;

    for(i=0; i<16; i++) k[i] = i;
    //- siphash_setkey(k, 16);

    for(i=fails=0; i<64; i++)
    {
        in[i] = i;

        SipHash_o128_Init(&x, k, 16);
        SipHash_c2_Update(&x, in, i);
        SipHash_c2d4o128_Final(&x, out, 16);
        
        if( memcmp(out, vectors_sip128[i], 16) )
            fails++;
        // else printf("SipHash-2-4-128[%d] Succeeded.\n", i);

        SipHash_o64_Init(&x, k, 16);
        SipHash_c2_Update(&x, in, i);
        SipHash_c2d4o64_Final(&x, out, 8);
        
        if( memcmp(out, vectors_sip64[i], 8) )
            fails++;
        // else printf("SipHash-2-4-64[%d] Succeeded.\n", i);
    }

    printf("%d test(s) failed.\n", fails);
    if( fails ) return EXIT_FAILURE; else EXIT_SUCCESS;
}
