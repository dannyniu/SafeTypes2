/* DannyNiu/NJF, 2024-02-25. Public Domain. */

#define safetypes2_implementing_dict

#include "s2dict.h"
#include "siphash.h"

// 2024-06-15:
// The "TODO: errno" comments are altogether removed.
// According to Single Unix Specification, Rationales
// Volume, System Interfaces General Information, Error
// Numbers, it is more preferable that new functions
// report errors through return values, and this is
// the approach taken in SafeTypes.

enum s2_dict_member_flags {
    s2_dict_member_null = 0, // unset.
    s2_dict_member_set = 1, // kept-counted reference.
    s2_dict_member_collision = 2, // collision in hashed key.
};

struct s2ctx_dict_member {
    int flags;
    s2dict_t *collection;
    s2data_t *key;
    union {
        s2obj_t *value;
        struct s2ctx_dict_table *nested;
    };
};

struct s2ctx_dict_table {
    int level; // 0 at root.
    struct s2ctx_dict_member members[256];
};

T {
    s2obj_t basetype;
    int iterlevel;
    int iterpos[S2_DICT_HASH_MAX];
    struct s2ctx_dict_table root;
};

static_assert( S2_DICT_HASH_MAX == 16,
               "Code changed too radically, cannot compile!");
static uint8_t key_siphash[S2_DICT_HASH_MAX];

void siphash_setkey(void const *in, size_t inlen)
{
    // implicitly truncates or zero-extends to 16 bytes.
    size_t t = inlen < sizeof(key_siphash) ? inlen : sizeof(key_siphash);

    memset(key_siphash, 0, sizeof(key_siphash));
    memcpy(key_siphash, in, t);
}

static void s2dict_free_table(struct s2ctx_dict_table *table);
static void s2dict_free_member(struct s2ctx_dict_member *m)
{
    switch( m->flags ){
    case s2_dict_member_null:
        break;

    case s2_dict_member_set:
        s2obj_release((s2obj_t *)m->key);
        s2obj_leave((s2obj_t *)m->value);
        break;

    case s2_dict_member_collision:
        s2dict_free_table(m->nested);
        break;

    default:
        break;
    }
}

static void s2dict_free_table(struct s2ctx_dict_table *table)
{
    int i;

    for(i=0; i<256; i++)
    {
        s2dict_free_member(table->members + i);
    }

    free(table);
}

static void s2dict_final(T *dict)
{
    int i;

    for(i=0; i<256; i++)
    {
        s2dict_free_member(dict->root.members + i);
    }
}

struct s2ctx_dict_iter {
    struct s2ctx_iter basetype;
    int iterlevel;
    int iterpos[S2_DICT_HASH_MAX];
    s2dict_t *dict;
};

static int s2dict_iter_step(s2dict_iter_t *restrict iter);
static s2dict_iter_t *s2dict_iter_create(T *restrict dict)
{
    s2dict_iter_t *iter = NULL;
    int i;

    iter = calloc(1, sizeof(s2dict_iter_t));
    if( !iter ) return NULL;

    iter->basetype.final = (s2iter_final_func_t)free;
    iter->basetype.next = (s2iter_stepfunc_t)s2dict_iter_step;
    iter->iterlevel = 0;
    iter->dict = dict;

    for(i=0; i<S2_DICT_HASH_MAX; i++)
        dict->iterpos[i] = 0;

    return iter;
}

T *s2dict_create()
{
    int i;
    T *dict;

    dict = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_DICT, sizeof(T));
    if( !dict ) return NULL;

    dict->basetype.itercreatf = (s2func_iter_create_t)s2dict_iter_create;
    dict->basetype.finalf = (s2func_final_t)s2dict_final;

    dict->root.level = 0;

    for(i=0; i<256; i++)
    {
        dict->root.members[i].flags = s2_dict_member_null;
        dict->root.members[i].collection = dict;
        dict->root.members[i].key = NULL;
        dict->root.members[i].value = NULL;
    }

    for(i=0; i<S2_DICT_HASH_MAX; i++)
    {
        dict->iterpos[i] = 0;
    }

    return dict;
}

