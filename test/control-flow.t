include "include/test.t"

int i;
float f;
i = 1;
if (i < 1) {
  f = -5.8;
} else {
  f = 3.2;
}
Test.Expect(f == 3.2);
return 3.2;
