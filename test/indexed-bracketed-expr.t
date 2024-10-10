include "include/test.t"

var foo : float[10];
foo[9] = 4321.0;
Test.Expect(foo[9] == 4321.0);
