{
  int outer_count = 1000000;
  int[] a = new int[1024];
  for (int i = 0; i < a.length; ++i) {
    a[i] = 2000000;
  }
  for (int j = 0; j < outer_count; ++j) {
    for (int i = 0; i < a.length; ++i) {
      a[i] = a[i] * 2 + 1;
    }
  }
  return 3.0; // a[0];
}
