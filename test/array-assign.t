include "include/test.t"

float[]* a = new float[10];
a[9] = -4321.0;
float[]* b = a;
Test.Expect(b[9] == -4321.0);
return b[9];
