// 07_arrays.ks — dynamic arrays, methods, iteration

func sumArray(arr: [I32]) -> I32 {
    let mut total: I32 = 0;
    for n: I32 in arr {
        total += n;
    }
    return total;
}

func buildRange(start: I32, end: I32) -> [I32] {
    let mut result: [I32] = [];
    for i: I32 in start..end {
        result.push(i);
    }
    return result;
}

func reverseSum(n: I32) -> I32 {
    let mut arr: [I32] = [];
    for i: I32 in 0..n {
        arr.push(i);
    }
    let mut sum: I32 = 0;
    while !arr.isEmpty() {
        let val: I32 = arr.pop();
        sum += val;
    }
    return sum;
}

func arrayLen() -> I32 {
    let mut arr: [Str] = [];
    arr.push("a");
    arr.push("b");
    arr.push("c");
    return arr.len();
}

func indexAccess() -> I32 {
    let nums: [I32] = [10, 20, 30, 40, 50];
    return nums[2];
}

func main() {
    let arr: [I32] = [1, 2, 3, 4, 5];
    let s1:  I32   = sumArray(arr);
    let r:   [I32] = buildRange(0, 5);
    let s2:  I32   = reverseSum(5);
    let l:   I32   = arrayLen();
    let v:   I32   = indexAccess();
}
