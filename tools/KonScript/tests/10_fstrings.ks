// 10_fstrings.ks — f-string interpolation and Print

func describe(name: Str, hp: I32, maxHp: I32) -> Str {
    return f"{name}: {hp}/{maxHp} HP";
}

func formatCoord(x: F64, y: F64) -> Str {
    return f"({x}, {y})";
}

func statusLine(level: I32, score: I32, lives: I32) -> Str {
    return f"Level {level}  Score: {score}  Lives: {lives}";
}

func main() {
    let name: Str  = "Hero";
    let hp:   I32  = 80;
    let maxHp: I32 = 100;

    let desc: Str  = describe(name, hp, maxHp);
    let coord: Str = formatCoord(12.5, -3.0);
    let level: I32 = 3;
    let score: I32 = 1500;
    let status: Str = statusLine(level, score, 3);

    // Nested expressions inside f-strings
    let msg: Str = f"Result: {hp * 2}";
    let pct: Str = f"HP: {hp}/{maxHp}";
}
