float4 a;
float4 b;
a = float4(1.0, 2.0, 3.0, 4.0);
b = float4(0.0, 0.0, 0.0, 0.0);
for (int i = 0; i < 1000000000; ++i) {
  b += a;
}
return b.w;
