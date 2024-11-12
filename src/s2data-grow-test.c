/* DannyNiu/NJF, 2024-11-12. Public Domain. */

#include "s2data.h"

#define min(a,b) (a>b?b:a)

int main()
{
    s2data_t *ctx = s2data_create(0);
    int i, j, k;
    uint8_t *ptr;
    int ret = EXIT_SUCCESS;

    for(i=1; i<256; i++)
    {
        s2data_trunc(ctx, 0);
        for(j=0; j<256; j+=i)
        {
            for(k=j; k<j+i && k<256; k++)
            {
                s2data_putc(ctx, k);
            }
            s2data_putfin(ctx);

            ptr = s2data_map(ctx, 0, k);
            for(k=0; k<j+i && k<256; k++)
            {
                if( ptr[k] != k )
                {
                    fprintf(stderr, "Wrong byte value!\n");
                    fprintf(stderr,
                            "i=%d, "
                            "j=%d, "
                            "k=%d, "
                            "*=%d.\n",
                            i, j, k, ptr[k]);

                    ret = EXIT_FAILURE;
                }
            }
            s2data_unmap(ctx);
            if( ret != EXIT_SUCCESS )
                return ret;
        }
    }

    return ret;
}
