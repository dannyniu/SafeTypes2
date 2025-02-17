/* DannyNiu/NJF, 2025-02-17. Public Domain. */

#include "../src/SafeTypes2.h"

#define get_T(membertype, x, ...) _Generic(     \
        x,                                      \
        s2dict_t *: s2dict_get_T(membertype),   \
        s2list_t *: s2list_get_T(membertype),   \
        default: NULL)(x, __VA_ARGS__)

int main()
{
    s2dict_t *dict = NULL;
    s2list_t *list = NULL;
    s2data_t *data = NULL;
    s2ref_t *out = NULL;

    bool a = get_T(s2ref_t, dict, data, &out) >= 0;
    bool b = get_T(s2ref_t, list, &out) >= 1;

    return a && b ? 0 : 1;
}

// 2025-02-17:
// Altough increasing compactness and expressiveness is desirable in
// some places, type safety is preferred to avoid mishaps such as
// calling a dict function on a list that may arise with uncaring typos,
// that're otherwise avoided when function (and type) names are
// spelled out in full. This is *SafeTypes2* after all.
