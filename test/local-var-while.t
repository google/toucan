include "include/test.t"

var a = 0;
var i = 5;
while (i > 0) {
    var b = 1;
    var c = 2;
    a += c;
    --i;
}
Test.Expect(i == 0);
Test.Expect(a == 10);
