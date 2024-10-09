include "include/string.t"

var filename = System.GetSourceFile();
int line = System.GetSourceLine();

System.Print(filename);
System.Print(":");
System.Print(String.From(line).Get());
System.PrintLine("");
