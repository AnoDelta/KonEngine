// 02_functions.ks — functions, return types, tuples, recursion

func add(a: I32, b: I32) -> I32 {
    return a + b;
}

func multiply(a: F64, b: F64) -> F64 {
    return a * b;
}

func clamp(val: F64, lo: F64, hi: F64) -> F64 {
    if val < lo { return lo; }
    if val > hi { return hi; }
    return val;
}

func isEven(n: I32) -> Bool {
    return (n % 2) == 0;
}

func greet(name: Str) -> Str {
    return f"Hello, {name}!";
}

func minMax(a: I32, b: I32) -> (I32, I32) {
    if a < b { return (a, b); }
    return (b, a);
}

func factorial(n: I32) -> I32 {
    if n <= 1 { return 1; }
    return n * factorial(n - 1);
}

func hypotenuse(a: F64, b: F64) -> F64 {
    return Sqrt(a * a + b * b);
}

func main() {
    let sum:  I32  = add(3, 4);
    let prod: F64  = multiply(2.5, 4.0);
    let c:    F64  = clamp(150.0, 0.0, 100.0);
    let even: Bool = isEven(8);
    let msg:  Str  = greet("Delta");
    let fact: I32  = factorial(5);
    let hyp:  F64  = hypotenuse(3.0, 4.0);
}
