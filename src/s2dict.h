/* DannyNiu/NJF, 2022-02-25. Public Domain. */

#ifndef SafeTypes2_Dict_H
#define SafeTypes2_Dict_H 1

/// @file
/// @brief The dict key-value mapping contaier type.

#include "s2obj.h"
#include "s2data.h"
#include "s2containers.h"

// Using SipHash-2-4-128.
#define S2_DICT_HASH_MAX 16

// 2024-07-27:
// Added type check macro.
//
/// @fn
/// @param obj the object handle the type of which is being checked.
/// @returns true if the object handle of dict type, false otherwise.
#define s2_is_dict(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_DICT)

/// @typedef s2dict_t
/// @brief
/// the working context for the dict type `s2dict_t`.
/// In the following prose, `s2dict_t` will be abbreviated as `T`.
#define T struct s2ctx_dict
typedef T s2dict_t;

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
    s2obj_base;
    struct s2ctx_dict_table root;
};

/// @fn
/// @brief Sets the randomization vector for hash tables globally.
///
/// @param in the randomization vector data.
/// @param inlen the length of the data.
void siphash_setkey(void const *restrict in, size_t inlen);

/// @fn
/// @brief Creates and returns an empty dictionary map.
/// @returns a pointer to the dict object handle, or NULL on error.
T *s2dict_create();

/// @fn
/// @param dict the dict from which a member is retrieved using a data key.
/// @param key the said data key.
/// @param out the pointer to the object handle where
///        the retrieved object will be stored.
///
/// @details
/// ''refcnt'' and ''keptcnt'' are not incremented, because
/// they're still available from the dict (i.e. not unset).
/// They have to be explicitly retained.
///
/// @returns
/// `s2_access_success` is returned when an object is successfully retrieved.
/// `s2_access_nullval` is returned on success, when the key is unset.
/// `s2_access_error` is returned on error.
/// See s2containers.h for further detail.
int s2dict_get(T *dict, s2data_t *key, s2obj_t **out);

/// @fn
/// @param dict the dict to which a value object will be set using a data key.
/// @param key the said key.
/// @param value the said value object.
/// @param semantic the setter semantic, see file s2containers.h.
///
/// @details
/// Sets a value object into `dict` at `key`.
/// If there exists a value at the key, it's first released by `s2obj_leave`.
///
/// If `semantic` is `s2_setter_kept`, then reference count stays the same
/// while kept count is incremented.
/// If `semantic` is `s2_setter_gave`, then reference count is decremented
/// while kept count is incremented.
/// See s2containers.h for further detail.
///
/// @returns
/// `s2_access_success` is returned when an object is successfully retrieved.
/// `s2_access_error` is returned on error.
/// See s2containers.h for further detail.
int s2dict_set(T *dict, s2data_t *key, s2obj_t *value, int semantic);

/// @fn
/// @brief unsets a key from a dict.
/// @param dict the dict from which a key will be unset.
/// @param key the key to unset.
///
/// @returns
/// `s2_access_success` is returned when an object is successfully retrieved.
/// `s2_access_error` is returned on error.
/// See s2containers.h for further detail.
int s2dict_unset(T *dict, s2data_t *key);

// Added 2024-10-09.
//
/// @fn
/// @param membertype the type of the object the handle would point to.
///
/// @details
/// The type-safe method to get a value object without type casting.
/// Invoked as: `s2dict_get_T(type)(dict, key, out)`.
/// Defined as a marcro in terms of `s2dict_get`.
#define s2dict_get_T(membertype) \
    ((int (*)(s2dict_t *dict, s2data_t *key, membertype **out))s2dict_get)

/// @typedef s2dict_iter_t
/// @brief
/// the dictionary iterator type that
/// enumerates members in arbitrary order.
typedef struct s2ctx_dict_iter s2dict_iter_t;

#ifndef safetypes2_implementing_dict
#undef T
#endif /* safetypes2_implementing_dict */

#endif /* SafeTypes_Dict_H */
