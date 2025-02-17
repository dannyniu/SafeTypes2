/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#ifndef SafeTypes2_obj_ctx_h
#define SafeTypes2_obj_ctx_h 1

/// @file
/// @brief
/// The object base type and auxiliary definitions for all of SafeTypes2.

#include "common.h"

/// @page objsys Object System
/// @section type-hier Type Hierarchy
/// The "object" type (denoted by `s2obj_t`) is the base type for all
/// externally-exposed types except a few (e.g. `s2iter_t`). It defines
/// common interface for derived types.
///
/// Before 2025-02-17, all derived types defined by SafeTypes2 were
/// incomplete (C language concept), as they're meant to be used from
/// pointer handles. This is still the case - although you technically
/// can declare a SafeTypes2 object either locally or statically, their
/// interaction with the garbage collection, and even with regard to
/// reference and kept counts when you're opting out of GC, are
/// completely undefined.
///
/// The reason now they're being defined to be complete, is that the way
/// they were defined were not compatible with C89 - a compatibility goal
/// which @dannyniu would like to achieve to maximize the audience of the
/// library; also, this make it easier for 3rd-party libraries and applications
/// to create further derived types.
///
/// The internal data structures have tag names prefixed with "s2ctx_",
/// whereas externally visible ones are defined as types and
/// not prefixed as such.

/// @typedef s2obj_t
/// @brief the working context for the base object type `s2obj_t`.
/// In the following prose, `s2obj_t` will be abbreviated as `T`.
#define T struct s2ctx_obj
typedef T s2obj_t;

/// @typedef s2iter_t
/// @brief
/// The base type of various iterators.
typedef struct s2ctx_iter s2iter_t;

/// @typedef s2iter_stepfunc_t
/// @brief The iterator stepping function.
/// @param ctx the pointer handle to the iterator.
/// @returns 1 on success, 0 at the end, and -1 on error.
///
/// @details
/// Iterator creator functions doen't set the position, a initial call
/// to step function is needed to initialize the position of the iterator.
typedef int (*s2iter_stepfunc_t)(s2iter_t *restrict ctx);

/// @typedef s2iter_final_func_t
/// @brief frees up resources used by the iterator.
/// @param ctx the pointer handle to the iterator
typedef void (*s2iter_final_func_t)(s2iter_t *restrict ctx);

/// @struct s2ctx_iter
/// @brief
/// The `struct s2ctx_iter` defines publically visible members
/// of the iterator base type structure.
struct s2ctx_iter
{
    /// @brief slot for the finalizer subroutine.
    s2iter_final_func_t final; 

    // Iterators that're container enumerators live in a time range
    // that're sub-ranges of their respective containers. This is
    // usually ensured by having the iterators operate in the same
    // thread as their respective containers, and protected by the
    // same "cluster" mutex.
    /// @brief the iterator stepping function.
    s2iter_stepfunc_t next;

    // 2024-07-27:
    // The following 2 was mistakenly restrict-qualified. Now corrected.
    //
    /// @brief the key for the current value.
    /// @details
    /// This is `intptr_t` for `s2list_t` and `s2data_t *` for `s2dict_t`.
    /// When the key is an object, it's an unretained reference to the
    /// internal structure member, so it shouldn't be modified or
    /// have its reference or kept count changed.
    void *key;

    /// @brief
    /// the slot holding the value object at the current iterator position.
    T *value;
};

/// @typedef s2func_iter_create_t
/// @returns non-NULL on success and NULL on failure.
///
/// @note
/// This is a change from the original SafeTypes library - instead of keeping
/// iterator state on the object, separate iterator context is used
/// so that 1 (read-only) object may have different iterator positions.
typedef s2iter_t *(*s2func_iter_create_t)(T *restrict ctx);

/// @typedef s2func_final_t
/// @brief subroutine type for the finalizer slot.
/// @param ctx the handle pointer to the object to finalize.
///
/// @details
/// When both the reference and the kept count of an object is zero, its
/// finalization routine is invoked and object is freed.
typedef void (*s2func_final_t)(T *restrict ctx);

/// @typedef s2obj_typeid_t
typedef uint16_t s2obj_typeid_t;

