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

        // 2024-08-01.a: # this note is referred to from other tests.
        // The following is a short comparison of verbosity of previous
        // language/library constructs and the current one. Note that
        // the previous versions couldn't enforce strong type safety,
        // and explicit castings were used.
        //- s2dict_set(x, k, (s2obj_t *)v, s2_setter_gave);
        //- s2dict_set(x, k, (void *)v, s2_setter_gave);
        ;;  s2dict_set(x, k, &v->base, s2_setter_gave);
    }
    
    for(i=0; i<512; i++)
    {
        memset(kraw, 0, klen);
        snprintf(kraw, klen, "%d", i);

        // 2024-08-01.b:
        // This is a nested pointer, and unfortunately
        // cannot have strong type safety enforcement.
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
        
        // 2024-08-01: see note labelled 2024-08-01.b.
        ret = s2dict_get(x, k, (void *)&v);

        if( ret != s2_access_nullval )
        {
            fails ++;
            printf("Shouldn't have been set for key: %s\n", kraw);
        }
    }

    // 2024-08-01: see note labelled 2024-08-01.a.
    //- iter = s2obj_iter_create((s2obj_t *)x);
    //- iter = s2obj_iter_create((void *)x);
    ;;  iter = s2obj_iter_create(&x->base);
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

        // 2024-08-01: see note labelled 2024-08-01.b.
        ret = s2dict_get(x, k, (void *)&v);
        if( ret != s2_access_nullval )
        {
            fails ++;
            printf("Should've been unset for key: %s\n", kraw);
        }
    }

    if( fails ) return EXIT_FAILURE; else return EXIT_SUCCESS;
}
