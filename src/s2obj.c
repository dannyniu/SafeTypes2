/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#if __has_include(<pthread.h>)
#include <pthread.h>
#else
#error If Pthread isn't available, adapt the code accordingly.
#endif /* !__has_include(<pthread.h>) */

#define safetypes2_implementing_obj
#include "s2obj.h"

// The SafeTypes2 GC is threading-aware with the help of a
// **reader-writer lock**. In multi-threaded applications, threads obtain
// "reader" locks to prevent GC from operating on data structures that
// the threads may be using. When GC operates, the "writer" lock is locked,
// and all threads are stalled and prevented from operating on objects
// so that GC can safely traverse the graph of reachable objects,
// marking them and releasing unreachable objects as needed.
//
// This mechanism applies to the GC, and not the rest of the application -
// threads still need to prevent threading conflict using explicit
// synchronization.

struct gc_anchor_ctx
{
    pthread_mutex_t lck_master;
    pthread_cond_t cv_threads, cv_gc;

    // it is assumed that this will only be changed
    // when there's only 1 thread running.
    bool threaded;

    // When GC is in-progress, ``lck_master'' shall be
    // held by the GC thread. Where as when the GC is
    // pending, the GC thread may be blocked waiting
    // for ``cv_gc''.
    bool gc_inprogress;

    // Total threads made GC calls.
    size_t gc_waiting;

    // Total GC calls made from within the ``thr_count'' threads.
    size_t gc_pending;

    // This variable records the number of threads that's
    // currently occupying some objects created from GC.
    size_t thr_count;

    T *head;
    T *tail;
};

static struct gc_anchor_ctx gc_anch = {
    .lck_master = PTHREAD_MUTEX_INITIALIZER,
    .cv_threads = PTHREAD_COND_INITIALIZER,
    .cv_gc = PTHREAD_COND_INITIALIZER,
    .threaded = true,
    .gc_inprogress = false,
    .gc_waiting = 0,
    .gc_pending = 0,
    .thr_count = 0,
    .head = NULL,
    .tail = NULL,
};

// This allows making "reader" lock recursive. The other way of making
// the "reader" lock recursive would make it impossible to make explicit
// GC call - namely, if the recursion counts are counted on ``thr_count'',
// then there will be no way to compare it with ``gc_pending''.
static pthread_key_t thrd_recursion_counter;
static bool thrd_recursion_counter_initialized = false;

static uint64_t mark_last = 0;

T *s2gc_obj_alloc(s2obj_typeid_t type, size_t sz)
{
    T *ret;
    assert( sz > sizeof(T) );

    ret = calloc(1, sz);
    if( !ret ) return NULL;

    static_assert(
        sizeof(ret->type) == sizeof(type),
        "Type specifier needing update or adaptation!" );
    ret->type = type;
    ret->guard = 0;
    ret->mark = mark_last;
    ret->refcnt = 1;
    ret->keptcnt = 0;

    // the lock is expected to be short-term.
    if( gc_anch.threaded )
        pthread_mutex_lock(&gc_anch.lck_master);

    if( !gc_anch.head )
    {
        assert( !gc_anch.tail );
        gc_anch.head = gc_anch.tail = ret;
        ret->gc_prev = ret->gc_next = NULL;
    }
    else
    {
        assert(  gc_anch.tail );
        assert( !gc_anch.tail->gc_next );

        gc_anch.tail->gc_next = ret;
        ret->gc_prev = gc_anch.tail;
        ret->gc_next = NULL;
        gc_anch.tail = ret;
    }

    if( gc_anch.threaded )
        pthread_mutex_unlock(&gc_anch.lck_master);
    return ret;
}

void s2gc_obj_dealloc(T *restrict obj)
{
    // the lock is expected to be short-term.
    if( gc_anch.threaded )
        pthread_mutex_lock(&gc_anch.lck_master);

    if( !obj->gc_prev )
    {
        assert( obj == gc_anch.head );
        gc_anch.head = obj->gc_next;
    }
    else
    {
        obj->gc_prev->gc_next = obj->gc_next;
    }

    if( !obj->gc_next )
    {
        assert( obj == gc_anch.tail );
        gc_anch.tail = obj->gc_prev;
    }
    else
    {
        obj->gc_next->gc_prev = obj->gc_prev;
    }

    if( gc_anch.threaded )
        pthread_mutex_unlock(&gc_anch.lck_master);

    memset(obj, 0, sizeof(*obj));
    free(obj);
}

