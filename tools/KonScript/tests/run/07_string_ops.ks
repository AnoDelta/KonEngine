// EXPECT: 5
// EXPECT: true
// EXPECT: HELLO
func main() {
    let s: Str = "hello";
    Print(s.len());
    Print(s.contains("ell"));
    Print(s.upper());
}
