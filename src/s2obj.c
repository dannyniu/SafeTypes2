/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#define safetypes2_implementing_obj
#include "s2obj.h"

#ifndef SAFETYPES2_BUILD_WITHOUT_GC

#if __has_include(<pthread.h>)
#include <pthread.h>
#else
#error If Pthread isn't available, adapt the code accordingly.
#endif /* !__has_include(<pthread.h>) */

struct gc_anchor_ctx
{
    // 2025-06-09:
    // The GC synchronization mechanism is being re-written in entirity
    // based on sketch in "gc-lock-rewrite-2025-06-09.md".

    pthread_mutex_t lck_master;
    pthread_cond_t cv_state, cv_counter;

    // it is assumed that this will only be changed
    // when there's only 1 thread running.
    enum {
        threading_mono = false,
        threading_multi = true,
    } threaded;

    enum {
        stateguard_invalid,
        stateguard_free,
        stateguard_gc_waiting,
        stateguard_gc_operating,
        stateguard_gc_completing,
    } stateguard;
#define gc_inprogress stateguard == stateguard_gc_operating

    // This variable records the number of threads that's
    // currently occupying some objects created from GC -
    // i.e. holding the shared "reader" lock.
    size_t thr_count;

    // Number of threads blocked waiting for GC operation.
    size_t gc_waiting;

    // Number of threads holding shared "reader" lock that're
    // blocked wating for GC operation.
    size_t gc_pending;

    T *head;
    T *tail;
};

static struct gc_anchor_ctx gc_anch = {
    .lck_master  = PTHREAD_MUTEX_INITIALIZER,
    .cv_state  = PTHREAD_COND_INITIALIZER,
    .cv_counter = PTHREAD_COND_INITIALIZER,
    .threaded = true,
    .stateguard = stateguard_free,
    .thr_count = 0,
    .gc_waiting = 0,
    .gc_pending = 0,

    .head = NULL,
    .tail = NULL,
};

// This allows making "reader" lock recursive. The other way of making
// the "reader" lock recursive would make it impossible to make explicit
// GC call - namely, if the recursion counts are counted on `thr_count`,
// then there will be no way to compare it with `gc_waiting_enter`.
static pthread_key_t thrd_recursion_counter;
static bool thrd_recursion_counter_initialized = false;

static uint32_t mark_last = 0;

#endif /* SAFETYPES2_BUILD_WITHOUT_GC */

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
    ret->refcnt = 1;
    ret->keptcnt = 0;
    ((s2obj_base *)ret)->pobj = ret;

#ifndef SAFETYPES2_BUILD_WITHOUT_GC
    ret->mark = mark_last;

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
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */

    return ret;
}

void s2gc_obj_dealloc(T *restrict obj)
{
#ifndef SAFETYPES2_BUILD_WITHOUT_GC
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
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */

    memset(obj, 0, sizeof(*obj));
    free(obj);
}

#ifndef SAFETYPES2_BUILD_WITHOUT_GC
void s2gc_set_threading(bool enabled)
{
    gc_anch.threaded = enabled;
}
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */

T *s2obj_retain(T *restrict obj)
{
    ++obj->refcnt;
    return obj;
}

T *s2obj_keep(T *restrict obj)
{
#ifndef SAFETYPES2_BUILD_WITHOUT_GC
    ++obj->keptcnt;
    return obj;
#else /* !SAFETYPES2_BUILD_WITHOUT_GC */
    return s2obj_retain(obj);
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */
}

