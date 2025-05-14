include "test.t"

var n : *int = null;
var a = new int;
var b = new int;
var c = a;

Test.Assert(n == null);
Test.Assert(null == n);
Test.Assert(a != null);
Test.Assert(null != a);
Test.Assert(a != b);
Test.Assert(b != a);
Test.Assert(a == c);
Test.Assert(c == a);
Test.Assert(b != null);
Test.Assert(null != b);