T *s2obj_retain(T *restrict obj)
{
    ++obj->refcnt;
    return obj;
}

T *s2obj_keep(T *restrict obj)
{
    ++obj->keptcnt;
    return obj;
}

void s2obj_release(T *restrict obj)
{
    if( obj->guard == 1 ) return; // Added 2024-02-26.

    assert( obj->refcnt > 0 );
    if( !--obj->refcnt && !obj->keptcnt )
    {
        // Added 2024-02-26.
        if( gc_anch.gc_inprogress )
            obj->guard = 1;

        // 2024-02-21:
        // Finalize the context of the object
        // first. This will ensure child objects
        // are deallocated beforehand and no
        // deadlock can occur with the following
        // deallocation call to the current object.
        obj->finalf(obj);

        if( gc_anch.gc_inprogress )
        {
            // 2024-02-27:
            // GC scanning may encounter some that're not released
            // until their container(s) are gone.
            obj->mark = mark_last|1;
        }
        else s2gc_obj_dealloc(obj);
    }
}

void s2obj_leave(T *restrict obj)
{
    if( obj->guard == 1 ) return;

    assert( obj->keptcnt > 0 );
    if( !--obj->keptcnt && !obj->refcnt )
    {
        // Added 2024-02-26.
        if( gc_anch.gc_inprogress )
            obj->guard = 1;

        // 2024-02-21: see note in ``*_release''.
        obj->finalf(obj);

        if( gc_anch.gc_inprogress )
        {
            // 2024-02-27: see note in ``*_release''.
            obj->mark = mark_last|1;
        }
        else s2gc_obj_dealloc(obj);
    }
}

s2iter_t *s2obj_iter_create(T *restrict obj)
{
    if( !obj->itercreatf )
        return NULL;
    else return obj->itercreatf(obj);
}

static void s2gc_thrd_recursion_initializer()
{
    register int e;
    if( !thrd_recursion_counter_initialized )
    {
        // This must only be done (only once) while
        // ``lck_master'' is held by the one thread.
        e = pthread_key_create(&thrd_recursion_counter, NULL);
        assert( e == 0 ); // 2024-02-24: cannot recover from this error.
        thrd_recursion_counter_initialized = true;
    }
}

int s2gc_thrd_lock()
{
    int e = 0;
    intptr_t threc;

    if( !gc_anch.threaded )
    {
        // The application is single-threaded, no need to lock.
        return 0;
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) )
        return e;

    s2gc_thrd_recursion_initializer();

    while( gc_anch.gc_waiting > 0 || gc_anch.gc_inprogress )
    {
        e = pthread_cond_wait(&gc_anch.cv_threads, &gc_anch.lck_master);
        assert( e == 0 ); // 2024-02-24: caller can't recover from this error.
    }

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc >= 0 );

    if( 0 == threc++ ) ++gc_anch.thr_count;
    e = pthread_setspecific(thrd_recursion_counter, (const void *)threc);
    assert( e == 0 ); // 2024-02-24: cannot recover from this error.

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 0;
}

int s2gc_thrd_unlock()
{
    int e = 0;
    intptr_t threc;

    if( !gc_anch.threaded )
    {
        // The application is single-threaded, no need to lock.
        return 0;
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) )
        return e;

    s2gc_thrd_recursion_initializer();

    assert( gc_anch.thr_count > 0 );

    // Anomaly enumeration:
    //
    // - ``gc_inprogress'' is true: (enumerated 2024-02-24.)
    //   GC is supposed to be only operating before or after
    //   this function, never during it runs.
    //
    //   The (intuitively) valid scenario(s):
    //   1. *_gcop_lock(); *_gcop_unlock(); *_thrd_unlock();
    //   2. *_thrd_unlock(); *_gcop_lock(); *_gcop_unlock();
    //
    //   The multi-threaded scenario(s):
    //   1. *_gcop_lock(); *_thrd_unlock(); *_gcop_unlock();
    //      This implies that there's a *_thrd_lock() before *_gcop_lock(),
    //      in which case *_gcop_lock() is supposed to be blocked until
    //      after *_thrd_unlock(); returns.
    //
    // - nothing else for now.

    assert( !gc_anch.gc_inprogress );

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc > 0 );

    if( 0 == --threc ) --gc_anch.thr_count;
    if( gc_anch.gc_pending >= gc_anch.thr_count && gc_anch.gc_waiting > 0 )
    {
        e = pthread_cond_broadcast(&gc_anch.cv_gc);
        assert( e == 0 ); // 2024-02-24: cannot recover from this error.
    }

    e = pthread_setspecific(thrd_recursion_counter, (const void *)threc);
    assert( e == 0 ); // 2024-02-24: cannot recover from this error.

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 0;
}

