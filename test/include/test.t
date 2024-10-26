include "string.t"
class Test {
  static Expect(expr : bool, file : ^[]ubyte = System.GetSourceFile(), line : uint = System.GetSourceLine()) {
    if (!expr) {
      System.Print(file);
      System.Print(":");
      System.Print(String.From(line).Get());
      System.PrintLine(": expectation failed");
    }
  }
  static Assert(expr : bool, file : ^[]ubyte = System.GetSourceFile(), line : uint = System.GetSourceLine()) {
    if (!expr) {
      System.Print(file);
      System.Print(":");
      System.Print(String.From(line).Get());
      System.PrintLine(": assertion failed");
      System.Abort();
    }
  }
}

