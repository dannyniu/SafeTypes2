/* DannyNiu/NJF, 2024-04-25. Public Domain. */

#ifndef SafeTypes2_Container_H
#define SafeTypes2_Container_H 1

enum s2_access_retvals {
    s2_access_error = -1,
    s2_access_nullval = 0,
    s2_access_success = 1,
};

enum s2_setter_semantics {
    s2_setter_kept = 1, // ``++ keptcnt''.
    s2_setter_gave = 2, // ``-- refcnt, ++keptcnt''.
};

#endif /* SafeTypes2_Container_H */
