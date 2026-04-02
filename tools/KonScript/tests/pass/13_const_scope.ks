// const at global scope -> constexpr, at function scope -> const (BUG-09 regression)

const GLOBAL_MAX: I32 = 1000;
const RATIO: F64 = 0.75;

func compute(n: I32) -> I32 {
    const STEP: I32 = 10;
    const SCALE: F64 = 2.0;
    let mut result: I32 = 0;
    for i: I32 = 0; i < n; i += STEP {
        result += i;
    }
    return result;
}

func main() {
    let r: I32 = compute(GLOBAL_MAX);
}
