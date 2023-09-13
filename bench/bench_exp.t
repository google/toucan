int i;
float a;
float c;
a = 1.0000001;
c = 1.0;
for(i = 0; i < 100000000; ++i) {
  c = a * c;
}
return c;
