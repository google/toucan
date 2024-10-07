include "include/test.t"
float<4, 4> m;
int i0 = 0, i1 = 1, i2 = 2, i3 = 3;
m[i0][i0] = 4.0;
m[i0][i1] = 3.0;
m[i0][i2] = 2.0;
m[i0][i3] = 1.0;
m[i3][i0] = 5.0;
m[i3][i1] = 6.0;
m[i3][i2] = 7.0;
m[i3][i3] = 8.0;
Test.Expect(m[i0][i0] == 4.0);
Test.Expect(m[i0][i1] == 3.0);
Test.Expect(m[i0][i2] == 2.0);
Test.Expect(m[i0][i3] == 1.0);
Test.Expect(m[i3][i0] == 5.0);
Test.Expect(m[i3][i1] == 6.0);
Test.Expect(m[i3][i2] == 7.0);
Test.Expect(m[i3][i3] == 8.0);
