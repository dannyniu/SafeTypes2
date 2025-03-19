/* DannyNiu/NJF, 2024-02-27. Public Domain. */

#ifndef SafeTypes2_H
#define SafeTypes2_H 1

/// @file
/// @brief The main include file for users of the library.

#include "common.h"
#include "siphash.h"
#include "s2obj.h"
#include "s2data.h"
#include "s2dict.h"
#include "s2list.h"
#include "s2ref.h"

/// @page gen General Overview
///
/// General Usage
/// =============
///
/// This section describe features that're common to more than one data type.
///
/// Creating Objects
/// ----------------
///
/// To create objects, declare objects of pointers to relevant types,
/// then assign the result of `s2{data,dict,list}_create()` to them.
///
///     s2{type-name}_t *x = s2{type-name}_create(...necessary params...);
///
///     ...  using the object ...
///
///     s2obj_release(x);
///
/// Operating on Reference Count
/// ----------------------------
///
/// In general, users should be using the `s2obj_retain(obj)` and
/// the `s2obj_release(obj)` functions to operate on objects in lexical
/// variables. The `s2obj_keep(obj)` and `s2obj_leave(obj)` are reserved
/// internally for implementing container types - they should not be used
/// unless you're implementing container.
///
/// Enumerating Container Members
/// -----------------------------
///
/// To enumerate members of a collection, iterators are needed. They are not
/// "objects" in the sense that their lifetime (and resource) are not managed
/// by the reference counting and garbage collection mechanism.
///
/// After creating the iterator, a call to the stepping function is needed
/// to set the position of the iterator to the initial element.
///
///     s2iter_t *it = s2obj_iter_create(x);
///     s2obj_t *val, *key;
///     int i;
///     for(i=it->next(it); i>0; i=it->next(it))
///     {
///         key = it->key;
///         val = it->value;
///         .... process 1 member ...
///     }
///
/// Accessing Container
/// -------------------
///
/// The meanings of the 2 setter semantics are explained in
/// the "README.md" file, and in the source code header comments.
///
///     enum s2_setter_semantics {
///         s2_setter_kept = 1, // `++ keptcnt`.
///         s2_setter_gave = 2, // `-- refcnt, ++keptcnt`.
///     };
///
/// There are 3 access function return values:
///
///     enum s2_access_retvals {
///         s2_access_error = -1,
///         s2_access_nullval = 0,
///         s2_access_success = 1,
///     };
///
/// Success returns are `s2_access_success` and `s2_access_nullval`.
///
/// - `s2_access_nullval` is returned by getter functions when a non-existent
///   member is requested,
///
/// - `s2_access_success` is returned by getter and setter functions
///   on success.
/// - `s2_access_error` is only returned when **internal** errors occur,
///   which are distinct from what normally would occur with container
///   access and are considered exceptional.
///
/// **2024-10-09 Update**
///
/// To support type safety, each container getter function gets
/// an "_T"-suffixed counterpart.
///
/// For example, previously, to get a member from a dict, one has to spell out
/// the cast verbosely: `s2dict_get(dict, key, (s2obj_t **)&out)`. One major
/// downside of this is that, if the type of `out` changes, type errors
/// won't be caught.
///
/// The new way to get an object from a dict is:
/// `s2dict_get_T(s2list_t)(dict, key, &out)`. An advantage of this approach
/// is that the type of `out` is clear, and it allows for consistency check.
///
/// This new change is implemented as macros in terms of older interfaces, and
/// the existing code don't need to haste to migrate as both methods are valid
/// and intended for general public use.
///
/// About Thread Safety and Garbage Collection
/// ------------------------------------------
///
/// The garbage collection mechanism is threading aware - when GC is requested,
/// it obtains relevant locks to protect the GC operation from interference
/// from other threads. When threads are operating on objects, each thread
/// should obtain the (shared a.k.a. "reader") lock to prevent interference
/// from the GC.
///
/// Sharing the "reader" lock allows different parts of the program to proceed
/// in parallel without blocking each other. Clusters of objects used by
/// various threads needs to be protected through explicit synchronization,
/// using e.g. the "cluster" mutex mentioned in "README.md" in the repo root.
///
/// One of the features of the locks ("reader" locks and GC locks) is that
/// they're recursion-adaptive - they can be locked recursively by different
/// parts of the program, and still maintain functional correctness; even if
/// some threads are holding the "reader" lock, GC is still possible
/// as long as all threads that's holding the "reader" lock enters
/// GC subroutine eventually simultaneously.
///
/// To obtain and release threads lock, call `s2gc_thrd_lock()` and
/// `s2gc_thrd_unlock()` respectively; to enter GC, call `s2gc_collect()`.
///
/// Custom Types
/// ============
///
/// It is possible to define custom data types that can integrate into the
/// SafeTypes2 eco-system. The following code listing demonstrate how to implement
/// a "pair" type using SafeTypes2 facility.
///
/// **"pair.h":**
///
///     #ifndef mylibrary_pair_h
///     #define mylibrary_pair_h 1
///
///     #include <SafeTypes2/s2data.h>
///
///     #define TYPE_PAIR 0x2001
///     #define type_is_pair(obj) (obj->type == TYPE_PAIR)
///
///     #define T struct typectx_pair
///     typedef T pair_t;
///
///     T {
///         s2obj_base;
///         s2data_t *left, *right;
///     };
///
///     // ''refcnt'' and ''keptcnt'' are not incremented.
///     int pair_get_left (T *pair, s2data_t **out);
///     int pair_set_right(T *pair, s2data_t **out);
///
///     int pair_set_left (T *pair, s2data_t *in, int semantic);
///     int pair_set_right(T *pair, s2data_t *in, int semantic);
///
///     #ifndef mylibrary_implementing_pair
///     #undef T
///     #endif /* mylibrary_implementing_pair */
///
///     #endif /* mylibrary_pair_h */
///
/// **"pair.c":**
///
///     #define mylibrary_implementing_pair
///     #define safetypes2_implementing_misc
///     #include "pair.h"
///
///     static void pair_final(T *pair)
///     {
///         // [note-refcnt-retained]
///         // Because `s2data_t` is not a container type,
///         // we don't have to concern with reference loop.
///         if( pair->left  ) s2obj_release((s2obj_t *)pair->left);
///         if( pair->right ) s2obj_release((s2obj_t *)pair->right);
///     }
///
///     T *pair_create()
///     {
///         T *ret = NULL;
///
///         ret = (T *)s2gc_obj_alloc(TYPE_PAIR, sizeof(T));
///         if( !ret ) return NULL;
///
///         ret->left = NULL;
///         ret->right = NULL;
///
///         ret->basetype.itercreatf = NULL;
///         ret->basetype.finalf = (s2func_final_t)pair_final;
///         return ret;
///     }
///
///     int pair_get_left(T *pair, s2data_t **out);
///     {
///         *out = pair->left;
///         return *out ? s2_access_success : s2_access_nullval;
///     }
///
///     int pair_get_right(T *pair, s2data_t **out);
///     {
///         *out = pair->right;
///         return *out ? s2_access_success : s2_access_nullval;
///     }
///
///     int pair_set_left(T *pair, s2data_t *in, int semantic)
///     {
///         // See note tagged [note-refcnt-retained].
///         if( pair->left ) s2obj_release((s2obj_t *)pair->left);
///         pair->left = in;
///         if( semantic == s2_setter_kept )
///             s2obj_retain((s2obj_t *)pair->left);
///     }
///
///     int pair_set_right(T *pair, s2data_t *in, int semantic)
///     {
///         // See note tagged [note-refcnt-retained].
///         if( pair->right ) s2obj_release((s2obj_t *)pair->right);
///         pair->right = in;
///         if( semantic == s2_setter_kept )
///             s2obj_retain((s2obj_t *)pair->right);
///     }
///
/// @important
/// @parblock
/// If your type contain component(s) that're container types, or
/// type that refer to objects of arbitrary type, then
/// your type need to implement an iterator that enumerates at least
/// these components - this is needed for GC to function correctly.
///
/// For example, if your type contain a component that is a dict or list,
/// then you need to implement the iterator; if however there's only data,
/// then it's not needed, as `s2data_t` isn't a container type
/// and doesn't reference other objects.
/// @endparblock

#endif /* SafeTypes2_H */
