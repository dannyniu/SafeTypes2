/* DannyNiu/NJF, 2024-08-15. Public Domain. */

#define safetypes2_implementing_ref

#include "s2ref.h"

// 2025-02-17:
// Several bugs are fixed:
// 1. forgetting to set the `ptr` member to the
//    value of its corresponding argument.
// 2. forgetting to expose the `ptr` member to the finalizer.
//
// Changed to publically exposed public complete structure type
// as part of 2025-02-17 overhaul.

T *s2ref_create(void *ptr, void (*finalizer)(T *ptr))
{
    T *ref;

    ref = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_REF, sizeof(T));
    if( !ref ) return NULL;

    ref->ptr = ptr;
    ref->base.itercreatf = NULL;
    ref->base.finalf = (s2func_final_t)finalizer;

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
