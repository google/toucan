int i;
float a;
float b;
float c;
float<3> v;
a = 1.0001;
b = 2.7;
c = 2.7;
for(i = 0; i < 1000; i = i + 1) {
  c = a * b;
}
return b;
