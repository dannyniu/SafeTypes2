API Document
============

This document complements the comments in the source code header files in
describing the API of the SafeTypes2 library.

General Usage
=============

This section describe features that're common to more than one data type.

Creating Objects
----------------

To create objects, declare objects of pointers to relevant types,
then assign the result of `s2{data,dict,list}_create()` to them.

```
s2{data,dict,list}_t *x = s2{data,dict,list}_create(...necessary params...);

...  using the object ...

s2obj_release(x);
```

Operating on Reference Count
----------------------------

In general, users should be using `s2obj_retain()` and `s2obj_release()` to
operate on objects in lexical variables. `s2obj_keep` and `s2obj_leave` are
reserved internally for implementing container types - they should not be
used unless you're implementing container.

Enumerating Container Members
-----------------------------

To enumerate members of a collection, iterators are needed. They are not
"objects" in the sense that their lifetime (and resource) are not managed
by the reference counting and garbage collection mechanism.

After creating the iterator, a call to the stepping function is needed
to set the position of the iterator to the initial element.

```
s2iter_t *it = s2obj_iter_create(x);
s2obj_t *e;
for(e=it->next(it); e; e=it->next(it))
{
    .... process 1 member ...
}
```

Accessing Container
-------------------

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

- `*_nullval` is returned by getter functions when a non-existent
  member is requested, otherwise
  
- `*_success` is returned.

- `s2_access_error` is only returned when **internal** errors occur, 
  which are distinct from what normally would occur with container
  access and are considered exceptional.

About Thread Safety
-------------------

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

Data Types' Description
=======================

This section describe each specific types.

`s2obj_t`
---------

This is generic "root" type of all types under the SafeTypes2 type hierarchy.

`s2data_t`
----------

This type is used to hold arbitrary data. The design goal has been to make it
easy to safely access datum within the address range of the underlaying buffer,
and to safely resize the buffer when needed.

To this end, 3 most essential functions are devised, they are:
`s2data_map()`, `s2data_unmap()`, and `s2data_trunc()`. 

`s2data_map` takes an offset and a length, check it against the currently 
allocated buffer length. If the offset and the length are within range,
it returns a pointer to the initial address of the buffer plus the offset,
and add 1 to an internal count. Caller is responsible for not accessing 
beyond the range it had requested of.

`s2data_unmap` decreases the internal count mentioned above. This internal
count is used to make sure when resizing the buffer, no one accidentally access
the old invalid buffer address should the underlaying `realloc` call actually
relocate the buffer somewhere else. To this end, this function does not need
to know the exact ranges mapped, only a total is required.

`s2data_trunc` takes a length parameter, checks no one is currently mapping
any range of the buffer, and resize it.

`s2dict_t`
----------

The dictionary type `s2dict_t` is keyed by `s2data_t`, as a 
pridefully shameless practice of dogfooding. The most essential functions 
of this type are `s2dict_get`, `s2dict_set`, and `s2dict_unset`.

`s2dict_get` places the requested member in a pointer to `s2obj_t`, and returns
one of the `s2_access_*` value described in: 
"General Usage >> Accessing Container"
The obtained member is unretained - caller has to explicitly call 
`s2obj_retain` on it, as SafeTypes2 assume the acquisition is temporary.

`s2dict_set` places (or replaces if there's an existing value) into the 
dictionary under they specified key, using the specified `s2_setter_*` 
semantic.

`s2dict_unset` unsets (a.k.a. deletes) the value at the specified key. Like
many existing language type systems, this differs from setting the key to null
in that, unsetting the slot causes the getter to return indication that the
slot has no value, where as setting it to null indicates that the slot has a
value.

`s2list_t`
----------

This is the *ordered* sequence type. As there's no silver bullet for a type
to support both fast arbitrary indexed access, and fast arbitrary slicing,
the direction of design decision lean towards more primitive operations that
can readily compose into useful sequential operations.

To this end, 4 functions are provided to insert and remove items from the list
(with 1 being dis-recommended), and 2 providing helper functionalities.

The list has a cursor, which is a non-negative integer value no greater than 
the length of the list. The list contains a pointer to the element at the 
position indicated by the cursor.

The `s2list_insert` and `s2list_push` functions insert elements at the current
cursor position. `s2list_insert` does not advance the cursor position, where as
`s2list_push` does.

The `s2list_shift` function removes an element from the current cursor position
without changing it, and place the removed element in a pointer to `s2obj_t`.
Because the element is no longer in the list, reference count is increased
(while the kept count is simultaneously decreased).

The `s2list_pop` function retreats the cursor position by 1 and remove the
element at the then cursor position and place it in a pointer to `s2obj_t`.
As noted in the comments for this function in the "s2list.h" header,
the function is implemented in terms of `s2listShift`, and is inefficient and
redundant.

`s2list_seek` alters the current cursor position:
- when `whence` is `S2_LIST_SEEK_SET`, it's set to 0+offset,
- when `whence` is `S2_LIST_SEEK_END`, it's set to [length]+offset,
- when `whence` is `S2_LIST_SEEK_CUR`, it's set to [cursor]+offset.

`s2list_sort` is a primitive implementation of sorting over the list.
Because theoretically, all sorting algorithms require O(n^2) time+memory
resource, insertion sort is used for its
- memory-space efficiency,
- implementation simplilcity.

Other Info
==========

The SafeTypes2 library is free and open-source software place in the 
Public Domain by DannyNiu/NJF. Refer to header files for function prototypes,
and source code for detailed implementation information
