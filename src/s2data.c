/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#define safetypes2_implementing_data

#include "s2obj.h"
#include "s2data.h"

struct s2ctx_data {
    s2obj_t basetype;
    size_t len;
    void *buf;
    long mapcnt;
};

static void s2data_final(T *restrict ctx)
{
    // this can optionally be enabled:
    // memset(ctx->buf, 0, ctx->len);
    free(ctx->buf);
}

T *s2data_create(size_t len)
{
    T *ret = NULL;
    void *buf;

    // [2024-03-06-nul-term]:
    // put a nul byte at the end so that buffer
    // can be usable as nul-terminated string.
    if( !(buf = calloc(1, len+1)) )
        return NULL;

    ret = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_BLOB, sizeof(T));
    if( !ret )
    {
        free(buf);
        return NULL;
    }

    ret->basetype.itercreatf = NULL;
    ret->basetype.finalf = (s2func_final_t)s2data_final;

    ret->len = len;
    ret->buf = buf;
    ret->mapcnt = 0;
    return ret;
}

T *s2data_from_str(const char *s)
{
    size_t len;
    T *ret = s2data_create(len = strlen(s));
    if( !ret ) return NULL;

    strcpy(s2data_map(ret, 0, len), s);
    s2data_unmap(ret);
    return ret;
}

size_t s2data_len(T *restrict ctx)
{
    return ctx->len;
}

void *s2data_map(T *restrict ctx, size_t offset, size_t len)
{
    // 2024-03-26:
    // The first check *was*: ``offset >= ctx->len''.
    // This caused a bit of surprise for codes in the real world.
    if( offset       > ctx->len ) return NULL;
    if( offset + len > ctx->len ) return NULL;

    ctx->mapcnt++;
    assert( ctx->mapcnt > 0 );
    
    // <s>[2024-03-06-nul-term]</s>
    //
    // 2024-03-09:
    // ----
    //
    // Concurrent writes are one of the best known undefined race condition
    // behaviors, and setting the nul terminating byte when mapping the buffer
    // for multiple threads - even if the mappings are supposedly read-only,
    // is no exception.
    //
    // Setting the nul terminating byte during the map call was a safety
    // measure in case some application wrote it with some other value.
    // Considering however, client codes that relied on this behavior is
    // already faulty, SafeTypes2 developer(s) has therefore decided
    // that there's no point trading a threading UD for a memory UD.
    //
    //- ((uint8_t *)ctx->buf)[ctx->len] = '\0';

    return ((uint8_t *)ctx->buf) + offset;
}

int s2data_unmap(T *restrict ctx)
{
    assert( ctx->mapcnt > 0 );
    ctx->mapcnt--;
    return 0;
}

int s2data_trunc(T *restrict ctx, size_t len)
{
    // 2024-06-15:
    // Although error reporting through return values are preferred,
    // errno is chosen for this function, as one of the subroutines
    // it invokes already use errno to report errors.

    void *tmp;
    
    if( ctx->mapcnt > 0 )
    {
        errno = EBUSY;
        return -1;
    }

    // [2024-03-06-nul-term]:
    if( !(tmp = realloc(ctx->buf, len+1)) ) return -1;
    ((uint8_t *)ctx->buf)[len] = '\0';

    ctx->buf = tmp;
    ctx->len = len;
    return 0;
}

int s2data_cmp(T *restrict s1, T *restrict s2)
{
    size_t len1 = s1->len;
    size_t len2 = s2->len;
    size_t len = len1 < len2 ? len1 : len2;

    void *buf1 = s1->buf;
    void *buf2 = s2->buf;
    int ret = memcmp(buf1, buf2, len);

    if( !ret )
    {
        return len1 < len2 ? -1 : len1 == len2 ? 0 : 1;
    }
    else return ret;
}
