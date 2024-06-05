include "include/test.t"

float a = 2.0;
float b;
float c;
b = 3.0;
c = a * b;
Test.Expect(c == 6.0);
return c;