/// @page objsys Object System
/// @section type-ids Type Identifier Name Space.
/// Rules of composition:
/// - 0x3000: container/plain/opaque type class flag mask.
/// - 0x0xxx: plain type.
/// - 0x10xx: container type.
/// - 0x2xxx: opaque (application-defined) type.
/// @note 2024-11-05: type class had been re-specified as prefix-free codes.
///
/// - if <plain type>:
///   - 0x0f00: plain type class mask.
///   - 0x0000: string/blob/misc.
///   - 0x0100: integer type.
///   - 0x0200: floating point type.
///   - 0x0300: array type.
///
/// - if <integer type> or <floating point type>:
///   - 0x00c0: endianness mask - not applicable to strings and blobs.
///   - 0x0000: host endianness.
///   - 0x0040: little-endian.
///   - 0x0080: big-endian.
///   - 0x003f: plain type width mask.
///
/// - if <array type>:
///   - 0x00ff: array element length mask.
///
/// - pre-defined values:
///   - 0x0000: exact value for the null type.
///   - 0x0001: exact value for the raw data type.
///   - 0x0002: exact value for utf-8 string type.
///   - 0x0003: exact value for 8-bit data type.
#define S2_OBJ_TYPE_NULL        0x0000

// No linter enforces the correctness of coding, they're only different in
// serialization to JSON:
// - *_BLOB is base-64 encoded,
// - *_UTF8 is converted to UTF-16, then with
//   Unicode characters converted to code point literal.
// - *_8BIT is encoded with non-printable characters
//   encoded in hex char literal.
#define S2_OBJ_TYPE_BLOB        0x0001
#define S2_OBJ_TYPE_UTF8        0x0002
#define S2_OBJ_TYPE_8BIT        0x0003

// width is a power of 2 between 1 and 8.
#define S2_OBJ_TYPE_INT(width)  (0x0100 | (width))

// width can be a power of 2 between 2 and 16.
// values other than 4 and 8 may not necessarily be supported.
#define S2_OBJ_TYPE_FLT(width)  (0x0200 | (width))

#define S2_OBJ_TYPE_DICT        0x1001
#define S2_OBJ_TYPE_LIST        0x1002
#define S2_OBJ_TYPE_REF         0x1003

T {
    // used by GC.
    T *gc_prev;
    T *gc_next;

    // type identification.
    // types chosen for structure member alignment.
    s2obj_typeid_t type;

    // used by GC.
    // types chosen for structure member alignment.

    // Added 2024-02-26.
    // 1: finalized by GC, do not traverse this object.
    // 0: normal state.
    uint16_t guard;

    // mark the objects reachable from some lexical variables (i.e. refcnt>0).
    uint32_t mark;
    
    // referenced by a lexical variable.
    uint32_t refcnt;
    
    // referenced by a container object.
    uint32_t keptcnt;

    s2func_iter_create_t itercreatf;
    s2func_final_t finalf;

    // 2024-08-01:
    // There was an unsized array of int at the end to ensure s2obj_t is
    // incomplete and cannot be declared as lexical variable unless it's
    // a pointer. However, considering that it's not useful to have an
    // object header without any content and value, this restriction is
    // removed. Subsequently, concrete types defined/declared elsewhere
    // will be able to include an `s2obj_t base;` field at the head, allowing
    // type-safe passing of derived types to variables and parameters
    // expecting generic base objects.

    // 2025-02-15:
    // A build option "SAFETYPES_BUILD_WITHOUT_GC" is introduced to build
    // a simplified variant of the library without garbage collection.
    // Although many fields of this structure will be ignored in such variant,
    // they're nonetheless retained in doubt that there may be break in ABI.
};

/// @def s2obj_base
/// @brief Base header for objects.
/// @details
/// This is the base object header to be placed at the beginning of all
/// types derived from `s2obj_t`.
/// - The `pobj` field will point to the beginning address of the object,
///   which make it easy to refer to the object itself as an `s2obj_t`.
/// - The `ctxinfo` can hold contextual information for arbitrary purpose.
#define s2obj_base struct { s2obj_t base; s2obj_t *pobj; intptr_t ctxinfo; }

