include "include/test.t"

var i = 0;
if (false) var a = i++;
Test.Expect(i == 0);

var j = 0;
if (true) {} else var b = j++;
Test.Expect(j == 0);
