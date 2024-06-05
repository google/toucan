include "include/test.t"

int i;
float f;
float g;
f = 3.0;
g = 1.1;
for (i = 0; i < 100; i = i + 1) {
  f = f * g;
}
Test.Expect(f == 41341.914062);
return f;
