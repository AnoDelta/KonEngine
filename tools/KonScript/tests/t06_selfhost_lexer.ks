// t06_selfhost_lexer.ks — run the self-hosted lexer on a small KonScript snippet
// This is the real milestone: KonScript tokenising KonScript.
//
// Include the self-hosted compiler source (lexer + token stream phases only).
// Adjust the path if konscript.ks lives elsewhere relative to this file.
#include "konscript.ks"

func main() {
    let source: Str = "func add(a: I32, b: I32) -> I32 { return a + b; }";

    let mut lexer: Lexer = Lexer { src: "", pos: 0, line: 1, col: 1, tokens: [] };
    lexer.init(source);
    let toks: [Token] = lexer.tokenize();

    Print(f"source  : {source}");
    Print(f"tokens  : {toks.len()}");

    // Print each token kind as an integer ordinal and its value
    for i: I32 in 0..toks.len() {
        // Can't index array-of-struct yet in for-in, so use index loop
    }

    // Just verify the token count and first/last token
    let first: Token = toks[0];
    let last: Token  = toks[toks.len() - 1];
    Print(f"first   : kind={first.kind} value={first.value}");
    Print(f"last    : kind={last.kind}  value={last.value}");
    // Expected: first=KwFunc(4), last=Eof
}
