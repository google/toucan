class Foo {
  static float[]* generate() {
    float[]* r = new float[10];
    r[9] = 1234.0;
    return r;
  }
}

return Foo.generate()[9];
