int outer_count = 1000000;
float4[256] a;
float4 mul = float4(1.00001, 1.00001, 1.00001, 1.00001);
float4 add = float4(1.0, 1.0, 1.0, 1.0);
float4 init = float4(2000000.0, 2000000.0, 2000000.0, 2000000.0);
for (int i = 0; i < a.length; ++i) {
  a[i] = init;
}
for (int j = 0; j < outer_count; ++j) {
  for (int i = 0; i < a.length;) {
    a[i] = a[i] * mul + add;
    ++i;
    a[i] = a[i] * mul + add;
    ++i;
    a[i] = a[i] * mul + add;
    ++i;
    a[i] = a[i] * mul + add;
    ++i;
  }
}
return a[1].x;
