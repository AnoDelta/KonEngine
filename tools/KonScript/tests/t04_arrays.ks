// t04_arrays.ks — array push/pop/len, for-in over range and array
func main() {
    // Range loop
    let mut sum: I32 = 0;
    for i: I32 in 1..=10 {
        sum += i;
    }
    Print(sum);   // 55

    // Array push / len / index
    let mut arr: [I32] = [];
    arr.push(10);
    arr.push(20);
    arr.push(30);
    Print(arr.len());    // 3

    // For-in over array
    let mut total: I32 = 0;
    for v: I32 in arr {
        total += v;
    }
    Print(total);  // 60

    // Pop
    arr.pop();
    Print(arr.len());  // 2
}
