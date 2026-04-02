// String operations and f-strings

func greet(name: Str) -> Str {
    return f"Hello, {name}!";
}

func main() {
    let s: Str = "  hello world  ";
    let trimmed: Str = s.trim();
    let upper: Str = s.upper();
    let parts: [Str] = s.split(" ");
    let hasHello: Bool = s.contains("hello");
    let replaced: Str = s.replace("world", "KonScript");

    let msg: Str = greet("Delta");
    let n: I32 = msg.len();

    let empty: Str = "";
    let isEmpty: Bool = empty.isEmpty();
}
