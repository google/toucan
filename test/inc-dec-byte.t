include "include/test.t"

byte a = 3b;
byte b = 2b;
byte c = 0b;
byte d = 1b;
Test.Expect((int) a++ == 3);
Test.Expect((int) --b == 1);
Test.Expect((int) ++c == 1);
Test.Expect((int) d-- == 1);
return (float) (a + b + c + d);
