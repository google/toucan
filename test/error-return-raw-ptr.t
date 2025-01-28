class ContainsRaw {
  var i : &int;
}
class C {
  m() : &int { var i : int; return &i; }
}

class D {
  m() : &[]int { return [3] new int; }
}

class F {
  m() : ContainsRaw { return ContainsRaw{}; }
}
