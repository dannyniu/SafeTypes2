/* DannyNiu/NJF, 2022-02-25. Public Domain. */

#ifndef SafeTypes2_Dict_H
#define SafeTypes2_Dict_H 1

#include "s2obj.h"
#include "s2data.h"
#include "s2containers.h"

// Using SipHash-2-4-128.
#define S2_DICT_HASH_MAX 16

// 2024-07-27:
// Added type check macro.
#define s2_is_dict(obj) (obj->type == S2_OBJ_TYPE_DICT)

#define T struct s2ctx_dict
typedef T s2dict_t;

#ifndef safetypes2_implementing_dict
T {
    s2obj_t base;
    int payload_context[];
};
#endif /* safetypes2_implementing_dict */

void siphash_setkey(void const *restrict in, size_t inlen);

T *s2dict_create();

// ``refcnt'' and ``keptcnt'' are not incremented, because
// they're still available from the dict (i.e. not unset).
// They have to be explicitly retained.
int s2dict_get(T *dict, s2data_t *key, s2obj_t **out);
int s2dict_set(T *dict, s2data_t *key, s2obj_t *value, int semantic);
int s2dict_unset(T *dict, s2data_t *key);

// enumerates members in arbitrary order.
typedef struct s2ctx_dict_iter s2dict_iter_t;

#ifndef safetypes2_implementing_dict
#undef T
#endif /* safetypes2_implementing_dict */

#endif /* SafeTypes_Dict_H */
