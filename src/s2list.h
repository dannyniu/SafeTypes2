/* DannyNiu/NJF, 2024-02-26. Public Domain. */

#ifndef SafeTypes2_List_H
#define SafeTypes2_List_H 1

#include "s2obj.h"
#include "s2containers.h"

#define T struct s2ctx_list
typedef T s2list_t;

T *s2list_create();

// - Inserts an element at the cursor without advancing it.
int s2list_insert(T *list, s2obj_t *obj, int semantic);

// - Inserts an element at the cursor and advance it.
int s2list_push(T *list, s2obj_t *obj, int semantic);

// for ``s2list_shift'' and ``s2list_pop'':
// ``refcnt'' is incremented and ``keptcnt'' decremented,
// both by 1 - because they're no longer in the list
// (at least not from where they used to be).

// - Removes the item at the cursor from the list and place it in ``*out''.
int s2list_shift(T *list, s2obj_t **out);

// - Removes the item just before the cursor and place it in ``*out''.
// - Implementation note: this function is found redundant and anti-logical,
//   adding the fact that it's implemented in terms of ``s2list_Shift'', this
//   function is probably inefficient and its use is better avoided.
int s2list_pop(T *list, s2obj_t **out);

// Retrieves object at current cursor position.
// Reference and kept counts're not changed.
int s2list_get(T *list, s2obj_t **out);

// 2024-03-26:
// Necessary, yet missing. Added.
ptrdiff_t s2list_pos(T *list);
ptrdiff_t s2list_len(T *list);

// Repositions the cursor.
#define S2_LIST_SEEK_SET 1
#define S2_LIST_SEEK_END 2
#define S2_LIST_SEEK_CUR 3
ptrdiff_t s2list_seek(T *list, ptrdiff_t offset, int whence);

typedef int (*s2func_sort_cmp_t)(s2obj_t *a, s2obj_t *b);
T *s2list_sort(T *list, s2func_sort_cmp_t cmpfunc);

typedef struct s2ctx_list_iter s2list_iter_t;

#ifndef safetypes2_implementing_list
#undef T
#endif /* safetypes2_implementing_list */

#endif /* SafeTypes2_Dict_H */
