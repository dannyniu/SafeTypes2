/* DannyNiu/NJF, 2024-06-30. Public Domain. */

#include "s2obj.c"

#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

int urand_fd;

void randwait()
{
    struct timespec ts = {};
    read(urand_fd, &ts.tv_nsec, sizeof(ts.tv_nsec));
    ts.tv_nsec %= 128*1024*1024;
    nanosleep(&ts, NULL);
}

//typedef void *(*thrd_entry)(void *);
typedef void *thrd_entry;

void *tt_rdlock_once(size_t arg)
{
    (void)arg;
    randwait();
    if( s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest1(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest2(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest1alt2(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_alt3(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest2gc1(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest1alt2gc1a(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest1alt2gc1b(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest2gc2(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_rdlock_nest1alt2gc2(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() != 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_wrlock_once(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_wrlock_twice(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_oldtest_tr01(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_lock() < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() < 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_oldtest_tr02(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_thrd_lock() < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_thrd_unlock() < 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

void *tt_oldtest_tr03(size_t arg)
{
    (void)arg;
    if( randwait(), s2gc_gcop_lock(0) < 0 ) exit(EXIT_FAILURE);
    if( randwait(), s2gc_gcop_unlock(0) < 0 ) exit(EXIT_FAILURE);
    eprintf("%zd ", (size_t)arg);
    return NULL;
}

int main()
{
    pthread_t t1, t2, t3, t4, t5;
    int iters;

    urand_fd = open("/dev/urandom", O_RDONLY);
    if( urand_fd == -1 ) return EXIT_FAILURE;

    for(iters = 1; iters <= 50; iters++)
    {
        eprintf("%d-th test.\n", iters);
        eprintf("guard: %d, tcnt: %zd, waiting: %zd, pending: %zd.\n",
                gc_anch.stateguard, gc_anch.thr_count,
                gc_anch.gc_waiting, gc_anch.gc_pending);

        pthread_create(&t1, NULL, (thrd_entry)tt_oldtest_tr01, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_oldtest_tr02, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_oldtest_tr03, (void *)3);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        eprintf("case-old passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_once, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_rdlock_once, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_once, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_wrlock_once, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_once, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-01 passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_once, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_rdlock_nest2, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_nest1, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_wrlock_once, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_once, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-02 passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_once, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_rdlock_alt3, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_nest1, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_rdlock_nest1alt2, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_once, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-03 passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_once, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_rdlock_alt3, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_nest2gc1, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_wrlock_once, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_once, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-04 passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_alt3, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_rdlock_once, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_nest1alt2gc1a, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_rdlock_nest1alt2gc1b, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_twice, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-05 passed.\n");

        pthread_create(&t1, NULL, (thrd_entry)tt_rdlock_alt3, (void *)1);
        pthread_create(&t2, NULL, (thrd_entry)tt_wrlock_once, (void *)2);
        pthread_create(&t3, NULL, (thrd_entry)tt_rdlock_nest2gc2, (void *)3);
        pthread_create(&t4, NULL, (thrd_entry)tt_rdlock_once, (void *)4);
        pthread_create(&t5, NULL, (thrd_entry)tt_wrlock_twice, (void *)5);

        pthread_join(t1, NULL);
        pthread_join(t2, NULL);
        pthread_join(t3, NULL);
        pthread_join(t4, NULL);
        pthread_join(t5, NULL);
        eprintf("case-06 passed.\n");
        eprintf("guard: %d, tcnt: %zd, waiting: %zd, pending: %zd.\n",
                gc_anch.stateguard, gc_anch.thr_count,
                gc_anch.gc_waiting, gc_anch.gc_pending);
    }

    return EXIT_SUCCESS;
}
