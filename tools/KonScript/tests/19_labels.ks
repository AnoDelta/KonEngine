// 19_labels.ks — labelled break/continue on all loop types
// Syntax: 'label: while / loop / for { ... break 'label; }

func findInMatrix(matrix: [I32], target: I32) -> I32 {
    let n: I32 = matrix.len();
    let mut found: I32 = -1;
    'outer: for i: I32 in 0..n {
        if matrix[i] == target {
            found = i;
            break 'outer;
        }
    }
    return found;
}

func firstDoubleOver(limit: I32) -> I32 {
    let mut n: I32 = 1;
    'search: loop {
        if n * 2 > limit { break 'search; }
        n += 1;
    }
    return n;
}

func skipEvens(limit: I32) -> I32 {
    let mut sum: I32 = 0;
    'counting: for i: I32 in 0..limit {
        if (i % 2) == 0 { continue 'counting; }
        sum += i;
    }
    return sum;
}

func main() {
    let matrix: [I32] = [10, 20, 30, 40, 50];
    let idx:  I32 = findInMatrix(matrix, 30);
    let n:    I32 = firstDoubleOver(20);
    let odds: I32 = skipEvens(10);
}
