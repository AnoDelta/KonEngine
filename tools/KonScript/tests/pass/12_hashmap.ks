// HashMap<K, V>: set, get, has, len

func main() {
    let mut scores: HashMap<Str, I32> = HashMap();
    scores.set("Alice", 95);
    scores.set("Bob", 87);
    scores.set("Carol", 92);

    let has: Bool = scores.has("Alice");
    let n: I32 = scores.len();
}
