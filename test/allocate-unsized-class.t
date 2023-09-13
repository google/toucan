class C {
  float f;
  int[] array;
};
C* c = new [5]C();
c.array[2] = 42;

return (float) c.array[2];
