/* DannyNiu/NJF, 2024-02-20. Public Domain. */

#ifndef SafeTypes2_Data_H
#define SafeTypes2_Data_H

/// @file
/// @brief The 'data' type for holding arbitrary data.

#include "s2obj.h"

// 2024-07-27:
// Added type check macro.
// Check type mask with "s2obj.h".
//
/// @fn
/// @param obj the object handle the type of which is being checked.
/// @returns true if the object handle of 'data' type, false otherwise.
#define s2_is_data(obj) ((((s2obj_t *)obj)->type & 0x3000) == 0x0000)

/// @typedef s2data_t
/// @brief the working context for the 'data' type `s2data_t`.
/// In the following prose, `s2data_t` will be abbreviated as `T`.
///
/// @details
/// The design goal has been to make it easy to access datum within
/// the address range of the underlaying buffer, and to safely
/// resize the buffer when needed.
#define T struct s2ctx_data
typedef T s2data_t;

#define DATA_INLINE_MAX 47

struct s2ctx_data {
    s2obj_base;
    size_t len;
    void *ptr;
    unsigned long mapcnt;
    unsigned long pushed;
    alignas(max_align_t) uint8_t buf[DATA_INLINE_MAX+1];
};

/// @fn
/// @param len the initial length of the created 'data' handle.
/// @returns the `s2data_t *` object handle. NULL is returned on failure.
T *s2data_create(size_t len);

/// @fn
/// @param s a nul-terminated string.
/// @returns the `s2data_t *` object handle. NULL is returned on failure.
T *s2data_from_str(const char *s);

/// @fn
/// @param ctx the handle to the 'data' object whose length is queried.
/// @returns the length in bytes of the 'data' object.
size_t s2data_len(T *restrict ctx);

/// @fn
/// @param ctx the handle to the 'data' object where a map is requested.
/// @param offset the offset from the beginning of the buffer of the request.
/// @param len the requested length of the map.
/// @returns a pointer to the requested map range.
///
/// @details
/// The function does a simple range check - if it passes,
/// a pointer at appropriate offset is returned, otherwise,
/// a NULL pointer is returned.
///
/// The function doesn't record the regions requested - it only
/// keeps a number, which is checked by `s2data_trunc`.
///
/// [2024-03-06-nul-term]:
/// In the version suchly labelled, it is safe to pass the entire
/// mapped pointer to functions that expect nul-terminated strings,
/// as the implementation internally adds an extra nul byte
/// just one byte beyond the effective length of the buffer.
///
/// @note
/// The implementation allocates a buffer in addition to
/// the `s2data_t` context working data structure to hold
/// the actual data. In versions after 2024-09-11, when the
/// requested buffer size is small (less than or equal to 19 bytes),
/// the data is held "inline" in the context working data structure
/// to save the additional allocation of the buffer to save heap space,
/// and to accelerate processing of small datum.
void *s2data_map(T *restrict ctx, size_t offset, size_t len);

/// @fn
/// @param ctx the handle to the 'data' object where a map was requested.
/// @returns 0 on success, see below.
///
/// @details
/// The function decreases what's increased by `s2data_map`.
/// The numbers of `s2data_map` and `s2data_unmap` should match.
/// Currently there's no error defined, and 0 is returned.
int s2data_unmap(T *restrict ctx);

/// @fn
/// @param ctx the handle to the 'data' object.
///
/// @details
/// Added 2025-01-04.
/// Request a transient map from the beginning of the 'data' object.
/// The caller shall ensure no resize or simultaneous modification occur,
/// as otherwise will result in undefined behavior.
void *s2data_weakmap(T *restrict ctx);

/// @fn
/// @param ctx the handle to the 'data' object to which the length is changed.
/// @param len len the requested new length of the 'data' object.
/// @returns 0 on success, and -1 on error.
///
/// @details
/// If there's someone mapping the data (i.e. more `s2data_map`
/// than `s2data_unmap`), then this function returns -1; it also
/// returns -1 when the memory reallocation fails.
/// Otherwise, the internal buffer is resized. Contrary to what
/// the name suggests, expansion is also possible, and this is
/// in fact a resize operation (it internally invokes `realloc`).
int s2data_trunc(T *restrict ctx, size_t len);

// 2024-02-25: This is a new interface in SafeTypes2.
/// @fn
/// @brief byte-collation total order,
///
/// @param s1 the first 'data' object.
/// @param s2 the second 'data' object.
///
/// @return
/// -1 if `s1` is ordered before `s2`, 1 if after,
/// and 0 if they're same. if one of them is
/// a prefix of the other, the shorter one
/// orders before the longer one.
int s2data_cmp(T *restrict s1, T *restrict s2);

/// @fn
/// @brief adds a byte to the accumulation staging area.
/// @param ctx the 'data' object handle to put the byte.
/// @param c the value of the byte to put.
/// @returns 0 on success and -1 on failure.
int s2data_putc(T *restrict ctx, int c);

/// @fn
/// @brief adds a string of bytes to the accumulation staging area.
/// @param ctx the 'data' object handle to put the bytes.
/// @param str the byte string to put.
/// @param len the number of bytes in the string.
/// @returns 0 on success and -1 on failure.
/// @details
/// The function is defined in terms of `s2data_putc`, this means that
/// the user don't have to call `s2data_putfin` *before* calling this
/// function, but does have to call it *afterwards*. This function has
/// no atomicity guarantee - if error occur during putting, the actual
/// number of bytes put may be less than `len`.
int s2data_puts(T *restrict ctx, void const *str, size_t len);

/// @fn
/// @brief flush bytes in the staging area into the internal buffer.
/// @param ctx the 'data' object handle to flush.
/// @returns 0 on success and -1 on failure.
int s2data_putfin(T *restrict ctx);

#ifndef safetypes2_implementing_data
#undef T
#endif /* safetypes2_implementing_data */

#endif /* SafeTypes2_Data_H */
