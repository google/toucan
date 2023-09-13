int i;
float4 a;
float4 b;
float4 c;
a = float4(1.5, 0.5, -1.5, 5.0);
c = float4(1.0, 1.0,  1.0, 1.0);
for(i = 0; i < 100000000; ++i) {
  c = a * c;
}
return c.x;
