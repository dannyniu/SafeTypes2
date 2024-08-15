/* DannyNiu/NJF, 2024-08-15. Public Domain. */

#define safetypes2_implementing_ref

#include "s2ref.h"

T {
    s2obj_t basetype;
    void *ptr;
};

T *s2ref_create(void *ptr, void (*finalizer)(void *ptr))
{
    T *ref;

    ref = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_REF, sizeof(T));
    if( !ref ) return NULL;

    ref->basetype.itercreatf = NULL;
    ref->basetype.finalf = (s2func_final_t)finalizer;

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
