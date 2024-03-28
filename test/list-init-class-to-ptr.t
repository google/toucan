class C {
  void clobber(C^ c)
  {
    f = c.f;
  }
  float f;
}

C c;
c.clobber({42.0});
return c.f;
