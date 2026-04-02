// Tuples as return types and variables

func divmod(a: I32, b: I32) -> (I32, I32) {
    return (a / b, a % b);
}

func bounds(arr: [I32]) -> (I32, I32) {
    let mut lo: I32 = arr[0];
    let mut hi: I32 = arr[0];
    for v: I32 in arr {
        if v < lo { lo = v; }
        if v > hi { hi = v; }
    }
    return (lo, hi);
}

func main() {
    let dm: (I32, I32) = divmod(17, 5);
    let nums: [I32] = [3, 1, 4, 1, 5, 9];
    let b: (I32, I32) = bounds(nums);
}
