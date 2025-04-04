**NOTE 2024-12-28**
===================

This document is obsoleted in favor of in-header Doxygen markups and
generated page files. It is retained for 1. historical significance,
and 2. any future modifications and improvements to the code remain
backwards compatible when referring to the specifications in this file.

Below is the original content of this document.

API Document
============

This document complements the comments in the source code header files in
describing the API of the SafeTypes2 library.

# 1. General Usage

This section describe features that're common to more than one data type.

## 1.1. Creating Objects

To create objects, declare objects of pointers to relevant types,
then assign the result of `s2{data,dict,list}_create()` to them.

```
s2{data,dict,list}_t *x = s2{data,dict,list}_create(...necessary params...);

...  using the object ...

s2obj_release(x);
```

## 1.2. Operating on Reference Count

In general, users should be using the `s2obj_retain(obj)` and
the `s2obj_release(obj)` functions to operate on objects in lexical variables.
The `s2obj_keep(obj)` and `s2obj_leave(obj)` are reserved internally for
implementing container types - they should not be used
unless you're implementing container.

## 1.3. Enumerating Container Members

To enumerate members of a collection, iterators are needed. They are not
"objects" in the sense that their lifetime (and resource) are not managed
by the reference counting and garbage collection mechanism.

After creating the iterator, a call to the stepping function is needed
to set the position of the iterator to the initial element.

```
s2iter_t *it = s2obj_iter_create(x);
s2obj_t *val, *key;
int i;
for(i=it->next(it); i>0; i=it->next(it))
{
    key = it->key;
    val = it->value;
    .... process 1 member ...
}
```

## 1.4. Accessing Container

The meanings of the 2 setter semantics are explained in the "README.md" file,
and in the source code header comments.

```
enum s2_setter_semantics {
    s2_setter_kept = 1, // ``++ keptcnt''.
    s2_setter_gave = 2, // ``-- refcnt, ++keptcnt''.
};
```

There are 3 access function return values:

```
enum s2_access_retvals {
    s2_access_error = -1,
    s2_access_nullval = 0,
    s2_access_success = 1,
};
```

Success returns are `s2_access_success` and `s2_access_nullval`.

- `s2_access_nullval` is returned by getter functions when a non-existent
  member is requested,

- `s2_access_success` is returned by getter and setter functions
  on success.

- `s2_access_error` is only returned when **internal** errors occur,
  which are distinct from what normally would occur with container
  access and are considered exceptional.

**2024-10-09 Update**

To support type safety, each container getter function gets an "_T"-suffixed
counterpart. 

For example, previously, to get a member from a dict, one has to spell out 
the cast verbosely: `s2dict_get(dict, key, (s2obj_t **)&out)`. One major 
downside of this is that, if the type of `out` changes, type errors 
won't be caught.

The new way to get an object from a dict is:
`s2dict_get_T(s2list_t)(dict, key, &out)`. An advantage of this approach
is that the type of `out` is clear, and it allows for consistency check.

This new change is implemented as macros in terms of older interfaces, and
the existing code don't need to haste to migrate as both methods are valid
and intended for general public use.

## 1.5. About Thread Safety and Garbage Collection

The garbage collection mechanism is threading aware - when GC is requested,
it obtains relevant locks to protect the GC operation from interference from
other threads. When threads are operating on objects, each thread should
obtain the (shared a.k.a. "reader") lock to prevent interference from the GC.

Sharing the "reader" lock allows different parts of the program to proceed
in parallel without blocking each other. Clusters of objects used by various
threads needs to be protected through explicit synchronization, using e.g.
the "cluster" mutex mentioned in the Readme file in the repo root.

One of the features of the locks ("reader" locks and GC locks) is that they're
recursion-adaptive - they can be locked recursively by different parts of the
program, and still maintain functional correctness; even if some threads are
holding the "reader" lock, GC is still possible as long as all threads that's
holding the "reader" lock enters GC subroutine eventually simultaneously.

To obtain and release threads lock, call `s2gc_thrd_lock()` and
`s2gc_thrd_unlock()` respectively; to enter GC, call `s2gc_collect()`.

# 2. Data Types' Description

This section describe each specific types.

## 2.1. `s2obj_t`

This is generic "root" type of all types under the SafeTypes2 type hierarchy.

It was recognized that type-safety is a necessity. Therefore, following
type-checking macros are added:

- `s2_is_data(obj)`
- `s2_is_dict(obj)`
- `s2_is_list(obj)`
- `s2_is_ref(obj)`

When objects of other concrete data types are passed to parameters or assigned
to variables of `s2obj_t *`, one can write `&obj->base` to let compiler enforce
type safety. Getting objects out of containers is unfortunately at this moment
not type-safe, as the type of the parameter is `s2obj_t **` - a nested pointer.

## 2.2. `s2data_t`

This type is used to hold arbitrary data. The design goal has been to make it
easy to safely access datum within the address range of the underlaying buffer,
and to safely resize the buffer when needed.

To this end, 3 most essential functions are devised, they are:
`s2data_map()`, `s2data_unmap()`, and `s2data_trunc()`.

