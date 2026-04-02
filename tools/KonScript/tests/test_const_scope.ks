func compute() -> I32 {
    const LIMIT: I32 = 100;
    let mut x: I32 = 0;
    while x < LIMIT {
        x += 1;
    }
    return x;
}

const GLOBAL: I32 = 42;

func main() {
    let r: I32 = compute();
}
