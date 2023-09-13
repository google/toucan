int i;
float3 a;
float3 b;
float3 c;
a = float3(1.5, 0.5, -1.5);
b = float3(2.5, -1.5, 0.5);
c = float3(0.0, 0.0,  0.0);
for(i = 0; i < 10000000; i = i + 1) {
  c = a * b;
}
return c.x;
