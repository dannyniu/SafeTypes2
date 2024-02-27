/* DannyNiu/NJF, 2023-07-15. Public Domain. */

#include "siphash.h"

#if __has_include(<endian.h>)
#include <endian.h>
#else
#include "fallback/endian.c"
#endif /* __has_include(<endian.h>) */

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define SIPROUND                                \
    do {                                        \
        v0 += v1;                               \
        v1 = ROTL(v1, 13);                      \
        v1 ^= v0;                               \
        v0 = ROTL(v0, 32);                      \
        v2 += v3;                               \
        v3 = ROTL(v3, 16);                      \
        v3 ^= v2;                               \
        v0 += v3;                               \
        v3 = ROTL(v3, 21);                      \
        v3 ^= v0;                               \
        v2 += v1;                               \
        v1 = ROTL(v1, 17);                      \
        v1 ^= v2;                               \
        v2 = ROTL(v2, 32);                      \
    } while (0)

#define v0 (x->v[0])
#define v1 (x->v[1])
#define v2 (x->v[2])
#define v3 (x->v[3])

void *SipHash_o64_Init(
    siphash_t *restrict x,
    void const *restrict k,
    size_t klen)
{
    uint64_t k0, k1;
    
    if( klen != 16 ) return NULL;

    k0 = le64toh( ((uint64_t const *)k)[0] );
    k1 = le64toh( ((uint64_t const *)k)[1] );
    
    x->v[0] = UINT64_C(0x736f6d6570736575) ^ k0;
    x->v[1] = UINT64_C(0x646f72616e646f6d) ^ k1;
    x->v[2] = UINT64_C(0x6c7967656e657261) ^ k0;
    x->v[3] = UINT64_C(0x7465646279746573) ^ k1;

    x->m = 0;
    x->l = 0;

    return x;
}

void *SipHash_o128_Init(
    siphash_t *restrict x,
    void const *restrict k,
    size_t klen)
{
    x = SipHash_o64_Init(x, k, klen);
    
    if( !x ) return NULL;
    
    x->v[1] ^= 0xee;
    
    return x;
}

void SipHash_c2_Update(
    siphash_t *restrict x,
    void const *restrict data,
    size_t len)
{
    uint8_t const *ptr = data;
    
    while( len )
    {
        x->m ^= // might as well be ``|='' or ``+=''.
            (uint64_t)*ptr++ << ((x->l & 7) << 3);

        ++ x->l;
        -- len;
        
        if( (x->l & 07) == 0 )
        {
            v3 ^= x->m;
            SIPROUND;
            SIPROUND;
            v0 ^= x->m;
            x->m = 0;
        }
    }
}

void SipHash_c2d4o64_Final(
    siphash_t *restrict x,
    void *restrict out,
    size_t t)
{
    if( x->l != -1 )
    {
        x->m ^= // same as above.
            x->l << 56;

        v3 ^= x->m;
        SIPROUND;
        SIPROUND;
        v0 ^= x->m;

        v2 ^= 0xff;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        x->out.l[0] = htole64(v0 ^ v1 ^ v2 ^ v3);
        
        x->l = -1;
    }

    if( out )
    {
        size_t i;
        uint8_t *ptr = out;
        
        for(i=0; i<t; i++)
        {
            ptr[i] = i < 8 ? x->out.b[i] : 0;
        }
    }
}

void SipHash_c2d4o128_Final(
    siphash_t *restrict x,
    void *restrict out,
    size_t t)
{
    if( x->l != -1 )
    {
        x->m ^= // same as above.
            x->l << 56;

        v3 ^= x->m;
        SIPROUND;
        SIPROUND;
        v0 ^= x->m;

        v2 ^= 0xee;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        x->out.l[0] = htole64(v0 ^ v1 ^ v2 ^ v3);
    
        v1 ^= 0xdd;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        SIPROUND;
        x->out.l[1] = htole64(v0 ^ v1 ^ v2 ^ v3);
        
        x->l = -1;
    }
    
    if( out )
    {
        size_t i;
        uint8_t *ptr = out;
        
        for(i=0; i<t; i++)
        {
            ptr[i] = i < 16 ? x->out.b[i] : 0;
        }
    }
}
