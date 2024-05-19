/* DannyNiu/NJF, 2023-06-30. Public Domain. */

#ifndef SafeTypes2_Common_H
#define SafeTypes2_Common_h 1

#ifndef __has_include
#define __has_include(...) 0
#endif /* __has_include */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#if INTERCEPT_MEM_CALLS
extern uint8_t shk[16];
extern uint32_t mh[4];
extern int allocs, frees;
void *mem_intercept(void *);
#define calloc(...) mem_intercept((allocs++, calloc(__VA_ARGS__)))
#define free(...) free((frees++, mem_intercept(__VA_ARGS__)))
#endif /* INTERCEPT_MEM_CALLS */

#endif /* SafeTypes2_Common_H */
