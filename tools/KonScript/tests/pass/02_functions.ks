// Functions: params, return types, calling, multiple returns via tuple

func add(a: I32, b: I32) -> I32 {
    return a + b;
}

func clamp(v: F64, lo: F64, hi: F64) -> F64 {
    if v < lo { return lo; }
    if v > hi { return hi; }
    return v;
}

func minmax(a: I32, b: I32) -> (I32, I32) {
    if a < b { return (a, b); }
    return (b, a);
}

func main() {
    let r: I32 = add(3, 4);
    let c: F64 = clamp(5.0, 0.0, 1.0);
    let mm: (I32, I32) = minmax(10, 3);
}