// "gcop" = "garbage collection operation".
// returns 1 to indicate GC should operate on the current thread,
// returns 0 to indicate the thread isn't responsible for GC.
static int s2gc_gcop_lock()
{
    int e = 0;
    intptr_t threc;

    if( !gc_anch.threaded )
    {
        // The application is single-threaded, no need to lock.
        return 1; // This return value is special & specific *here*.
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) )
        return e;

    s2gc_thrd_recursion_initializer();

    ++gc_anch.gc_waiting;

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc >= 0 );
    if( threc > 0 ) ++gc_anch.gc_pending;

    while( gc_anch.gc_pending < gc_anch.thr_count )
    {
        // gc_waiting is definitely >0.
        e = pthread_cond_wait(&gc_anch.cv_gc, &gc_anch.lck_master);
        assert( e == 0 ); // 2024-02-24: caller can't recover from this error.
    }

    // 2024-02-24 note after testing: help waiting peers to wake up.
    pthread_cond_broadcast(&gc_anch.cv_gc);

    if( gc_anch.gc_inprogress )
    {
        // some thread had already started garbage collection operation,
        // this thread don't need to bother.
        pthread_mutex_unlock(&gc_anch.lck_master);
        return 0;

        // 2024-02-24 (low-priority):
        // maybe allow application to specify a thread to run GC?
        // shouldn't be (too) difficult to implement.
    }

    gc_anch.gc_inprogress = true;
    pthread_mutex_unlock(&gc_anch.lck_master);
    return 1;
}

static int s2gc_gcop_unlock()
{
    int e = 0;
    intptr_t threc;

    if( !gc_anch.threaded )
    {
        // The application is single-threaded, no need to lock.
        return 1; // This return value is special & specific *here*.
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) )
        return e;

    s2gc_thrd_recursion_initializer();

    --gc_anch.gc_waiting;

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc >= 0 );
    if( threc > 0 ) --gc_anch.gc_pending;

    while( gc_anch.gc_waiting > 0 )
    {
        // 2024-02-24:
        // Take this example (t means thread, g means
        // garbage collector, l means lock, u means unlock):
        // ---------------
        // Thrd 1 | Thrd 2
        // T L    |
        // G L    |
        //        | G L
        // G U    |
        // t u    | g u <-- notice here.
        // ---------------
        // If we don't stall all the threads that requested GC
        // until the GC is complete, otherwise, the atomicity
        // invariant of ``*_gcop_{,un}lock'' could be violated.

        e = pthread_cond_wait(&gc_anch.cv_gc, &gc_anch.lck_master);
        assert( e == 0 ); // 2024-02-24: caller can't recover from this error.
    }

    gc_anch.gc_inprogress = false;
    pthread_cond_broadcast(&gc_anch.cv_gc);
    pthread_cond_broadcast(&gc_anch.cv_threads);
    // 2024-02-24: ignoring (possibly fatal) error(s) for now.

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 0;
}

#ifdef UniTest_S2GC_Locking

#include <unistd.h>

static int ctr = -1;
static pthread_mutex_t m_prog = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cv_prog = PTHREAD_COND_INITIALIZER;

