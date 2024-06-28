/* DannyNiu/NJF, 2024-06-27. Public Domain. */

#ifndef SafeTypes2_Terse_Idioms_h
#define SafeTypes2_Terse_Idioms_h 1

#include "SafeTypes2.h"

// 2024-06-28:
// ==========
//
// The "i" stand for "implicit" operand, "improve".
//
// While it was intended as an improvement, advice were received on
// StackOverflow saying that this could lead to fragmentation of the use of API
// and that the supposed benefit offered by it is trivial that of the original
// API, especially taken into account when the tooling used by developer
// may not be capable of parsing a "made-up" syntax such as that from this
// header.
//
// What's more, idioms envisioned for this header - i.e. Rust-like unwrap chain
// require error and type checking. For example,
// the fragment ``dict[key].array_shift().map()'' require an implementation of
// the shift function to check for list type, and a map that checks for
// data blob type. Since none of these were ever planned (as their benefit and
// drawbacks are far from known), the implementation of this idiom is
// put on hold indefinitely, and this branch should be considered abandoned.
//
// THE USE OF THIS HEADER AND ITS INTERFACE IS AT YOUR OWN RISK.

typedef s2obj_t *obj;

#define s2i_decl obj _
#define s2i_bind(arg) _ = (obj)(arg)

#define s2i_length()   s2data_len((s2data_t *)_)
#define s2i_map()      s2data_map((s2data_t *)_, 0, 0)
#define s2i_unmap()    s2data_unmap((s2data_t *)_)
#define s2i_trunc(len) s2data_trunc((s2data_t *)_, (len))

#define s2i_kget(k, out) s2dict_get((s2dict_t *)_, (k), (out))
#define s2i_kset(k, val) s2dict_set((s2dict_t *)_, (k), (val), s2_setter_kept)
#define s2i_unset(k)     s2dict_unset((s2dict_t *)_, (k))

#define s2i_insert(arg) s2list_insert((s2list_t *)_, (arg), s2_setter_kept)
#define s2i_push(arg)   s2list_push  ((s2list_t *)_, (arg), s2_setter_kept)
#define s2i_shift(arg)  s2list_shift ((s2list_t *)_, (arg))
#define s2i_pop(arg)    s2list_pop   ((s2list_t *)_, (arg))
#define s2i_pget(arg)   s2list_get   ((s2list_t *)_, (arg))

#define s2i_pos() s2list_pos((s2list_t *)_)
#define s2i_len() s2list_len((s2list_t *)_)
#define s2i_seek(offset, whence) s2list_seek((s2list_t *)_, (offset), (whence))
#define s2i_sort(cmpfunc) s2list_sort((s2list_t *)_, (cmpfunc))

#endif /* SafeTypes2_Terse_Idioms_h */
