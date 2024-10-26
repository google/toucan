include "include/test.t"

var foo : [10]float;
foo[9] = 4321.0;
Test.Expect(foo[9] == 4321.0);
