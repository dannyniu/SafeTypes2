/* DannyNiu/NJF, 2024-08-15. Public Domain. */

#ifndef SafeTypes2_Ref_H
#define SafeTypes2_Ref_H

#include "common.h"

#define s2_is_ref(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_REF)

#define T struct s2ctx_ref
typedef T s2ref_t;

#ifndef safetypes2_implementing_ref
T {
    s2obj_t base;
    int payload_context[];
};
#endif /* safetypes2_implementing_ref */

T *s2ref_create(void *ptr, void (*finalizer)(void *ptr));
T *s2ref_create_weakref(void *ptr);

void *s2ref_unwrap(T *ref);

#ifndef safetypes2_implementing_ref
#undef T
#endif /* safetypes2_implementing_ref */

#endif /* SafeTypes2_Ref_H */
