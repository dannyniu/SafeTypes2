/* DannyNiu/NJF, 2024-02-26. Public Domain. */

#ifndef SafeTypes2_List_H
#define SafeTypes2_List_H 1

/// @file
/// @brief The list type.

#include "s2obj.h"
#include "s2containers.h"

// 2024-07-27:
// Added type check macro.
//
/// @fn
/// @param obj the object handle the type of which is being checked.
/// @returns true if the object handle of list type, false otherwise.
#define s2_is_list(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_LIST)

/// @typedef s2list_t
/// @brief the working context for the 'list' type `s2list_t`.
/// In the following prose, `s2list_t` will be abbreviated as `T`.
///
/// @details
/// As there's no silver bullet for a type to support both fast arbitrary
/// indexed access, and fast arbitrary slicing, the direction of the
/// design decision lean towards more primitive operations that
/// can readily compose into useful sequential operations.
///
/// The list consist of
/// - an ordered sequence of values derived from `s2obj_t`,
/// - a cursor positioned at one of the element on the sequence.
#define T struct s2ctx_list
typedef T s2list_t;

struct s2ctx_list_element {
    T *collection;
    s2obj_t *value;
    struct s2ctx_list_element *prev;
    struct s2ctx_list_element *next;
};

T {
    s2obj_base;
    size_t len;
    size_t pos;
    struct s2ctx_list_element *cursor;
    struct s2ctx_list_element anch_head;
    struct s2ctx_list_element anch_tail;
};

/// @fn
/// @brief Creates and returns an empty list.
/// @returns a pointer to the list object handle, or NULL on error.
T *s2list_create();

/// @fn
/// @brief Inserts an element at the cursor without advancing it.
/// @param list the list object handle.
/// @param obj the element to insert.
/// @param semantic the setter semantic to use (see s2containers.h).
/// @returns one of the access return values (see s2containers.h).
int s2list_insert(T *list, s2obj_t *obj, int semantic);

/// @fn
/// @brief Inserts an element at the cursor and advance it.
/// @param list the list object handle.
/// @param obj the element to insert.
/// @param semantic the setter semantic to use (see s2containers.h).
/// @returns one of the access return values (see s2containers.h).
int s2list_push(T *list, s2obj_t *obj, int semantic);

// for `s2list_shift` and `s2list_pop`:
// ''refcnt'' is incremented and ''keptcnt'' decremented,
// both by 1 - because they're no longer in the list
// (at least not from where they used to be).

// - Removes the item at the cursor from the list and place it in `*out`.
/// @fn
/// @brief Removes the item at the cursor from the list and place it in `out`.
/// @param list the list object handle.
/// @param out the pointer to the object handle where
///        the retrieved object will be stored.
/// @returns one of the access return values (see s2containers.h).
///
/// @details
/// This function decreases the kept count and increases
/// the reference count both by 1, as the retrieved object
/// is no longer in the list at where it was.
int s2list_shift(T *list, s2obj_t **out);

/// @fn
/// @brief Removes the item just before the cursor and place it in `*out`.
/// @param list the list object handle.
/// @param out the pointer to the object handle where
///        the retrieved object will be stored.
/// @returns one of the access return values (see s2containers.h).
///
/// @details
/// This function decreases the kept count and increases
/// the reference count both by 1, as the retrieved object
/// is no longer in the list at where it was.
///
/// Implementation note: this function is found redundant and anti-logical,
/// adding the fact that it's implemented in terms of `s2list_shift`, this
/// function is probably inefficient and its use is better avoided.
int s2list_pop(T *list, s2obj_t **out);

/// @fn
/// @brief Retrieves object at current cursor position.
/// @param list the list object handle.
/// @param out the pointer to the object handle where
///        the retrieved object will be stored.
///
/// @details
/// Reference and kept counts're not changed.
int s2list_get(T *list, s2obj_t **out);

// Added 2024-10-09.
//
/// @fn
/// @param membertype the type of the object the handle would point to.
///
/// @details
/// The type-safe method to get a value object without type casting.
/// Invoked as `s2list_shift_T(type)(list, out)`.
/// Defined as a marcro in terms of `s2list_shift`.
#define s2list_shift_T(membertype) \
    ((int (*)(s2list_t *list, membertype **out))s2list_shift)

// Added 2024-10-09.
//
/// @fn
/// @param membertype the type of the object the handle would point to.
///
/// @details
/// The type-safe method to get a value object without type casting.
/// Invoked as `s2list_pop_T(type)(list, out)`.
/// Defined as a marcro in terms of `s2list_pop`.
#define s2list_pop_T(membertype) \
    ((int (*)(s2list_t *list, membertype **out))s2list_pop)

// Added 2024-10-09.
//
/// @fn
/// @param membertype the type of the object the handle would point to.
///
/// @details
/// The type-safe method to get a value object without type casting.
/// Invoked as `s2list_get_T(type)(list, out)`.
/// Defined as a marcro in terms of `s2dict_get`.
#define s2list_get_T(membertype) \
    ((int (*)(s2list_t *list, membertype **out))s2list_get)

// 2024-03-26:
// 2 necessary, yet missing. Added.

/// @fn
/// @param list the list object handle.
/// @returns the current cursor position in the list.
ptrdiff_t s2list_pos(T *list);

/// @fn
/// @param list the list object handle.
/// @returns the length of the list.
ptrdiff_t s2list_len(T *list);

#define S2_LIST_SEEK_SET 1 ///< Seek relative to the beginning of the list
#define S2_LIST_SEEK_END 2 ///< Seek relative to the end of the list
#define S2_LIST_SEEK_CUR 3 ///< Displace the cursor on the list

/// @fn
/// @brief reposition the cursor on the list.
/// @param list the list object handle.
/// @param offset the offset to add relative to `whence`.
/// @param whence the anchor relative to which to offset
///        (see S2_LIST_SEEK_* constants).
///
/// @returns
/// The new cursor position in the list.
/// If any error occurs (e.g. out-of-bound position), -1 is returned.
ptrdiff_t s2list_seek(T *list, ptrdiff_t offset, int whence);

/// @typedef s2func_sort_cmp_t
/// @brief total order predicate.
/// @return less than 0 if a<b, 0 if a==b, and 1 if a>b.
/// @param a the first object.
/// @param b the second object.
///
/// @details
/// The implmentations of this function computes
/// the total order predicate of its operands.
typedef int (*s2func_sort_cmp_t)(s2obj_t *a, s2obj_t *b);

/// @fn
/// @brief insert sort of `list` with total order predicate `compfunc`.
/// @returns `list`.
T *s2list_sort(T *list, s2func_sort_cmp_t cmpfunc);

/// @typedef s2list_iter_t
/// @brief the list iterator type that enumerates members in order.
typedef struct s2ctx_list_iter s2list_iter_t;

#ifndef safetypes2_implementing_list
#undef T
#endif /* safetypes2_implementing_list */

#endif /* SafeTypes2_List_H */
