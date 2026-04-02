// EXPECT: 3
// EXPECT: 4
func main() {
    let mut arr: [I32] = [10, 20, 30];
    Print(arr.len());
    arr.push(40);
    Print(arr.len());
}
