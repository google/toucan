int outer_count = 1000000;
float[] a = new float[1024];
for (int i = 0; i < a.length; ++i) {
  a[i] = 2000000.0;
}
for (int j = 0; j < outer_count; ++j) {
  for (int i = 0; i < a.length; ++i) {
    a[i] = a[i] * 1.00001 + 1.0;
  }
}
return a[0];
