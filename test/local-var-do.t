include "include/test.t"
int a = 0;
int i = 5;
do {
    int b = 1;
    int c = 2;
    a += c;
    --i;
} while (i > 0);
Test.Expect(i == 0);
Test.Expect(a == 10);
return (float) a;