void s2obj_release(T *restrict obj)
{
    if( obj->guard == 1 ) return; // Added 2024-02-26.

    assert( obj->refcnt > 0 );
    if( !--obj->refcnt && !obj->keptcnt )
    {
#ifndef SAFETYPES2_BUILD_WITHOUT_GC
        // Added 2024-02-26.
        if( gc_anch.gc_inprogress )
            obj->guard = 1;
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */

        // 2024-02-21:
        // Finalize the context of the object
        // first. This will ensure child objects
        // are deallocated beforehand and no
        // deadlock can occur with the following
        // deallocation call to the current object.
        if( obj->finalf ) // 2024-08-15: weak reference support
            obj->finalf(obj);

#ifndef SAFETYPES2_BUILD_WITHOUT_GC
        if( gc_anch.gc_inprogress )
        {
            // 2024-02-27:
            // GC scanning may encounter some that're not released
            // until their container(s) are gone.
            obj->mark = mark_last|1;
        }
        else
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */
            s2gc_obj_dealloc(obj);
    }
}

void s2obj_leave(T *restrict obj)
{
#ifndef SAFETYPES2_BUILD_WITHOUT_GC
    if( obj->guard == 1 ) return;

    assert( obj->keptcnt > 0 );
    if( !--obj->keptcnt && !obj->refcnt )
    {
        // Added 2024-02-26.
        if( gc_anch.gc_inprogress )
            obj->guard = 1;

        // 2024-02-21: see note in `s2obj_release`.
        if( obj->finalf ) // 2024-08-15: weak reference support
            obj->finalf(obj);

        if( gc_anch.gc_inprogress )
        {
            // 2024-02-27: see note in `s2obj_release`.
            obj->mark = mark_last|1;
        }
        else s2gc_obj_dealloc(obj);
    }
#else /* !SAFETYPES2_BUILD_WITHOUT_GC */
    return s2obj_release(obj);
#endif /* SAFETYPES2_BUILD_WITHOUT_GC */
}

s2iter_t *s2obj_iter_create(T *restrict obj)
{
    if( !obj->itercreatf )
        return NULL;
    else return obj->itercreatf(obj);
}

#ifndef SAFETYPES2_BUILD_WITHOUT_GC

static void s2gc_thrd_recursion_initializer()
{
    register int e;
    if( !thrd_recursion_counter_initialized )
    {
        // This must only be done (only once) while
        // `lck_master` is held by the one thread.
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

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) != 0 )
        return -1;

    s2gc_thrd_recursion_initializer();

    while( gc_anch.stateguard != stateguard_free &&
           gc_anch.stateguard != stateguard_gc_waiting )
    {
        e = pthread_cond_wait(&gc_anch.cv_state, &gc_anch.lck_master);
        assert( e == 0 );
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

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) != 0 )
        return -1;

    s2gc_thrd_recursion_initializer();

    while( gc_anch.stateguard != stateguard_free &&
           gc_anch.stateguard != stateguard_gc_waiting )
    {
        e = pthread_cond_wait(&gc_anch.cv_state, &gc_anch.lck_master);
        assert( e == 0 );
    }

    assert( gc_anch.thr_count > 0 );

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc > 0 );

    assert( gc_anch.gc_pending <= gc_anch.thr_count );
    if( 0 == --threc ) --gc_anch.thr_count;
    if( gc_anch.gc_waiting > 0 )
    {
        if( gc_anch.gc_pending == gc_anch.thr_count )
        {
            e = pthread_cond_broadcast(&gc_anch.cv_counter);
            assert( e == 0 ); // 2024-02-24: cannot recover from this error.
        }
    }

    e = pthread_setspecific(thrd_recursion_counter, (const void *)threc);
    assert( e == 0 ); // 2024-02-24: cannot recover from this error.

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 0;
}

