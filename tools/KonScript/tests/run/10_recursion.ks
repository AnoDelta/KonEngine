// EXPECT: 120
// EXPECT: 55
func factorial(n: I32) -> I32 {
    if n <= 1 { return 1; }
    return n * factorial(n - 1);
}
func fib(n: I32) -> I32 {
    if n <= 1 { return n; }
    return fib(n - 1) + fib(n - 2);
}
func main() {
    Print(factorial(5));
    Print(fib(10));
}
