/* DannyNiu/NJF, 2022-02-25. Public Domain. */

#include "s2dict.h"

#include <stdio.h>

#define klen 16

int main(void)
{
    int i, ret;
    int fails = 0;

    s2dict_t *x = NULL;
    s2data_t *v;

    s2data_t *k = NULL;
    char *kraw;

    s2iter_t *iter;

    x = s2dict_create();
    k = s2data_create(16);
    kraw = s2data_map(k, 0, 16);
    
    for(i=0; i<512; i++)
    {
        memset(kraw, 0, klen);
        snprintf(kraw, klen, "%d", i);
        
        v = s2data_create(sizeof(long));
        *(long *)s2data_map(v, 0, sizeof(long)) = i;
        s2data_unmap(v);
        s2dict_set(x, k, (void *)v, s2_setter_gave);
    }
    
    for(i=0; i<512; i++)
    {
        memset(kraw, 0, klen);
        snprintf(kraw, klen, "%d", i);
        
        ret = s2dict_get(x, k, (void *)&v);
        
        if( ret != s2_access_success )
        {
            fails ++;
            printf("Unable to get: %s, ret: %d.\n", kraw, ret);
        }

        if( (ret = (int)*(long *)s2data_map(v, 0, sizeof(long))) != i )
        {
            fails ++;
            printf("Wrong value: expected: %d, had %d.\n", i, ret);
        }
        s2data_unmap(v);
    }

    for(i=0; i<512; i++)
    {
        memset(kraw, 0, klen);
        snprintf(kraw, klen, "!%d", i);
        
        ret = s2dict_get(x, k, (void *)&v);

        if( ret != s2_access_nullval )
        {
            fails ++;
            printf("Shouldn't have been set for key: %s\n", kraw);
        }
    }

    iter = s2obj_iter_create((s2obj_t *)x);
    while( iter->next(iter) > 0 )
    {
        v = (s2data_t *)iter->value;
        printf("Value from iterator: %d.\n",
               (int)*(long *)s2data_map(v, 0, sizeof(long)));
        s2data_unmap(v);
    }

    for(i=256; i<512; i++)
    {
        memset(kraw, 0, klen);
        snprintf(kraw, klen, "%d", i);
        
        ret = s2dict_unset(x, k);
        if( ret != s2_access_success )
        {
            fails ++;
            printf("Failed to unset for key: %s\n", kraw);
        }

        ret = s2dict_unset(x, k);
        if( ret != s2_access_nullval )
        {
            fails ++;
            printf("Failed to verify unset for key: %s\n", kraw);
        }

        ret = s2dict_get(x, k, (void *)&v);
        if( ret != s2_access_nullval )
        {
            fails ++;
            printf("Should've been unset for key: %s\n", kraw);
        }
    }

    if( fails ) return EXIT_FAILURE; else return EXIT_SUCCESS;
}
