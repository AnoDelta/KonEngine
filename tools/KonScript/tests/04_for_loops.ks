// 04_for_loops.ks — for-C, for-in with ranges, iterating arrays

func sumRange(n: I32) -> I32 {
    let mut total: I32 = 0;
    for i: I32 in 0..n {
        total += i;
    }
    return total;
}

func sumInclusive(n: I32) -> I32 {
    let mut total: I32 = 0;
    for i: I32 in 1..=n {
        total += i;
    }
    return total;
}

func countdownForC() -> I32 {
    let mut last: I32 = 0;
    for i: I32 = 10; i > 0; i-- {
        last = i;
    }
    return last;
}

func iterateArray() -> I32 {
    let nums: [I32] = [1, 2, 3, 4, 5];
    let mut sum: I32 = 0;
    for n: I32 in nums {
        sum += n;
    }
    return sum;
}

func nestedRanges() -> I32 {
    let mut count: I32 = 0;
    for i: I32 in 0..3 {
        for j: I32 in 0..3 {
            count += 1;
        }
    }
    return count;
}

func main() {
    let s1: I32 = sumRange(10);
    let s2: I32 = sumInclusive(10);
    let cd: I32 = countdownForC();
    let ia: I32 = iterateArray();
    let nr: I32 = nestedRanges();
}
