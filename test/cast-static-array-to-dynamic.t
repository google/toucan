include "include/test.t"

auto a = float[2](-2.0, 4.0);
auto b = &a;
Test.Expect(b[0] == -2.0);
Test.Expect(b[1] == 4.0);
