int i;
int a, b;
a = 0;
b = 2;
for(i = 0; i < 1000000; ++i) {
  a += b;
}
float f = 0.0;
while(a > 0) {
  f += 1.0;
  --a;
}
return f;
