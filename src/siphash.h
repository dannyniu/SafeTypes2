/* DannyNiu/NJF, 2023-07-15. Public Domain. */

#ifndef SafeTypes_SipHash_H
#define SafeTypes_SipHash_H 1

#include "common.h"

typedef struct {
    uint64_t v[4];
    uint64_t m;
    int64_t l;
    union {
        uint8_t b[16];
        uint64_t l[2];
    } out;
} siphash_t;

void *SipHash_o64_Init(
    siphash_t *restrict x,
    void const *restrict k,
    size_t klen);

void *SipHash_o128_Init(
    siphash_t *restrict x,
    void const *restrict k,
    size_t klen);

void SipHash_c2_Update(
    siphash_t *restrict x,
    void const *restrict data,
    size_t len);

void SipHash_c2d4o64_Final(
    siphash_t *restrict x,
    void *restrict out,
    size_t t);

void SipHash_c2d4o128_Final(
    siphash_t *restrict x,
    void *restrict out,
    size_t t);

#endif /* SafeTypes_SipHash_H */
