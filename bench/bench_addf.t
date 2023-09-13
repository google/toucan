int i;
float a;
float c;
a = 0.0000001;
c = 0.0;
for(i = 0; i < 1000000000; ++i) {
  c += a;
}
return c;
