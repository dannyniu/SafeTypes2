/* DannyNiu/NJF, 2024-08-15. Public Domain. */

#ifndef SafeTypes2_Ref_H
#define SafeTypes2_Ref_H

/// @file
/// @brief
/// The 'reference' type (container of a single externally-managed pointer).
///
/// @details
/// The purpose of this type is 2-fold:
/// 1. it allows arbitrary user resource from the application or
///    3rd-party libraries to be managed under SafeTypes2.
/// 2. it enables the program to (either entirely, or in compartments)
///    opt out of tracing garbage collection and use weakrefs instead.

#include "s2obj.h"

/// @fn
/// @param obj the object handle the type of which is being checked.
/// @returns true if the object is a 'reference', false otherwise.
#define s2_is_ref(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_REF)

/// @typedef s2ref_t
/// @brief the working context for the 'reference' type `s2ref_t`.
/// In the following prose, `s2ref_t` will be abbreviated as `T`.
#define T struct s2ctx_ref
typedef T s2ref_t;

T {
    s2obj_base;
    void *ptr;
};

/// @fn
/// @brief Creates and returns an reference object.
/// @param ptr the pointer to wrap in the reference object.
/// @param finalizer the finalizer to be called when releasing this object.
/// @returns a pointer to the list object handle, or NULL on error.
T *s2ref_create(void *ptr, void (*finalizer)(T *ptr));

/// @fn
/// @brief Creates a weak reference. Equivalent to `s2ref_create(ptr, NULL)`.
T *s2ref_create_weakref(void *ptr);

/// @fn
/// @brief retrieve the underlying pointer.
/// @returns the underlying pointer.
void *s2ref_unwrap(T *ref);

#ifndef safetypes2_implementing_ref
#undef T
#endif /* safetypes2_implementing_ref */

#endif /* SafeTypes2_Ref_H */
