class C {
  C(float _x, float _y) {
    x = _x;
    y = _y;
  }
  float x;
  float y;
}

C c = C(21.0, 42.0);
return c.x + c.y;
