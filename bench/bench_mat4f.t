int i;
float4 m0;
float4 m1;
float4 m2;
float4 m3;
float4 v;
float x;
float y;
float z;
float w;
m0 = float4(1.0, 0.0, 0.0, 0.0);
m0 = float4(0.0, 1.0, 0.0, 0.0);
m0 = float4(0.0, 0.0, 1.0, 0.0);
m0 = float4(0.0, 0.0, 0.0, 1.0);
v = float4(1.0, 2.0, 3.0, -4.0);
for(i = 0; i < 10000000; i = i + 1) {
  x = dot(m0, v);
  y = dot(m1, v);
  z = dot(m2, v);
  w = dot(m3, v);
}
x;
y;
z;
w;
return x;
