include "include/test.t"

float x = 3.0;
float y;
{
  float z = x * 2.0;
  y = z;
}
Test.Expect(y == 6.0);
