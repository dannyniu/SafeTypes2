/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#ifndef SafeTypes2_Data_H
#define SafeTypes2_Data_H

#include "common.h"

// 2024-07-27:
// Added type check macro.
// Check type mask with "s2obj.h".
#define s2_is_data(obj) ((obj->type & 0x3000) == 0x0000)

#define T struct s2ctx_data
typedef T s2data_t;

#ifndef safetypes2_implementing_data
T {
    s2obj_t base;
    int payload_context[];
};
#endif /* safetypes2_implementing_data */

T *s2data_create(size_t len);
T *s2data_from_str(const char *s);
size_t s2data_len(T *restrict ctx);

// ``*_map'' does a simple range check - if it passes,
// a pointer at appropriate offset is returned, otherwise,
// a NULL pointer is returned.
// ``*_map'' doesn't record the regions requested - it only
// keeps a number, which is checked by ``*_trunc''.
//
// [2024-03-06-nul-term]:
// In the version suchly tagged, it is safe to pass the entire
// mapped pointer to functions that expect nul-terminated strings,
// as the implementation internally adds an extra nul byte
// just one byte beyond the effective length of the buffer.
void *s2data_map(T *restrict ctx, size_t offset, size_t len);

// ``*_unmap'' decreases what's increased by ``*_map''.
// The numbers of ``*_map'' and ``*_unmap'' should match.
// Currently there's no error defined, and 0 is returned.
int s2data_unmap(T *restrict ctx);

// If there's someone mapping the data (i.e. more ``*_map''
// than ``*_unmap''), then this function returns -1; it also
// returns -1 when the memory reallocation fails.
// Otherwise, the internal buffer is resized. Contrary to what
// the name suggests, expansion is also possible, and this is
// in fact a resize operation (it internally invokes ``realloc'').
int s2data_trunc(T *restrict ctx, size_t len);

// 2024-02-25: This is a new interface in SafeTypes2.
int s2data_cmp(T *restrict s1, T *restrict s2);

#ifndef safetypes2_implementing_data
#undef T
#endif /* safetypes2_implementing_data */

#endif /* SafeTypes2_Data_H */
