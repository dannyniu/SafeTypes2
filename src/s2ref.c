/* DannyNiu/NJF, 2024-08-15. Public Domain. */

#define safetypes2_implementing_ref

#include "s2ref.h"

static void s2ref_final(T *ref)
{
    if( ref->finalizer )
        ref->finalizer(ref->ptr);
}

T *s2ref_create(void *ptr, s2ref_final_func_t finalizer)
{
    T *ref;

    ref = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_REF, sizeof(T));
    if( !ref ) return NULL;

    ref->ptr = ptr;
    ref->finalizer = finalizer;
    ref->base.itercreatf = NULL;
    ref->base.finalf = (s2func_final_t)s2ref_final;

    return ref;
}

T *s2ref_create_weakref(void *ptr)
{
    return s2ref_create(ptr, NULL);
}

void *s2ref_unwrap(T *ref)
{
    return ref->ptr;
}
