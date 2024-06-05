include "include/test.t"
include "include/fiveclass.t"

Test.Expect(C.m() == 5.0);
return C.m() + 1.0;