`s2data_map(x, offset, len)` takes an offset and a length, check it against
the currently allocated buffer length. If the offset and the length are within
range, it returns a pointer to the initial address of the buffer plus
the offset, and add 1 to an internal count. Caller is responsible for not
accessing  beyond the range it had requested of.

Since this [2024-03-06-nul-term] revision, it is safe to pass the entire
mapped buffer pointer to functions that expect nul-terminated strings,
as the implementation now internally adds an extra nul byte just one byte
beyond the effective length of the buffer.

The implementation allocates a buffer in addition to the `s2data_t` context
working data structure to hold the actual data. In versions after 2024-09-11,
when the requested buffer size is small (less than or equal to 19 bytes), 
the data is held "inline" in the context working data structure to save the
additional allocation of the buffer to save heap space, and to accelerate
processing of small datum.

`s2data_unmap(x)` decreases the internal count mentioned above. This internal
count is used to make sure when resizing the buffer, no one accidentally access
the old invalid buffer address should the underlaying `realloc` call actually
relocate the buffer somewhere else. To this end, this function does not need
to know the exact ranges mapped, only a total is required.

`s2data_trunc(x, len)` takes a length parameter, checks no one is currently
mapping any range of the buffer, and resize it.

From real-world experience, it's discovered that adding bytes 1-by-1 is a
frequent task. Before 2024-11-12, this functionality is achieved by calling
code in ad-hoc ways. However, since there's now an inline buffer, growing
the data buffer 1 byte at a time is now possible in an efficient way. Small
amounts can be handled entirely by the inline buffer; where as larger ones
can first accumulate in the (reused) inline buffer, and calling `realloc(3)`
less frequently 1 chunk at a time.

The `s2data_putc(x, c)` function put 1 byte (with the value of `c`) onto the
end of `x`. For efficiency reasons, this byte may not necessarily be visible
right away; if all bytes have been provided, `s2data_putfin()` can be called
to indicate that no more data is pending.

The `s2data_putfin(x)` flushes any accumulated data so that they're guaranteed
to be visible at the next access operation (i.e. trunc, map, etc.)

## 2.3. `s2dict_t`

The dictionary type `s2dict_t` is keyed by `s2data_t`, as a
pridefully shameless practice of dogfooding. The most essential functions
of this type are `s2dict_get`, `s2dict_set`, and `s2dict_unset`.

The `s2dict_get_T(type)(dict, key, type **out)` generic function places
the requested member in the `out` pointer which points to an object of 
type `type *`, and returns one of the `s2_access_*` value described in:
"General Usage >> Accessing Container"
The obtained member is unretained - caller has to explicitly call
`s2obj_retain` on it, as SafeTypes2 assume the acquisition is temporary.

`s2dict_set(dict, key, value, semantic)` places (or replaces if there's an
existing value) into the  dictionary under they specified key, using the
specified `s2_setter_*` semantic.

`s2dict_unset(dict, key)` unsets (a.k.a. deletes) the value at the specified
key. Like many existing language type systems, this differs from setting the
key to null in that, unsetting the slot causes the getter to return indication
that the slot has no value, where as setting it to null indicates that the
slot has a value.

## 2.4. `s2list_t`

This is the *ordered* sequence type. As there's no silver bullet for a type
to support both fast arbitrary indexed access, and fast arbitrary slicing,
the direction of design decision lean towards more primitive operations that
can readily compose into useful sequential operations.

To this end, 4 functions are provided to insert and remove items from the list
(with 1 being dis-recommended). A few others provide helper functionalities.

The list has a cursor, which is a non-negative integer value no greater than
the length of the list. The list contains a pointer to the element at the
position indicated by the cursor.

The `s2list_insert(list, obj, semantic)` and `s2list_push(list, obj, semantic)`
functions  insert elements at the current cursor position. `s2list_insert`
does not advance the cursor position, where as `s2list_push` does.

The `s2list_shift_T(type)(list, type **out)` generic function 
removes an element from the current cursor position without changing it, 
and place the removed element in the `out` pointer to a variable of
type `type *`, and returns one of the `s2_access_*` indication. 
Because the element is no longer in the list, reference count is increased
(with the kept count decremented).

The `s2list_pop_T(type)(list, type **out)` generic function retreats 
the cursor position by 1 and remove the element at the then cursor position
and place it in the `out` pointer to a variable of type `type *`. 
As noted in the comments for this function in the "s2list.h" header, 
the function is implemented in terms of `s2list_shift`, and is
inefficient and redundant.

The `s2list_get_T(type)(list, type **out)` generic function gets the object
at the current cursor position. The reference and kept counts're not changed.

The `s2list_pos` and `s2list_len` functions reports the current cursor position
and the length of the list respectively.

`s2list_seek` alters the current cursor position:
- when `whence` is `S2_LIST_SEEK_SET`, it's set to 0+offset,
- when `whence` is `S2_LIST_SEEK_END`, it's set to [length]+offset,
- when `whence` is `S2_LIST_SEEK_CUR`, it's set to [cursor]+offset.

