include "include/test.t"

short<2> a = short<2>(-3s, 32767s);
Test.Expect((int) a.x == -3);
Test.Expect((int) a.y == 32767);
return (float) (a.x + a.y);
