
include "include/test.t"

var v : int<3>;
v.yz = v.xx;                   // Valid
v.xx = int<2>{4, 3};           // Error: duplicate indices on store
v.w = 0;                       // Error: invalid component
v.w;                           // Error: invalid component
v.q;                           // Error: invalid component
v.a;                           // Error: invalid component
v.xy = 3;                      // Error: rhs is not a vector
v.xy = int<3>();               // Error: rhs vector is wrong length
v.x = null;                    // Error: null is not widenable to int
