// Dynamic arrays: literals, push, pop, len, for-in

func main() {
    let mut arr: [I32] = [1, 2, 3, 4, 5];
    arr.push(6);

    let n: I32 = arr.len();
    let flag: Bool = arr.isEmpty();

    let mut total: I32 = 0;
    for v: I32 in arr {
        total += v;
    }
}
