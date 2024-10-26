class String {
  String(b : *[]ubyte) { buffer = b; }
  static IntLog2(value : int) : int { return 31 - Math.clz(value | 1); }
  static From(value : int) : *String {
    var table = [9]int(9, 99, 999, 9999, 99999, 999999, 9999999, 99999999, 999999999);
    var negative = false;
    if (value < 0) {
      negative = true;
      value = -value;
    }
    var len = (9 * String.IntLog2(value)) / 32;
    if (value > table[len]) len += 1;
    len += 1;
    if (negative) len += 1;
    var b = [len] new ubyte;
    for (var j = len - 1; j >= 0; j--) {
      b[j] = (ubyte) (value % 10) + 48ub;
      value /= 10;
    }
    if (negative) b[0] = 45ub;
    return new String(b);
  }
  Get() : *[]ubyte { return buffer; }
  var buffer : *[]ubyte;
}
