class C {
  C(float v) {
    value = v;
  }
  float value;
}

C* c = new C(3.14159);
return c.value;