int s2dict_get(T *dict, s2data_t *key, s2obj_t **out)
{
    size_t klen = s2data_len(key);
    uint8_t hash[S2_DICT_HASH_MAX];
    siphash_t hx;
    int level = 0;

    struct s2ctx_dict_table *V;
    struct s2ctx_dict_member *M;

    SipHash_o128_Init(&hx, key_siphash, sizeof(key_siphash));
    SipHash_c2_Update(&hx, s2data_map(key, 0, klen), klen);
    SipHash_c2d4o128_Final(&hx, hash, S2_DICT_HASH_MAX);
    s2data_unmap(key);

    V = &dict->root;
    while( true )
    {
        M = V->members + hash[level];

        switch( M->flags ){
        case s2_dict_member_null:
            *out = NULL;
            return s2_access_nullval;
            break;

        case s2_dict_member_set:
            if( !s2data_cmp(key, M->key) )
            {
                *out = M->value;
                return s2_access_success;
            }
            else
            {
                *out = NULL;
                return s2_access_nullval;
            }
            break;

        case s2_dict_member_collision:
            if( ++level < S2_DICT_HASH_MAX ) V = M->nested; else
            {
                *out = NULL;
                return s2_access_error;
            }
            break;


        default:
            *out = NULL;
            return s2_access_error;
            break;
        }
    }
}

// Unset is logically similar to Get. Implement it first.
int s2dict_unset(T *dict, s2data_t *key)
{
    size_t klen = s2data_len(key);
    uint8_t hash[S2_DICT_HASH_MAX];
    siphash_t hx;
    int level = 0;

    struct s2ctx_dict_table *V;
    struct s2ctx_dict_member *M;

    SipHash_o128_Init(&hx, key_siphash, sizeof(key_siphash));
    SipHash_c2_Update(&hx, s2data_map(key, 0, klen), klen);
    SipHash_c2d4o128_Final(&hx, hash, S2_DICT_HASH_MAX);
    s2data_unmap(key);

    V = &dict->root;
    while( true )
    {
        M = V->members + hash[level];

        switch( M->flags ){
        case s2_dict_member_null:
            return s2_access_nullval;
            break;

        case s2_dict_member_set:
            if( s2data_cmp(key, M->key) )
                return s2_access_nullval;

            s2obj_leave((s2obj_t *)M->value);
            s2obj_release((s2obj_t *)M->key);
            M->flags = s2_dict_member_null;
            M->value = NULL;
            M->key = NULL;
            return s2_access_success;
            break;

        case s2_dict_member_collision:
            if( ++level < S2_DICT_HASH_MAX ) V = M->nested; else
            {
                return s2_access_error;
            }
            break;


        default:
            return s2_access_error;
            break;
        }
    }
}

