/* DannyNiu/NJF, 2024-02-26. Public Domain. */

#include "s2list.h"
#include "s2data.h"

#include <stdio.h>

int main(void)
{
    int i, ret;
    int fails = 0;
    long *lp;
        
    s2list_t *x = s2list_create();
    s2data_t *v;
    s2iter_t *iter;

    if( s2list_seek(x, 0, S2_LIST_SEEK_SET) == -1 )
        printf("SET Seek on Empty List Failed!\n"), fails ++;

    if( s2list_seek(x, 0, S2_LIST_SEEK_END) == -1 )
        printf("END Seek on Empty List Failed!\n"), fails ++;
    
    if( s2list_seek(x, 0, S2_LIST_SEEK_CUR) == -1 )
        printf("CUR Seek on Empty List Failed!\n"), fails ++;

    for(i=4; i<=8; i+=2)
    {
        v = s2data_create(sizeof(long));
        *(long *)s2data_map(v, 0, sizeof(long)) = i;
        s2data_unmap(v);

        if( s2list_push(x, &v->base, s2_setter_gave) != s2_access_success )
            printf("List Push Failed!\n"), fails ++;
    }

    for(i=7; i>=5; i-=2)
    {
        v = s2data_create(sizeof(long));
        *(long *)s2data_map(v, 0, sizeof(long)) = i;
        s2data_unmap(v);

        if( s2list_seek(x, -1, S2_LIST_SEEK_CUR) == -1 )
            printf("CUR Seek on Live List Failed!\n"), fails ++;
        
        if( s2list_insert(x, &v->base, s2_setter_gave) != s2_access_success )
            printf("List Insert Failed!\n"), fails ++;
    }

    if( s2list_seek(x, 0, S2_LIST_SEEK_SET) == -1 )
        printf("SET Seek on Live List Failed!\n"), fails ++;

    for(i=3; i>=0; i--)
    {
        v = s2data_create(sizeof(long));
        *(long *)s2data_map(v, 0, sizeof(long)) = i;
        s2data_unmap(v);

        if( s2list_insert(x, &v->base, s2_setter_gave) != s2_access_success )
            printf("List Insert Failed!\n"), fails ++;
    }

    if( s2list_seek(x, 0, S2_LIST_SEEK_END) == -1 )
        printf("END Seek on Live List Failed!\n"), fails ++;

    for(i=9; i<=10; i++)
    {
        v = s2data_create(sizeof(long));
        *(long *)s2data_map(v, 0, sizeof(long)) = i;
        s2data_unmap(v);

        if( s2list_push(x, &v->base, s2_setter_gave) != s2_access_success )
            printf("List Push Failed!\n"), fails ++;
    }

    iter = s2obj_iter_create(&x->base);
    for(ret=iter->next(iter), i=0; ret>0; ret=iter->next(iter), i++)
    {
        v = (void *)iter->value;
        if( *(lp = (long *)s2data_map(v, 0, sizeof(long))) != i )
        {
            printf("List Enumeration Encountered Wrong Value: %ld != %d.\n",
                   *lp, i), fails ++;
        }
        s2data_unmap(v);
        if( (intptr_t)iter->key != i )
        {
            printf("List Enumeration Encountered Wrong Index: %ld != %d.\n",
                   (long)iter->key, i), fails ++;
        }
    }

    s2list_seek(x, 0, S2_LIST_SEEK_SET);
    for(i=0; i<3; i++)
    {
        if( s2list_shift(x, (void *)&v) != s2_access_success )
            printf("List Shift Failed!\n"), fails ++;

        if( *(lp = (long *)s2data_map(v, 0, sizeof(long))) != i )
        {
            printf("List Shift Encountered Wrong Value: %ld != %d.\n",
                   *lp, i), fails ++;
        }
        s2data_unmap(v);

        s2obj_release(&v->base);
    }
   
    s2list_seek(x, 0, S2_LIST_SEEK_END);
    for(i=10; i>7; i--)
    {
        if( s2list_pop(x, (void *)&v) != s2_access_success )
            printf("List Pop Failed!\n"), fails ++;

        if( *(lp = (long *)s2data_map(v, 0, sizeof(long))) != i )
        {
            printf("List Pop Encountered Wrong Value: %ld != %d.\n",
                   *lp, i), fails ++;
        }
        s2data_unmap(v);
        
        s2obj_release(&v->base);
    }

    s2list_seek(x, -1, S2_LIST_SEEK_END);
    
    if( s2list_pop(x, (void *)&v) != s2_access_success )
        printf("List Pop Failed!\n"), fails ++;
    if( *(lp = (long *)s2data_map(v, 0, sizeof(long))) != 6 )
    {
        printf("List Pop Encountered Wrong Value: %ld != %d.\n",
               *lp, i), fails ++;
    }
    s2data_unmap(v);

    if( s2list_shift(x, (void *)&v) != s2_access_success )
        printf("List Shift Failed!\n"), fails ++;
    if( *(lp = (long *)s2data_map(v, 0, sizeof(long))) != 7 )
    {
        printf("List Shift Encountered Wrong Value: %ld != %d.\n",
               *lp, i), fails ++;
    }
    s2data_unmap(v);
   
    if( fails ) return EXIT_FAILURE; else return EXIT_SUCCESS;
}
