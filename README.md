SafeTypes2
==========

Introduction
------------

Reference counting is a popular method for resource management. Its advantages
include avoid copying, small overhead, and easy to reason with. However, when
there's reference cycle, memory leak occurs. Typical solution to this is to use
weak references. My comment on this is:

> memory leak caused by reference cycle is a by-product of 
> conflating memory management and data structure.

Instead of requiring the programmer to carefully implant weak references,
in the original SafeTypes library, a different approach was chosen, where
users are required to specify a "semantic" when assigning objects to 
containers. Those semantics included:

- `keep`

  The object will be used later elsewhere, do not decrease reference count.
  
- `gave`: 

  The object will not be used any more elsewhere, change one of its reference
  to a container reference (known as "kept count" in the implementation). 
  In other word: decrease the reference count and increase its kept count. 
  This is most used when an object is created exclusively for saving on that
  container object.
  
- `fast`:

  The `gave` semantic invokes garbage collector after altering the reference
  and kept count - in case a reference cycle is created exactly when the
  assignment occurs. If the programmer can be sure that this won't occur, 
  they can use this semantic for optimization.

A sophisticated procedure was used to clean up circular references in the
original SafeTypes library. 

In the new SafeTypes2, the `fast` is subsummed into `gave`, and a "precise"
garbage collection scheme is used. The reason the GC is able to be "precise"
is because references had been categorized into: `refcnt` - references from
lexical variables, and `keptcnt` - references from container objects.

The SafeType2 library is also "threading-aware" - the GC system of SafeTypes2
incorporates facility that allow safe creation and destruction of objects in
multi-threaded programs; explicit synchronization from application is HOWEVER
still needed to safely operate on objects created using the library. 

Considering:
1. explicitly locking every individual objects incurs too much overhead.
2. group of objects (and related resources obtained elsewhere) may be
   operated on in concert, and a single "big" mutex may be able to offer
   "necessary and sufficient" protection.

Applications are recommended to use "cluster mutex" to protect clusters of
related objects, and not to too frequently call synchronization functions.

Features
--------

The library implements 3 types that're safe with regard to memory management:

- data: for storing arbitrary byte string, and can be resized safely.
- dict: a dictionary type that stores key-value pairs.
- list: the ordered set type.

The library uses SipHash in the dictionary type. According to its designer,
this hash function can be keyed to deter attacks on hash collision in dict.
Users are strongly advised to set the key to one that's securely generated
and that's different each time the program is run.
