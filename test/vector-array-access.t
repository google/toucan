include "include/test.t"
var v : float<4>;
var i0 = 0, i1 = 1, i2 = 2, i3 = 3;
v[i0] = 5.0;
v[i1] = 6.0;
v[i2] = 7.0;
v[i3] = 8.0;
Test.Expect(v[i0] == 5.0);
Test.Expect(v[i1] == 6.0);
Test.Expect(v[i2] == 7.0);
Test.Expect(v[i3] == 8.0);
