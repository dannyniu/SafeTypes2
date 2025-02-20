/* DannyNiu/NJF, 2024-02-26. Public Domain. */

#define safetypes2_implementing_list
#include "s2list.h"

void s2list_final(T *list)
{
    struct s2ctx_list_element *M, *L;

    for(M=list->anch_head.next; M!=&list->anch_tail; )
    {
        s2obj_leave(M->value);
        L = M;
        M = M->next;
        free(L);
    }
}

struct s2ctx_list_iter {
    struct s2ctx_iter base;
    struct s2ctx_list_element *cursor;
    s2list_t *list;
};

static int s2list_iter_step(s2list_iter_t *restrict iter)
{
    if( iter->cursor != &iter->list->anch_tail )
    {
        if( iter->base.value )
            iter->base.key = (void *)((intptr_t)iter->base.key + 1);
        iter->base.value = iter->cursor->value;
        iter->cursor = iter->cursor->next;
        return 1;
    }
    else
    {
        iter->base.key = (void *)((intptr_t)iter->base.key + 1);
        iter->base.value = NULL;
        return 0;
    }
}

static s2list_iter_t *s2list_iter_create(T *list)
{
    struct s2ctx_list_iter *ret;

    ret = calloc(1, sizeof(struct s2ctx_list_iter));
    if( !ret ) return NULL;

    ret->base.final = (s2iter_final_func_t)free;
    ret->base.next = (s2iter_stepfunc_t)s2list_iter_step;
    ret->base.key = (void *)0;
    ret->base.value = NULL;

    ret->cursor = list->anch_head.next;
    ret->list = list;

    return ret;
}

T *s2list_create()
{
    T *list;

    list = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_LIST, sizeof(T));
    if( !list ) return NULL;

    list->base.itercreatf = (s2func_iter_create_t)s2list_iter_create;
    list->base.finalf = (s2func_final_t)s2list_final;

    list->len = 0;
    list->pos = 0;
    list->cursor = &list->anch_tail;

    list->anch_head.collection = list;
    list->anch_head.value = NULL;
    list->anch_head.prev = NULL;
    list->anch_head.next = &list->anch_tail;

    list->anch_tail.collection = list;
    list->anch_tail.value = NULL;
    list->anch_tail.prev = &list->anch_head;
    list->anch_tail.next = NULL;

    return list;
}

#define S2_LIST_SETTER_ASSERTIONS                                       \
    assert( (ptrdiff_t)list->pos >= 0 && list->pos <= list->len);       \
    assert( semantic == s2_setter_kept ||                               \
            semantic == s2_setter_gave );

#define S2_LIST_GETTER_ASSERTIONS                                       \
    assert( (ptrdiff_t)list->pos >= 0 && list->pos <= list->len);

int s2list_insert(T *list, s2obj_t *obj, int semantic)
{
    struct s2ctx_list_element *M;

    S2_LIST_SETTER_ASSERTIONS;

    if( !(M = calloc(1, sizeof(struct s2ctx_list_element))) )
        return s2_access_error;

    M->collection = list;
    M->value = obj;

    ++ list->len;

    M->next = list->cursor;
    M->prev = list->cursor->prev;
    list->cursor->prev->next = M;
    list->cursor->prev = M;
    list->cursor = M;

    switch( semantic ){
    case s2_setter_kept:
        s2obj_keep(obj);
        break;

    case s2_setter_gave:
        s2obj_keep(obj);
        s2obj_release(obj);
        break;
    }

    return s2_access_success;
}

int s2list_push(T *list, s2obj_t *obj, int semantic)
{
    struct s2ctx_list_element *M;

    S2_LIST_SETTER_ASSERTIONS;

    if( !(M = calloc(1, sizeof(struct s2ctx_list_element))) )
        return s2_access_error;

    M->collection = list;
    M->value = obj;

    ++ list->len;
    ++ list->pos;

    M->next = list->cursor;
    M->prev = list->cursor->prev;
    list->cursor->prev->next = M;
    list->cursor->prev = M;

    switch( semantic ){
    case s2_setter_kept:
        s2obj_keep(obj);
        break;

    case s2_setter_gave:
        s2obj_keep(obj);
        s2obj_release(obj);
        break;
    }

    return s2_access_success;
}

int s2list_shift(T *list, s2obj_t **out)
{
    struct s2ctx_list_element *M;

    S2_LIST_GETTER_ASSERTIONS;

    if( list->len <= 0 || list->pos >= list->len )
    {
        *out = NULL;
        return s2_access_nullval;
    }

    M = list->cursor;
    *out = M->value;

    // 2024-12-27:
    // In a revision of the original SafeTypes that had not been covered under
    // version control, there was a note tagged [2023-07-17-cnt-op] that was
    // refuted by a note dated 2023-07-27. This version was never published,
    // and the 2023-07-27 note applied to the older GC in the original
    // SafeTypes library - it is irrelevant in SafeTypes2.
    ++ (*out)->refcnt;
    -- (*out)->keptcnt;

    -- list->len;

    list->cursor->prev->next = list->cursor->next;
    list->cursor->next->prev = list->cursor->prev;
    list->cursor = list->cursor->next;

    free(M);
    return s2_access_success;
}

