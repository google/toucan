class String {
  String(ubyte[]* b) : { buffer = b } {}
  static int IntLog2(int value) { return 31 - Math.clz(value | 1); }
  static String* From(int value) {
    int[9] table = {9, 99, 999, 9999, 99999, 999999, 9999999, 99999999, 999999999};
    bool negative = false;
    if (value < 0) {
      negative = true;
      value = -value;
    }
    int len = (9 * String.IntLog2(value)) / 32;
    if (value > table[len]) len += 1;
    len += 1;
    if (negative) len += 1;
    var b = new ubyte[len];
    for (int j = len - 1; j >= 0; j--) {
      b[j] = (ubyte) (value % 10) + 48ub;
      value /= 10;
    }
    if (negative) b[0] = 45ub;
    return new String(b);
  }
  ubyte[]* Get() { return buffer; }
  ubyte[]* buffer;
}
