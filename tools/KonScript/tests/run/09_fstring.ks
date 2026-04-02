// EXPECT: score: 42
// EXPECT: hi Bob
func main() {
    let score: I32 = 42;
    let msg: Str = f"score: {score}";
    Print(msg);
    let name: Str = "Bob";
    Print(f"hi {name}");
}
