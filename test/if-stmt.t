include "include/test.t"

float f;
int i = 3;
if (i > 1) {
  f = 3.0;
} else {
  f = 4.0;
}
Test.Expect(f == 3.0);
return f;
