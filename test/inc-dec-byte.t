include "include/test.t"

var a = 3b;
var b = 2b;
var c = 0b;
var d = 1b;
Test.Expect(a++ as int == 3);
Test.Expect(--b as int == 1);
Test.Expect(++c as int == 1);
Test.Expect(d-- as int == 1);
