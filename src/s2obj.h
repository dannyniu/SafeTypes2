/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#ifndef SafeTypes2_obj_ctx_h
#define SafeTypes2_obj_ctx_h 1

#include "common.h"

// The "object" type (denoted by ``s2obj_t'') is the base type for all
// externally-exposed types except a few (e.g. ``s2iter_t''). It defines
// common interface for derived types. All derived types defined by SafeTypes2
// are incomplete (C language concept), and are meant to be used from
// pointer handles.
//
// The internal data structures have tag names prefixed with "s2ctx_", whereas
// externally visible ones are defined as types and not prefixed as such.
#define T struct s2ctx_obj
typedef T s2obj_t;

// The incomplete base type of various iterators.
typedef struct s2ctx_iter s2iter_t;

// The iterator stepping function. Returns:
// * 1 on success,
// * 0 at the end,
// * -1 on error.
//
// Iterator creator functions doen't set the position, a initial call
// to step function is needed to initialize the position of the iterator.
typedef int (*s2iter_stepfunc_t)(s2iter_t *restrict ctx);

// frees up resources used by the iterator.
typedef void (*s2iter_final_func_t)(s2iter_t *restrict ctx);

// Publically visible members of the iterator base type structure.
struct s2ctx_iter
{
    s2iter_final_func_t final;

    // Iterators that're container enumerators live in a time range
    // that're sub-ranges of their respective containers. This is
    // usually ensured by having the iterators operate in the same
    // thread as their respective containers, and protected by the
    // same "cluster" mutex.
    s2iter_stepfunc_t next;

    // For ``s2list_t'', key is interpreted as ``intptr_t key'',
    // for ``s2dict_t'', key is interpreted as ``s2data_t *key''.
    // If key is an object, it's an unretained reference to the
    // internal structure member, so it shouldn't be modified or
    // have its reference or kept count changed.
    //
    // 2024-07-27:
    // The following 2 was mistakenly restrict-qualified. Now corrected.
    //
    void *key;
    T *value;
};

// Returns non-NULL on success and NULL on failure.
// This is a change from the original version - instead of keeping
// iterator state on the object, separate iterator context is used
// so that 1 (read-only) object may have different iterator positions.
typedef s2iter_t *(*s2func_iter_create_t)(T *restrict ctx);

// When both the reference and the kept count of an object is zero, its
// finalization routine is invoked and object is freed.
typedef void (*s2func_final_t)(T *restrict ctx);

// Rules of composition:
// - 0x3000: container/plain/opaque type class flag mask.
// - 0x0xxx: plain type.
// - 0x10xx: container type.
// - 0x2xxx: opaque (application-defined) type.
// # 2024-11-05: type class had been re-specified as prefix-free codes.
// ----
// ---- if <plain type>:
// - 0x0f00: plain type class mask.
// - 0x0000: string/blob/misc.
// - 0x0100: integer type.
// - 0x0200: floating point type.
// - 0x0300: array type.
// ----
// ---- if <integer type> or <floating point type>:
// - 0x00c0: endianness mask - not applicable to strings and blobs.
// - 0x0000: host endianness.
// - 0x0040: little-endian.
// - 0x0080: big-endian.
// - 0x003f: plain type width mask.
// ----
// ---- if <array type>:
// - 0x00ff: array element length mask.
// ----
// ---- pre-defined values:
// - 0x0000: exact value for the null type.
// - 0x0001: exact value for the raw data type.
// - 0x0002: exact value for utf-8 string type.
// - 0x0003: exact value for 8-bit data type.
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

typedef uint16_t s2obj_typeid_t;

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
    size_t refcnt;
    
    // referenced by a container object.
    size_t keptcnt;

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
};

// 2024-02-25:
// This could've been called ``s2obj_create'', however
// the existing naming signifies it's a internal interface.
T *s2gc_obj_alloc(s2obj_typeid_t type, size_t sz);
void s2gc_obj_dealloc(T *restrict obj);

// 2024-03-09:
// It is assumed that this will only be called when
// there's only 1 thread. By default, threading is
// enabled for GC, and this function is provided for
// single-threaded applications to avoid costs associated
// with synchronization.
void s2gc_set_threading(bool enabled);

// Explicit invocation of SafeTypes2 garbage collection.
// Only needed if there's "reference cycle".
void s2gc_collect(void);

// Returns ``obj''.
T *s2obj_retain(T *restrict obj); // ``++refcnt''.
T *s2obj_keep(T *restrict obj); // ``++keptcnt''.

void s2obj_release(T *restrict obj); // ``--refcnt''.
void s2obj_leave(T *restrict obj); // ``--keptcnt''.

// creates an iterator by invoking the internal ``itercreatf''.
s2iter_t *s2obj_iter_create(T *restrict obj);

// To obtain a "reader" lock for the application thread.
// "thrd" is a fortunate portmanteau of "thread-reading".
int s2gc_thrd_lock();

// To release the "reader" lock from a application thread.
int s2gc_thrd_unlock();

#ifndef safetypes2_implementing_obj
#undef T
#endif /* safetypes2_implementing_obj */

#endif /* SafeTypes2_obj_ctx_h */
