include "include/test.t"

float a = 1.0001;
float b = 2.7;
float c = 2.7;
for(int i = 0; i < 1000; i = i + 1) {
  c = a * b;
}
Test.Expect(b == 2.7);
return b;