`s2list_sort(x, cmpfunc)` is a primitive implementation of sorting over the
list. Because theoretically, all sorting algorithms require O(n^2) time+memory
resource, insertion sort is used for its
- memory-space efficiency,
- implementation simplilcity.

The `cmpfunc` shall be compatible with the prototype:
`int (*)(s2obj_t *a, s2obj_t *b);`. It shall return less than 0 if a is
ordered before b, and greater than 0 if ordered after, and 0 if a and b
are equivalent in order.

The `s2data_cmp` function is one such that is compatible with the prototype.
It compares 2 data string byte-wise similar to `memcmp`, except when the
shorter one is the prefix of the longer one, the shorter one is ordered before.

## 2.5. `s2ref_t`

The reference type `s2ref_t` serves 2 purposes:

1. it allows arbitrary user resource from the application or 3rd-party library
   be managed under SafeTypes2.

2. it enables the program to (either entirely, or in compartments) opt out of
   garbage collection and use weak references instead.

The `s2ref_create(ptr, finalizer)` function creates a reference that contains
the resource referred to by `ptr`, and finalized by calling `finalizer(ptr)`.

The `s2ref_create_weakref(ptr)` function is equivalent to
calling `s2ref_create(ptr, NULL)`.

The `s2ref_unwrap(ref)` returns the `ptr` specified during creation.

# 3. Custom Types

It is possible to define custom data types that can integrate into the
SafeTypes2 eco-system. The following code listing demonstrate how to implement
a "pair" type using SafeTypes2 facility.

**"pair.h":**

```
#ifndef mylibrary_pair_h
#define mylibrary_pair_h 1

#include <SafeTypes2/s2data.h>

#define TYPE_PAIR 0x2001
#define type_is_pair(obj) (obj->type == TYPE_PAIR)

#define T struct typectx_pair
typedef T pair_t;

#ifndef mylibrary_implementing_pair
// This allows enforcing type-safe passing of the 'pair' type
// to variables and parameters.
T {
    s2obj_t base;
    int payload_context[];
};
#endif /* mylibrary_implementing_pair

// ``refcnt'' and ``keptcnt'' are not incremented.
int pair_get_left (T *pair, s2data_t **out);
int pair_set_right(T *pair, s2data_t **out);

int pair_set_left (T *pair, s2data_t *in, int semantic);
int pair_set_right(T *pair, s2data_t *in, int semantic);

#ifndef mylibrary_implementing_pair
#undef T
#endif /* mylibrary_implementing_pair */

#endif /* mylibrary_pair_h */
```

**"pair.c":**

```
#define mylibrary_implementing_pair
#define safetypes2_implementing_misc
#include "pair.h"

struct typectx_pair {
    s2obj_t basetype;
    s2data_t *left, *right;
};

static void pair_final(T *pair)
{
    // [note-refcnt-retained]
    // Because ``s2data_t'' is not a container type,
    // we don't have to concern with reference loop.
    if( pair->left  ) s2obj_release((s2obj_t *)pair->left);
    if( pair->right ) s2obj_release((s2obj_t *)pair->right);
}

T *pair_create()
{
    T *ret = NULL;

    ret = (T *)s2gc_obj_alloc(TYPE_PAIR, sizeof(T));
    if( !ret ) return NULL;

    ret->left = NULL;
    ret->right = NULL;

    ret->basetype.itercreatf = NULL;
    ret->basetype.finalf = (s2func_final_t)pair_final;
    return ret;
}

int pair_get_left(T *pair, s2data_t **out);
{
    *out = pair->left;
    return *out ? s2_access_success : s2_access_nullval;
}

int pair_get_right(T *pair, s2data_t **out);
{
    *out = pair->right;
    return *out ? s2_access_success : s2_access_nullval;
}

int pair_set_left(T *pair, s2data_t *in, int semantic)
{
    // See note tagged [note-refcnt-retained].
    if( pair->left ) s2obj_release((s2obj_t *)pair->left);
    pair->left = in;
    if( semantic == s2_setter_kept ) s2obj_retain((s2obj_t *)pair->left);
}

int pair_set_right(T *pair, s2data_t *in, int semantic)
{
    // See note tagged [note-refcnt-retained].
    if( pair->right ) s2obj_release((s2obj_t *)pair->right);
    pair->right = in;
    if( semantic == s2_setter_kept ) s2obj_retain((s2obj_t *)pair->right);
}
```

It needs to be heeded that, if your type contain component(s) that're container
types, or type that refer to objects of arbitrary type, then your type need to
implement an iterator that enumerates at least these components - this is
needed for GC to function correctly.

For example, if your type contain a component that is a dict or list, then
you need to implement the iterator; if however there's only data, then it's
not needed, as `s2data_t` isn't a container type and doesn't reference other
objects.

# 4. Other Info

The SafeTypes2 library is free and open-source software place in the
Public Domain by DannyNiu/NJF. Refer to header files for function prototypes,
and source code for detailed implementation information.
