include "include/test.t"

int r = (((1 | 4) + (6 & 2)) ^ 15);
Test.Expect(r == 8);
return (float) r;
