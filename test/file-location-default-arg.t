include "include/string.t"

class C {
  static f(ubyte[]* filename = System.GetSourceFile(), int line = System.GetSourceLine()) {
    System.Print(filename);
    System.Print(":");
    System.Print(String.From(line).Get());
    System.PrintLine("");
  }
}

C.f();
C.f();
C.f();
