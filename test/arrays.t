include "include/test.t"

var c : int[3];
var e : float[3][3];
e[2][1] = 5.0;
var f = new float[3];
var g = new float[3][3];
f[0] = 1234.0;
g[2][1] = 3.0;
Test.Expect(g[2][1] == 3.0);
Test.Expect(f[0] == 1234.0);
Test.Expect(e[2][1] == 5.0);
