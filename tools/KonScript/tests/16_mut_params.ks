// 16_mut_params.ks — mut function parameters (mutable local copy, like Rust)
// func f(mut n: I32) means n is a mutable local inside f, not a reference.

func clamp(mut val: F64, lo: F64, hi: F64) -> F64 {
    if val < lo { val = lo; }
    if val > hi { val = hi; }
    return val;
}

func normalise(mut x: F64, mut y: F64) -> F64 {
    let len: F64 = Sqrt(x * x + y * y);
    if len == 0.0 { return 0.0; }
    x = x / len;
    y = y / len;
    return Sqrt(x * x + y * y);
}

func countdown(mut n: I32) -> I32 {
    let mut steps: I32 = 0;
    while n > 0 {
        n -= 1;
        steps += 1;
    }
    return steps;
}

func main() {
    let speed: F64  = clamp(250.0, 0.0, 200.0);
    let unit:  F64  = normalise(3.0, 4.0);
    let steps: I32  = countdown(10);
}
