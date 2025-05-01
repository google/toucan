include "include/string.t"
var pi = new int;
var wpi : ^int = pi;
pi = null;
var result = wpi:;
System.PrintLine(String.From(result).Get());
