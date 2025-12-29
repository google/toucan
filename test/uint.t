include "include/test.t"

var a : uint = 3;
var b : uint = 3000000000u;
Test.Expect(a < b);
var c : int = 3;
var d : int = 3000000000u as int;
Test.Expect(c > d);
