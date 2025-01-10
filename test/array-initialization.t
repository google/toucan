include "include/test.t"

var a = [3] new int;
var b = [3] new int{};
var c = [3] new int{42};
var d = [3] new int{3, 2, 1};

Test.Expect(a[0] == 0 && a[1] == 0 && a[2] == 0);
Test.Expect(b[0] == 0 && b[1] == 0 && b[2] == 0);
Test.Expect(c[0] == 42 && c[1] == 42 && c[2] == 42);
Test.Expect(d[0] == 3 && d[1] == 2 && d[2] == 1);

var e = [3] int();
var f = [3] int(42);
var g = [3] int(3, 2, 1);

Test.Expect(e[0] == 0 && e[1] == 0 && e[2] == 0);
Test.Expect(f[0] == 42 && f[1] == 42 && f[2] == 42);
Test.Expect(g[0] == 3 && g[1] == 2 && g[2] == 1);

var h : [3] int = {};
var i : [3] int = {42};
var j : [3] int = {3, 2, 1};

Test.Expect(h[0] == 0 && h[1] == 0 && h[2] == 0);
Test.Expect(i[0] == 42 && i[1] == 42 && i[2] == 42);
Test.Expect(j[0] == 3 && j[1] == 2 && j[2] == 1);
