include "include/test.t"

var a : [3]int;
a = { 3, 2, 1 };
Test.Expect(a[0] == 3);
Test.Expect(a[1] == 2);
Test.Expect(a[2] == 1);
