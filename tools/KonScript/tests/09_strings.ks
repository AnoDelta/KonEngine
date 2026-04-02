// 09_strings.ks — string methods: split, trim, contains, replace, etc.

func countWords(s: Str) -> I32 {
    let words: [Str] = s.split(" ");
    return words.len();
}

func normalize(s: Str) -> Str {
    return s.trim().lower();
}

func replaceSpaces(s: Str) -> Str {
    return s.replace(" ", "_");
}

func main() {
    let s: Str = "  Hello World  ";

    let trimmed: Str    = s.trim();
    let upper:   Str    = trimmed.upper();
    let lower:   Str    = trimmed.lower();

    let csv: Str        = "a,b,c,d";
    let parts: [Str]    = csv.split(",");
    let count: I32      = parts.len();

    let hasHello: Bool  = s.contains("Hello");
    let startsH:  Bool  = trimmed.starts("Hello");
    let endsD:    Bool  = trimmed.ends("World");
    let sub:      Str   = trimmed.substr(0, 5);

    let sentence: Str   = "the quick brown fox";
    let wc: I32         = countWords(sentence);
    let norm: Str       = normalize("  KonScript  ");
    let slug: Str       = replaceSpaces("hello world");
}
