include "include/test.t"

uint a = 3;
uint b = 3000000000u;
Test.Expect(a < b);
int c = 3;
int d = (int) 3000000000u;
Test.Expect(c > d);
