class P {}
class Q {
  Q(p : *P) {
    mP = p;
  }
  var mP : *P;
}
var p = new P();
var q = new Q(null);