// "gcop" = "garbage collection operation".
// returns 1 to indicate GC should operate on the current thread,
// returns 0 to indicate the thread isn't responsible for GC.
// returns -1 to indicate errors.
static int s2gc_gcop_lock(long id)
{
    //- int ret = 0;
    int e = 0;
    intptr_t threc;

    (void)id; // 2024-05-19: was for debugging.

    if( !gc_anch.threaded )
    {
        // 2024-08-28:
        // On 2024-03-09, support was added to allow application to
        // opt-out of the overhead associated multi-threading GC
        // synchronization. However, the GC operation internal
        // protection was missing. The following line is added to
        // retroactively fix that.
        gc_anch.stateguard = stateguard_gc_operating;

        // The application is single-threaded, no need to lock.
        return 1; // This return value is special & specific *here*.
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) != 0 )
        return -1;

    s2gc_thrd_recursion_initializer();

    while( gc_anch.stateguard != stateguard_free &&
           gc_anch.stateguard != stateguard_gc_waiting )
    {
        e = pthread_cond_wait(&gc_anch.cv_state, &gc_anch.lck_master);
        assert( e == 0 );
    }
    gc_anch.stateguard = stateguard_gc_waiting;

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc >= 0 );

    gc_anch.gc_waiting ++;
    if( threc > 0 ) gc_anch.gc_pending ++;

    while( gc_anch.gc_pending < gc_anch.thr_count )
    {
        // gc_waiting_enter is definitely >0.
        e = pthread_cond_wait(&gc_anch.cv_counter, &gc_anch.lck_master);
        assert( e == 0 ); // 2024-02-24: caller can't recover from this error.
    }

    if( gc_anch.stateguard == stateguard_gc_operating )
    {
        pthread_mutex_unlock(&gc_anch.lck_master);
        return 0;
    }

    // else
    gc_anch.stateguard = stateguard_gc_operating;

    // 2024-05-18: ignoring (possibly fatal) error(s) for now.
    // 2025-06-09: still ignoring.
    pthread_cond_broadcast(&gc_anch.cv_state);
    pthread_cond_broadcast(&gc_anch.cv_counter);

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 1;
}

static int s2gc_gcop_unlock(long id)
{
    int e = 0;
    intptr_t threc;

    (void)id; // 2024-05-19: was for debugging.

    if( !gc_anch.threaded )
    {
        // 2024-08-28:
        // See note in `s2gc_gcop_lock` dated today.
        gc_anch.stateguard = stateguard_free;

        // The application is single-threaded, no need to lock.
        return 1; // This return value is special & specific *here*.
    }

    if( (e = pthread_mutex_lock(&gc_anch.lck_master)) )
        return -1;

    s2gc_thrd_recursion_initializer();

    while( gc_anch.stateguard != stateguard_gc_operating &&
           gc_anch.stateguard != stateguard_gc_completing )
    {
        e = pthread_cond_wait(&gc_anch.cv_state, &gc_anch.lck_master);
        assert( e == 0 );
    }
    gc_anch.stateguard = stateguard_gc_completing;

    --gc_anch.gc_waiting;

    while( gc_anch.gc_waiting > 0 )
    {
        e = pthread_cond_wait(&gc_anch.cv_counter, &gc_anch.lck_master);
        assert( e == 0 ); // 2024-02-24: caller can't recover from this error.
    }

    threc = (intptr_t)pthread_getspecific(thrd_recursion_counter);
    assert( threc >= 0 );
    if( threc > 0 ) --gc_anch.gc_pending;

    gc_anch.stateguard = stateguard_free;

    // 2024-02-24: ignoring (possibly fatal) error(s) for now.
    // 2025-06-09: still ignoring.
    pthread_cond_broadcast(&gc_anch.cv_state);
    pthread_cond_broadcast(&gc_anch.cv_counter);

    pthread_mutex_unlock(&gc_anch.lck_master);
    return 0;
}

void s2gc_collect(void)
{
    size_t ti = 0; // traversal iteration.
    size_t oc = 0; // objects count.
    T *p, *q;
    s2iter_t *it;
    int i;

    // leave 1 bit for marked but not traversed objects.
    uint32_t mark;

    i = s2gc_gcop_lock(0);
    assert( i != -1 );
    if( i == 0 ) // GC don't operate in this thread.
    {
        s2gc_gcop_unlock(0);
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
    s2gc_gcop_unlock(0);
}

#endif /* SAFETYPES2_BUILD_WITHOUT_GC */
