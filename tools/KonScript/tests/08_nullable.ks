// 08_nullable.ks — nullable types, null coalescing, force unwrap
// NOTE: 0..arr.len() has a parser precedence issue (parses as (0..arr).len())
//       Workaround: store .len() in a variable first.

func findFirst(arr: [I32], target: I32) -> I32? {
    let n: I32 = arr.len();
    for i: I32 in 0..n {
        if arr[i] == target { return Some(i); }
    }
    return null;
}

func safeDiv(a: F64, b: F64) -> F64? {
    if b == 0.0 { return null; }
    return Some(a / b);
}

func withDefault(val: I32?, fallback: I32) -> I32 {
    return val ?? fallback;
}

func main() {
    let arr: [I32] = [10, 20, 30, 40];

    let idx: I32? = findFirst(arr, 30);
    let pos: I32  = idx ?? -1;

    let result: F64? = safeDiv(10.0, 2.0);
    let value:  F64  = result ?? 0.0;

    let bad:  F64? = safeDiv(10.0, 0.0);
    let safe: F64  = bad ?? -1.0;

    let a: I32 = withDefault(null, 42);
    let b: I32 = withDefault(Some(7), 42);
}
