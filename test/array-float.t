include "include/test.t"

int outer_count = 200;
float[1024] a;
for (int i = 0; i < a.length; i++) {
  a[i] = 2000000.0;
}
for (int j = 0; j < outer_count; ++j) {
  for (int i = 0; i < a.length; ++i) {
    a[i] = a[i] * 1.00001 + 1.0;
  }
}

Test.Expect(a[0] == 2004203.875);