/// @page objsys Object System
/// @section mem-gc Resource Management and Garbage Collection.
/// The SafeTypes2 GC is threading-aware with the help of a
/// **reader-writer lock**. In multi-threaded applications, threads obtain
/// "reader" locks to prevent GC from operating on data structures that
/// the threads may be using. When GC operates, the "writer" lock is locked,
/// and all threads are stalled and prevented from operating on objects
/// so that GC can safely traverse the graph of reachable objects,
/// marking them and releasing unreachable objects as needed.
///
/// The "reader" lock is recursive - threads can invoke codes from 3rd-party
/// libraries and not worry they interfere with the calling code;
/// The "reader" lock is also "rewindable" - if while a "reader" lock is held
/// by the calling thread, an acquire of the "writer" lock (implicitly invoked
/// as part of `s2gc_collect`) automatically clears up whilest remembering
/// the "reader" lock.
///
/// As such, when all involved threads requests "writer" lock,
/// the GC would eventually operate, regardless of
/// - which threads first requested GC,
/// - how many threads are holding the "reader" lock,
/// - how many level of recursion the "reader" locks are in,
///
/// As long as all threads that had held any "reader" lock
/// eventually calls `s2gc_collect`.
///
/// This mechanism applies to the GC, and not the rest of the application -
/// threads still need to prevent threading conflict using explicit
/// synchronization.


// 2024-02-25:
// This could've been called `s2obj_create`, however
// the existing naming signifies it's a internal interface.
//
/// @fn
/// @brief Allocates memory for an object. Invoked by type implementations.
/// @param type the type id for the object, see @ref type-ids
/// @returns the pointer handle to the object.
T *s2gc_obj_alloc(s2obj_typeid_t type, size_t sz);

/// @fn
/// @brief Deallocates memory for the object. Invoked by type implementations.
/// @param obj the object to deallocate.
/// @details additional resources are released by finalizer.
void s2gc_obj_dealloc(T *restrict obj);

/// @fn
/// @brief enable or disable threading support in garbage collector.
/// @param enabled `true` to enable threading, and `false` to disable it.
///
/// @details
/// 2024-03-09: It is assumed that this will only be called when
/// there's only 1 thread. By default, threading is enabled for GC,
/// and this function is provided for single-threaded applications
/// to avoid synchronization overheads.
void s2gc_set_threading(bool enabled);

/// @fn
/// @brief
/// Explicitly invoke SafeTypes2 garbage collection.
/// Applications that never creates reference cycles don't need this.
/// @note
/// 2025-02-15: This function is not available in a build without GC.
void s2gc_collect(void);

/// @fn
/// @brief Increases reference count. Invoked by the application.
/// @returns obj
T *s2obj_retain(T *restrict obj);

/// @fn
/// @brief Increases 'kept' count. Invoked by container implementations.
/// @returns obj
/// @note
/// 2025-02-15:
/// This function is identical to `s2obj_retain` in builds without GC.
T *s2obj_keep(T *restrict obj);

/// @fn
/// @brief Decreases reference count. Invoked by the application.
void s2obj_release(T *restrict obj);

/// @fn
/// @brief Decreases 'kept' count. Invoked by container implementations.
/// @note
/// 2025-02-15:
/// This function is identical to `s2obj_release` in builds without GC.
void s2obj_leave(T *restrict obj);

/// @fn
/// @brief creates an iterator by invoking the internal `itercreatf`.
/// @param obj the object of which the iterator is created.
/// @return the iterator object, or NULL on failure.
s2iter_t *s2obj_iter_create(T *restrict obj);

/// @fn
/// @brief
/// To obtain a "reader" lock for the application thread.
/// ("thrd" is a fortunate portmanteau of "thread-reading".)
/// @note
/// 2025-02-15: This function is not available in a build without GC.
int s2gc_thrd_lock();

/// @fn
/// @brief To release the "reader" lock from a application thread.
/// @note
/// 2025-02-15: This function is not available in a build without GC.
int s2gc_thrd_unlock();

#ifndef safetypes2_implementing_obj
#undef T
#endif /* safetypes2_implementing_obj */

#endif /* SafeTypes2_obj_ctx_h */
