class C {
  C(float _x, float _y) : { x = _x + 5.0, y = _y + 2.0 } {}
  C(float _x) : C(_x, _x) {}
  float x, y;
}

C c = C(40.0);
return c.y;
