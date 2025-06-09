2025-06-09
==========

Description
-----------

The garbage collector synchronization mechanism for multi-threaded applications
is entirely re-written based on more rigorous and formal design and reasoning.

The ultimate goal is to ensure garbage collection won't interfere with
application use of SafeTypes2 objects. The extended goals are as follow:
1. the thread that performs GC shouldn't be fixed,
2. any thread can request GC even if it's holding a shared "reader" lock.

To this end, we need to first track the number of threads holding
the shared "reader" lock, hence "thread" lock, thus the `thr_count` variable.
Secondly, since multiple threads can enter GC, we need a counter for that as
well, thus the `gc_waiting` variable. Thirdly, since some threads requesting
GC aren't necessarily the one(s) holding the thread lock, we need a third
variable to keep a count, thus we need another `gc_pending` variable.

Because threads lock are shared (i.e. not exclusive), they only have to
atomically change the `thr_count` counter; on the other hand, GC lock must
block the calling threads until all other locks are released - this includes:
- the threads locks; unless they also attempt to acquire the GC lock,
- any previous unfinished GC locks.
Also, GC operation should have priority over application activity, 
to ensure resource availability. 

To this end, the GC synchronization mechanism have several states:
1. 'initial state', where threads locks may be acquired freely,
2. 'GC entry state', where further threads lock acquisitions are blocked,
3. 'GC operation state', a thread is performing GC, other threads should wait.
4. 'GC finish state', further GC locking should wait.

Sketch Implementation
---------------------

**Synchronization Object State Variables**:
- `stateguard` - the current state of the synchronization object.
- `thr_count` - records the number of threads holding the "reader" lock.
- `gc_waiting` - the number of threads blocked waiting for GC operation.
- `gc_pending` - the number of threads holding the threads lock that're
                 blocked waiting for GC operation.
- `threc` - a thread-local variable initialized to 0, introduced to support
            lock recursion for the threads lock.

For ease of presentation, it is assumed that during the execution of the 
following procedures, no other threads will access the state variables
unless during the window of waiting, which will be explicitly indicated.

BEGIN - **To Aqcuire a Threads Lock**:
- {Wait} until `stateguard` becomes the 'initial state'.
- if `threc` is 0, increment `thr_count`.
- increment `threc`.
END

BEGIN - **To Release a Threads Lock**:
- {Wait} until `stateguard` becomes 'initial state' or 'GC entry state'.
- decrement `threc`.
- if `threc` is 0, decrement `thr_count`.
END

BEGIN - **To Acquire a GC Lock**:
- {Wait} until `stateguard` becomes 'initial state' or 'GC entry state'.
- set `stateguard` to 'GC entry state'.
- increment `gc_waiting`.
- if `threc` is greater than 0, then this thread is holding a threads lock,
  so increment `gc_pending`.
- {Wait} until `gc_pending` reaches `thr_count`.
- if `stateguard` is 'GC operation state', then {return}.
- otherwise, indicate to the current thread that it should perform GC.
  and set `stateguard` to 'GC operation state'.
END

BEGIN - **To Release a GC Lock**:
- {Wait} until `stateguard` becomes 'GC operation state' or 'GC finish state'.
- set `stateguard` to 'GC finish state'.
- decrement `gc_waiting`.
- {Wait} until `gc_waiting` reaches 0.
- If `threc` is greater than 0, then this thread is holding a threads lock,
  so decrement `gc_pending`. (Note that this step must come *after* waiting
  for `gc_waiting` to reach 0, to ensure the wait for `gc_pending` to 
  reach `thr_count` in GC lock acquisition don't fail spuriously.)
- set `stateguard` to 'initial state'.
END
