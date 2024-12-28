/* DannyNiu/NJF, 2024-04-25. Public Domain. */

#ifndef SafeTypes2_Container_H
#define SafeTypes2_Container_H 1

/// @file
/// @brief enumerations for use in operations involving container types.

/// @enum s2_access_retvals
/// @brief general return values from container access functions.
enum s2_access_retvals {
    s2_access_error = -1, ///< The container operation failed with error.
    s2_access_nullval = 0, ///< The contrainer getting succeeded empty-handed.
    s2_access_success = 1, ///< The container operation secceeded.
};

/// @enum s2_setter_semantics
/// @brief container setter semantics.
enum s2_setter_semantics {
    s2_setter_kept = 1, ///< ++ keptcnt.
    s2_setter_gave = 2, ///< -- refcnt, ++keptcnt.
};

#endif /* SafeTypes2_Container_H */
