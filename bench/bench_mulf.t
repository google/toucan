float a = 1.3, b = 2.7;
float c = 0.0;
for(int i = 0; i < 10000000; ++i) {
  a += b;
}
return a;
