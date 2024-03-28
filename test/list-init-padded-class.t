class C {
  float f;
  float<4> v;
}

C c = { 0.0, { 1.0, 0.0, 0.0, 0.0 } };

return c.v.x;
