class C {
  int i;
  float a = 42.0;
};

class D : C {
}

D d = { i = 3, a = 5.0 };
return d.a + (float) d.i;