int s2dict_set(T *dict, s2data_t *key, s2obj_t *value, int semantic)
{
    size_t klen = s2data_len(key);
    uint8_t hash[S2_DICT_HASH_MAX], h2[S2_DICT_HASH_MAX];
    int level = 0;

    siphash_t hx;

    struct s2ctx_dict_table *U;
    struct s2ctx_dict_table *V;
    struct s2ctx_dict_member *M;
    s2obj_t *tmp = NULL;

    assert( semantic == s2_setter_kept ||
            semantic == s2_setter_gave );

    SipHash_o128_Init(&hx, key_siphash, sizeof(key_siphash));
    SipHash_c2_Update(&hx, s2data_map(key, 0, klen), klen);
    SipHash_c2d4o128_Final(&hx, hash, S2_DICT_HASH_MAX);
    s2data_unmap(key);

    V = &dict->root;
    while( level >= 0 )
    {
        M = V->members + hash[level];

        switch( M->flags ){
        case s2_dict_member_null:
            level = -1;
            break;

        case s2_dict_member_set:
            if( s2data_cmp(key, M->key) == 0 )
                goto replace_value_prepare;

            // hash collision. do the following:
            // 1. increment `level`.
            // 2. calculate h2 from the key of M if it hasn't been done.
            // 3. create subtable U.
            // 4. move M to U.
            // 5. attach U to V.
            // 6. test for collision:
            // 6.1. if collision, restart with U as V.
            // 6.2. otherwise, add `value` to U, and done.

            U = NULL;

            while( true )
            {
                if( ++level >= S2_DICT_HASH_MAX )
                {
                    return s2_access_error;
                    /* NOTREACHED */
                }

                if( !U )
                {
                    size_t M_klen = s2data_len(M->key);

                    SipHash_o128_Init(
                        &hx, key_siphash, sizeof(key_siphash));

                    SipHash_c2_Update(
                        &hx, s2data_map(M->key, 0, M_klen), M_klen);

                    SipHash_c2d4o128_Final(
                        &hx, h2, S2_DICT_HASH_MAX);

                    s2data_unmap(M->key);
                }

                U = calloc(1, sizeof(struct s2ctx_dict_table));
                if( !U )
                {
                    return s2_access_error;
                }
                U->level = level;

                memcpy(U->members + h2[level], M,
                       sizeof(struct s2ctx_dict_member));

                M->flags = s2_dict_member_collision;
                M->nested = U;
                M->key = NULL;

                V = U;
                M = V->members + hash[level];

                if( hash[level] == h2[level] )
                {
                    continue;
                }
                else break;
            }

        replace_value_prepare:
            level = -1;
            break;

        case s2_dict_member_collision:
            if( ++level < S2_DICT_HASH_MAX ) V = M->nested; else
            {
                return s2_access_error;
            }
            break;

        default:
            return s2_access_error;
            break;
        }
    }

    assert(M);

    if( M->flags == s2_dict_member_set )
        tmp = M->value;

    M->flags = s2_dict_member_set;
    M->collection = dict;
    if( !M->key )
    {
        M->key = s2data_create(klen);
        assert( M->key ); // 2024-02-25: caller may have no way of recovering

        memcpy(
            s2data_map(M->key, 0, klen),
            s2data_map(key, 0, klen), klen);
        s2data_unmap(M->key);
        s2data_unmap(key);
    }
    M->value = value;

    switch( semantic ){
    case s2_setter_kept:
        s2obj_keep(value);
        break;

    case s2_setter_gave:
        s2obj_keep(value);
        s2obj_release(value);
        break;
    }

    if( tmp ) s2obj_leave(tmp);

    return s2_access_success;
}

static int s2dict_iter_step(s2dict_iter_t *iter)
{
    int i, level;
    struct s2ctx_dict_table *V;

descend_in:
    level = iter->iterlevel;
    V = &iter->dict->root;
    for(i=0; i<level; i++)
    {
        V = V->members[iter->iterpos[i]].nested;
    }

dive_in:
    i = iter->iterpos[level];

    if( i >= 256 )
    {
        iter->iterpos[level] = 0;
        if( level -- == 0 )
        {
            iter->iterlevel = 0;
            for(i=0; i<S2_DICT_HASH_MAX; i++)
            {
                iter->iterpos[i] = 0;
            }
            return 0;
        }
        else
        {
            iter->iterlevel = level;
            iter->iterpos[level] ++;
            goto descend_in; // simple method of seeking for `V`.
        }
    }

    if( V->members[i].flags == s2_dict_member_collision )
    {
        V = V->members[i].nested;
        ++ level;
        assert( level < S2_DICT_HASH_MAX );

        iter->iterlevel = level;
        iter->iterpos[level] = 0;
        goto dive_in;
    }
    else if( V->members[i].flags == s2_dict_member_null )
    {
        iter->iterpos[level] ++;
        goto dive_in;
    }
    else if( V->members[i].flags == s2_dict_member_set )
    {
        iter->basetype.value = V->members[i].value;
        iter->basetype.key   = V->members[i].key;
        iter->iterpos[level] ++;
        return 1;
    }

    // supposedly unreachable.
    return -1;
}
