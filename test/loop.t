int i;
float f;
float g;
f = 3.0;
g = 1.1;
for (i = 0; i < 10000; i = i + 1) {
  f = f * g;
}
return f;
