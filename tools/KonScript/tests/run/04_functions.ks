// EXPECT: 7
// EXPECT: -1
func add(a: I32, b: I32) -> I32 { return a + b; }
func sign(n: I32) -> I32 {
    if n > 0 { return 1; }
    if n < 0 { return -1; }
    return 0;
}
func main() {
    Print(add(3, 4));
    Print(sign(-99));
}