int s2list_pop(T *list, s2obj_t **out)
{
    S2_LIST_GETTER_ASSERTIONS;

    if( list->len <= 0 || list->pos <= 0 || list->len < list->pos )
    {
        *out = NULL;
        return s2_access_nullval;
    }

    list->pos --;
    list->cursor = list->cursor->prev;

    return s2list_shift(list, out);
}

int s2list_get(T *list, s2obj_t **out)
{
    S2_LIST_GETTER_ASSERTIONS;

    if( list->pos >= list->len )
    {
        *out = NULL;
        return s2_access_nullval;
    }

    *out = list->cursor->value;
    return s2_access_success;
}

int s2list_put(T *list, s2obj_t *obj, int semantic)
{
    S2_LIST_SETTER_ASSERTIONS;

    if( list->pos >= list->len )
    {
        return s2_access_error;
    }

    switch( semantic ){
    case s2_setter_kept:
        s2obj_keep(obj);
        break;

    case s2_setter_gave:
        s2obj_keep(obj);
        s2obj_release(obj);
        break;
    }

    if( list->cursor->value ) s2obj_leave(list->cursor->value);

    list->cursor->value = obj;
    return s2_access_success;
}

ptrdiff_t s2list_pos(T *list){ return list->pos; }
ptrdiff_t s2list_len(T *list){ return list->len; }

ptrdiff_t s2list_seek(T *list, ptrdiff_t offset, int whence)
{
    switch( whence )
    {
    case S2_LIST_SEEK_SET:
        if( offset < 0 || (size_t)offset > list->len )
        {
            return -1;
        }
        else
        {
            list->pos = offset;
            list->cursor = list->anch_head.next;

            while( offset > 0 )
            {
                list->cursor = list->cursor->next;
                offset --;
            }

            return list->pos;
        }
        break;

    case S2_LIST_SEEK_END:
        if( offset + (ptrdiff_t)list->len < 0 || offset > 0 )
        {
            return -1;
        }
        else
        {
            list->pos = list->len + offset;
            list->cursor = &list->anch_tail;

            while( offset < 0 )
            {
                list->cursor = list->cursor->prev;
                offset ++;
            }

            return list->pos;
        }
        break;

    case S2_LIST_SEEK_CUR:
        if( offset + (ptrdiff_t)list->pos < 0 ||
            offset + (ptrdiff_t)list->pos > (ptrdiff_t)list->len )
        {
            return -1;
        }
        else
        {
            list->pos += offset;

            while( offset > 0 )
            {
                list->cursor = list->cursor->next;
                offset --;
            }

            while( offset < 0 )
            {
                list->cursor = list->cursor->prev;
                offset ++;
            }

            return list->pos;
        }
        break;

    default:
        return -1;
    }
}

T *s2list_sort(T *list, s2func_sort_cmp_t cmpfunc)
{
    /* A conjecture credited to DannyNiu/NJF (that's me, hi) says:
     * for a given class of problem, the asymptotic time-space
     * overall complexity of the most optimal algorithms that
     * solve the problem cannot be optimized beyond a certain
     * limit. In plain words, if you optimize the time complexity
     * of an algorithm to solve a specific problem, the space
     * complexity will grow up by as much as is optimized away in
     * the time dimension.
     *
     * This is akin to data compression - where entropy of the
     * data source dictates the minimum size of the compressed
     * file that represents the original data; and akin to
     * "Kolmogorov Complexity" - a similar concept where the
     * subject is a data-producing "functional" description.
     *
     * Therefore, after comparing the time/space complexity of
     * popular sorting algorithms, and noting that all of them
     * have worst-case complexity of at least O(n^2), a dumb
     * algorithm with O(1) space complexity is chosen for
     * implementation for SafeTypes library.
     */

    struct s2ctx_list_element *o, *t;

    if( list->len <= 1 ) return list;

    list->pos = 0;
    list->cursor = list->anch_head.next;
    o = t = NULL;
    list->anch_head.next->prev = NULL;
    list->anch_tail.prev->next = NULL;
    list->anch_head.next = &list->anch_tail;
    list->anch_tail.prev = &list->anch_head;

    while( list->cursor )
    {
        for(o=list->anch_head.next; o->next; o=o->next)
        {
            // using the `<=` operator to preserve the relative order
            // of equal elements scattered over the list.
            // !!not a guaranteed feature!!
            if( cmpfunc(o->value, list->cursor->value) <= 0 )
                continue;
            else break;
        }

        // insert cursor head just before `o`.

        t = list->cursor;
        list->cursor = list->cursor->next;

        t->next = o;
        t->prev = o->prev;
        o->prev->next = t;
        o->prev = t;
    }

    return list;
}
