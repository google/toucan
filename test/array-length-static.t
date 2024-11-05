include "include/test.t"

var f : [5]float;
Test.Expect(f.length == 5);

var pf = &f;
Test.Expect(pf.length == 5);
