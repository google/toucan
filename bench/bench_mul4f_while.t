int i;
float4 a;
float4 b;
float4 c;
a = float4(1.5, 0.5, -1.5, 5.0);
c = float4(1.0, 1.0,  1.0, 1.0);
i = 0;
while(i < 100000000) {
  c *= a;
  ++i;
}
return c.x;
