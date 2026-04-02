// 11_hashmap.ks — HashMap<K, V>

func buildScoreTable() -> HashMap<Str, I32> {
    let mut scores: HashMap<Str, I32> = HashMap();
    scores.set("Alice", 1500);
    scores.set("Bob",   1200);
    scores.set("Carol", 1800);
    return scores;
}

func topPlayer(scores: HashMap<Str, I32>) -> I32 {
    let alice: I32? = scores.get("Alice");
    let carol: I32? = scores.get("Carol");
    let a: I32 = alice ?? 0;
    let c: I32 = carol ?? 0;
    if a > c { return a; }
    return c;
}

func main() {
    let mut map: HashMap<Str, I32> = HashMap();

    map.set("x", 10);
    map.set("y", 20);
    map.set("z", 30);

    let val: I32? = map.get("x");
    let n: I32 = val ?? 0;

    let hasX: Bool = map.has("x");
    let hasW: Bool = map.has("w");

    map.remove("y");
    let size: I32 = map.len();

    let scores: HashMap<Str, I32> = buildScoreTable();
    let top: I32 = topPlayer(scores);
}
