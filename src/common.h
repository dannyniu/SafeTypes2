/* DannyNiu/NJF, 2023-06-30. Public Domain. */

#ifndef SafeTypes2_Common_H
#define SafeTypes2_Common_h 1

#ifndef __has_include
#define __has_include(...) 0
#endif /* __has_include */

#include <assert.h>
#include <errno.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#if INTERCEPT_MEM_CALLS
extern uint8_t shk[16];
extern uint32_t mh[4];
extern long allocs, frees;
extern bool trace;
void *mem_intercept(void *);

#define trace_intercept(addr)                           \
    (trace ? printf("minterc: %s, %s, %d.\n",           \
                    __func__, __FILE__, __LINE__) : 0,  \
     mem_intercept(addr))

// prefer `calloc` over `malloc`, avoid the latter at all effort.
#define malloc(...) trace_intercept((allocs++, malloc(__VA_ARGS__)))
#define calloc(...) trace_intercept((allocs++, calloc(__VA_ARGS__)))

#define realloc(ptr, sz) trace_intercept(                       \
        (allocs++, frees++, realloc(trace_intercept(ptr), sz)))

#define free(...) free((frees++, trace_intercept(__VA_ARGS__)))

// C23 additions:
#define aligned_alloc(a, s)                             \
    trace_intercept((allocs++, aligned_alloc(a, s)))

#define free_sized(p, s)                                \
    free_sized((frees++, trace_intercept(p)), s)

#define free_aligned_sized(p, a, s)             \
    free_aligned_sized((frees++, trace_intercept(p)), a, s)

#endif /* INTERCEPT_MEM_CALLS */

#endif /* SafeTypes2_Common_H */
