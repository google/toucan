include "string.t"
class Test {
  static void Expect(bool expr, ubyte[]^ file = System.GetSourceFile(), int line = System.GetSourceLine()) {
    if (!expr) {
      System.Print(file);
      System.Print(":");
      System.Print(String.From(line).Get());
      System.PrintLine(": expectation failed");
    }
  }
  static void Assert(bool expr, ubyte[]^ file = System.GetSourceFile(), int line = System.GetSourceLine()) {
    if (!expr) {
      System.Print(file);
      System.Print(":");
      System.Print(String.From(line).Get());
      System.PrintLine(": assertion failed");
      System.Abort();
    }
  }
}

