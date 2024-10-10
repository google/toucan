class P {}
class Q {
  Q(P* p) {
    mP = p;
  }
  var mP : P*;
}
var p = new P();
var q = new Q(null);
