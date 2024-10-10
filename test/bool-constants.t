include "include/test.t"

var a : bool = true;
var b : bool = false;
Test.Expect(a != b);
Test.Expect(b != a);
Test.Expect(a == a);
Test.Expect(b == b);
