// Nullable types: T?, null, ??, !

func safeDivide(a: F64, b: F64) -> F64? {
    if b == 0.0 { return null; }
    return a / b;
}

func main() {
    let r: F64? = safeDivide(10.0, 2.0);
    let fallback: F64 = r ?? 0.0;

    let none: I32? = null;
    let also: F64? = safeDivide(1.0, 0.0);
}
