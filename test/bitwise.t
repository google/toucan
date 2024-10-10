include "include/test.t"

var r : int = (((1 | 4) + (6 & 2)) ^ 15);
Test.Expect(r == 8);
