// EXPECT: 5
// EXPECT: 0
func safeDivide(a: I32, b: I32) -> I32? {
    if b == 0 { return null; }
    return a / b;
}
func main() {
    let r: I32? = safeDivide(10, 2);
    let v: I32 = r ?? 0;
    Print(v);
    let bad: I32? = safeDivide(10, 0);
    Print(bad ?? 0);
}
