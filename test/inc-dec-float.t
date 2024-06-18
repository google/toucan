include "include/test.t"

float a = 3.0;
float b = 2.0;
float c = 0.0;
float d = 1.0;

Test.Expect(a++ == 3.0);
Test.Expect(--b == 1.0);
Test.Expect(++c == 1.0);
Test.Expect(d-- == 1.0);
