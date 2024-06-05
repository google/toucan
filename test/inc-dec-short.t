include "include/test.t"

short a = 3s;
short b = 2s;
short c = 0s;
short d = 1s;
Test.Expect((int) a++ == 3);
Test.Expect((int) --b == 1);
Test.Expect((int) ++c == 1);
Test.Expect((int) d-- == 1);
return (float) (a + b + c + d);
