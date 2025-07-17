class A<T> {}

class B : A<float> {}

class C<U> {
  Set(value : U:T) {}
}

var c = C<B>{};
