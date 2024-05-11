class C {
  C(float _x, float _y) : { y = _x, x = _y } {}
  float x, y;
}

C c = C{21.0, 42.0};
return c.x - c.y;
