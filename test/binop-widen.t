include "include/test.t"

uint a = 3u + (uint) -1;
Test.Expect(a == 2);
