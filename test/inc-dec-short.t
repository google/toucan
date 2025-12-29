include "include/test.t"

var a = 3s;
var b = 2s;
var c = 0s;
var d = 1s;
Test.Expect(a++ as int == 3);
Test.Expect(--b as int == 1);
Test.Expect(++c as int == 1);
Test.Expect(d-- as int == 1);
