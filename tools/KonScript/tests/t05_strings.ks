// t05_strings.ks — exercises the runtime functions added in the bug fix
func main() {
    let s: Str = "Hello, World!";

    // concat (string +)
    let greeting: Str = "Hello" + ", " + "KonScript!";
    Print(greeting);                        // Hello, KonScript!

    // len
    Print(s.len());                         // 13

    // charAt
    Print(s.charAt(0));                     // H
    Print(s.charAt(7));                     // W

    // contains / starts / ends
    Print(s.contains("World"));            // 1
    Print(s.starts("Hello"));              // 1
    Print(s.ends("!"));                    // 1

    // upper / lower
    Print(s.upper());                       // HELLO, WORLD!
    Print(s.lower());                       // hello, world!

    // trim
    let padded: Str = "   hi   ";
    Print(padded.trim());                   // hi

    // split
    let csv: Str = "a,b,c,d";
    let parts: [Str] = csv.split(",");
    Print(parts.len());                     // 4

    // isAlpha / isDigit on charAt results
    let ch: Str = s.charAt(0);
    Print(ch.isAlpha());                    // 1
    Print(ch.isDigit());                    // 0

    // toInt / toFloat
    let n: Str = "42";
    Print(n.toInt());                       // 42

    // toCharCode / fromCharCode
    let code: I32 = s.charAt(0).toCharCode();
    Print(code);                            // 72  (ASCII 'H')
    Print(ToString(code));                  // 72
}
