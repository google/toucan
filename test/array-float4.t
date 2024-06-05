include "include/test.t"

int count = 200;
float<4>[256] a;
float<4> mul = float<4>(1.00001, 1.00001, 1.00001, 1.00001);
float<4> add = float<4>(1.0, 1.0, 1.0, 1.0);
for (int i = 0; i < a.length; ++i) {
 a[i] = float<4>(2000000.0, 2000000.0, 2000000.0, 2000000.0);
}
for (int j = 0; j < count; ++j) {
  for (int i = 0; i < a.length; ++i) {
    a[i] = a[i] * mul + add;
  }
}
Test.Expect(a[1].x == 2004203.875);
return a[1].x;
