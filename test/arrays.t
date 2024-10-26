include "include/test.t"

var c : [3]int;
var e : [3][3]float;
e[2][1] = 5.0;
var f = [3] new float;
var g = [3] new [3]float;
f[0] = 1234.0;
g[2][1] = 3.0;
Test.Expect(g[2][1] == 3.0);
Test.Expect(f[0] == 1234.0);
Test.Expect(e[2][1] == 5.0);
