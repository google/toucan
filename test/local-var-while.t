include "include/test.t"

int a = 0;
int i = 5;
while (i > 0) {
    int b = 1;
    int c = 2;
    a += c;
    --i;
}
Test.Expect(i == 0);
Test.Expect(a == 10);
