// EXPECT: 45
func main() {
    let mut sum: I32 = 0;
    for i: I32 in 0..10 {
        sum += i;
    }
    Print(sum);
}
