// 03_control_flow.ks — if/else, while, loop, break, continue

func sign(n: I32) -> I32 {
    if n > 0 {
        return 1;
    } else if n < 0 {
        return -1;
    } else {
        return 0;
    }
}

func countDown(start: I32) -> I32 {
    let mut n: I32 = start;
    let mut steps: I32 = 0;
    while n > 0 {
        n -= 1;
        steps += 1;
    }
    return steps;
}

func firstEven(limit: I32) -> I32 {
    let mut i: I32 = 1;
    while i < limit {
        if isEven(i) { return i; }
        i += 1;
    }
    return -1;
}

func isEven(n: I32) -> Bool {
    return (n % 2) == 0;
}

func loopWithBreak() -> I32 {
    let mut count: I32 = 0;
    loop {
        count += 1;
        if count >= 10 { break; }
    }
    return count;
}

func sumOdds(limit: I32) -> I32 {
    let mut sum: I32 = 0;
    let mut i: I32 = 0;
    while i < limit {
        i += 1;
        if isEven(i) { continue; }
        sum += i;
    }
    return sum;
}

func main() {
    let s1: I32 = sign(5);
    let s2: I32 = sign(-3);
    let s3: I32 = sign(0);
    let cd: I32 = countDown(10);
    let fe: I32 = firstEven(20);
    let lb: I32 = loopWithBreak();
    let so: I32 = sumOdds(10);
}