#define PROG_DEF(arg_id, arg_call)                              \
    { .id = arg_id, .call = arg_call, .label = #arg_call }

static int barr()
{
    sleep(1);
    return 0;
}

static struct progress_entry {
    int id;
    int (*call)();
    char *label;
} progress[] = {
    PROG_DEF(1, s2gc_thrd_lock), // 0
    PROG_DEF(1, s2gc_thrd_lock), // 1
    PROG_DEF(2, s2gc_thrd_lock), // 2
    PROG_DEF(1, barr), // 3
    PROG_DEF(2, barr), // 4
    PROG_DEF(3, barr), // 5
    PROG_DEF(3, s2gc_gcop_lock), // 6
    PROG_DEF(1, s2gc_gcop_lock), // 7
    PROG_DEF(2, s2gc_gcop_lock), // 8
    PROG_DEF(3, s2gc_gcop_unlock), // 9
    PROG_DEF(1, s2gc_gcop_unlock), // 10
    PROG_DEF(2, s2gc_gcop_unlock), // 11
    PROG_DEF(1, s2gc_thrd_unlock), // 12
    PROG_DEF(1, s2gc_gcop_lock), // 13
    PROG_DEF(2, s2gc_gcop_lock), // 14
    PROG_DEF(1, s2gc_gcop_unlock), // 15
    PROG_DEF(2, s2gc_gcop_unlock), // 16
    PROG_DEF(1, s2gc_thrd_unlock), // 17
    PROG_DEF(2, s2gc_thrd_unlock), // 18
    PROG_DEF(0, NULL),
};

static void iwaitfor(int i)
{
    pthread_mutex_lock(&m_prog);
    while( ctr < i )
    {
        pthread_cond_wait(&cv_prog, &m_prog);
    }
    ctr++;
    pthread_cond_broadcast(&cv_prog);
    pthread_mutex_unlock(&m_prog);
}

static intptr_t thrd(intptr_t id)
{
    long i = 0;
    for(i=0; progress[i].call; i++)
    {
        if( id != progress[i].id && progress[i].id != 0 ) continue;
        iwaitfor(i);
        printf("%ld: calling %s from %ld,\n", i, progress[i].label, (long)id);

        progress[i].call();
        printf("%ld: call from %ld returned.\n", i, (long)id);
    }
    return id;
}

typedef void *(*thrd_entry_func_t)(void *);

int main()
{
    pthread_t t1, t2, t3;
    /* printf(
       "Due to limitation of testing code, it's possible for this\n"
       "test to fail occasionally even though it's overall correct.\n"
       "If the test passes more than it fails, consider it passing.\n"); */
    pthread_create(&t1, NULL, (thrd_entry_func_t)thrd, (void *)1);
    pthread_create(&t2, NULL, (thrd_entry_func_t)thrd, (void *)2);
    pthread_create(&t3, NULL, (thrd_entry_func_t)thrd, (void *)3);
    iwaitfor(-1);
    pthread_cond_broadcast(&cv_prog);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    return 0;
}

#endif /* UniTest_S2GC_Locking */

void s2gc_collect(void)
{
    size_t ti = 0; // traversal iteration.
    size_t oc = 0; // objects count.
    T *p, *q;
    s2iter_t *it;
    int i;

    // leave 1 bit for marked but not traversed objects.
    uint32_t mark;

    if( s2gc_gcop_lock() != 1 )
    {
        s2gc_gcop_unlock();
        return;
    }

    assert( mark_last % 2 == 0 );
    mark = mark_last + 2;

    do
    {
        p = gc_anch.head;
        while( p )
        {
            if( p->refcnt > 0 && p->mark != (mark|1) )
                p->mark = mark;

            if( p->mark == mark )
            {
                it = s2obj_iter_create(p);
                for(i=it?it->next(it):-1; i>0; i=it->next(it))
                {
                    q = it->value;
                    q->mark = mark;
                }
                if( it ) it->final(it);
                p->mark |= 1;
            }

            if( ti == 0 ) oc++;
            p = p->gc_next;
        }
    }
    while( ti++ < oc );

    for(p=gc_anch.head; p; p=p->gc_next)
    {
        if( (p->mark|1) != (mark|1) )
        {
            if( p->guard == 1 ) continue;
            p->guard = 1;
            p->finalf(p);
        }
    }

    while( true )
    {
        oc = 0;
        p = gc_anch.head;
        if( !p ) break;
        assert( p );

        if( (p->mark|1) != (mark|1) )
        {
            s2gc_obj_dealloc(p);
            oc++;
        }
        else p = p->gc_next;

        if( oc > 0 ) continue;
        else break;
    }

    mark_last = mark;
    s2gc_gcop_unlock();
}
