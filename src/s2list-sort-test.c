/* DannyNiu/NJF, 2023-07-28. Public Domain. */

#define safetypes2_implementing_misc
#include "s2list.h"
#include <time.h>

typedef struct {
    s2obj_t base;
    int v;
} s2val_t;

#define VALS_LEN 120
#define VAL_MOD 97
s2val_t *vals[VALS_LEN] = {0};

s2list_t *list;

int s2val_cmp(s2val_t *a, s2val_t *b)
{
    return a->v < b->v ? -1 : a->v == b->v ? 0 : 1;
}

int main()
{
    int i, l, s;
    s2iter_t *iter;
    
    srand(time(NULL));

    for(l=0; l<VALS_LEN; l+=l/10+1)
    {
        list = s2list_create();
        for(i=0; i<l; i++)
        {
            if( !vals[i] )
                vals[i] = (s2val_t *)s2gc_obj_alloc(0x0104, sizeof(s2val_t));

            vals[i]->v = rand() % VAL_MOD;

            // See 2024-08-01.a note in "s2dict-test.c"
            //- s2list_push(list, (s2obj_t *)vals[i], s2_setter_kept);
            ;;  s2list_push(list, &vals[i]->base, s2_setter_kept);
        }

        s2list_sort(list, (s2func_sort_cmp_t)s2val_cmp);

        if( l > 0 )
        {
            iter = s2obj_iter_create(&list->base);
            iter->next(iter);
            s = ((s2val_t *)iter->value)->v;
        }
        
        for(i=0; i<l; i++)
        {
            if( s > ((s2val_t *)iter->value)->v )
                exit(EXIT_FAILURE);

            if( iter->next(iter) > 0 )
                s = ((s2val_t *)iter->value)->v;
        }

        s2obj_release(&list->base);
    }

    return EXIT_SUCCESS;
}
