include "include/test.t"

bool a = true;
bool b = false;
Test.Expect(a != b);
Test.Expect(b != a);
Test.Expect(a == a);
Test.Expect(b == b);
