include "include/test.t"

var a = [10] new float;
a[9] = -4321.0;
var b = a;
Test.Expect(b[9] == -4321.0);
