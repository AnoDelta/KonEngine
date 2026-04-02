// t01_basic.ks — arithmetic, functions, print
func add(a: I32, b: I32) -> I32 {
    return a + b;
}

func factorial(n: I32) -> I32 {
    if n <= 1 { return 1; }
    return n * factorial(n - 1);
}

func main() {
    Print(add(3, 4));           // 7
    Print(factorial(6));        // 720
    let x: I32 = 10;
    let y: I32 = x * x + 1;
    Print(y);                   // 101
    Print(f"sum={add(10,20)}"); // sum=30
}
