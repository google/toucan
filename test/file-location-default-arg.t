include "include/string.t"

class C {
  static f(filename : *[]ubyte = System.GetSourceFile(), line : int = System.GetSourceLine()) {
    System.Print(filename);
    System.Print(":");
    System.Print(String.From(line).Get());
    System.PrintLine("");
  }
}

C.f();
C.f();
C.f();
