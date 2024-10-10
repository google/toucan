include "include/test.t"
var a = 0;
var i = 5;
do {
    var b = 1;
    var c = 2;
    a += c;
    --i;
} while (i > 0);
Test.Expect(i == 0);
Test.Expect(a == 10);
