include "include/test.t"

float[10] foo;
foo[9] = 4321.0;
Test.Expect(foo[9] == 4321.0);
return foo[9];
