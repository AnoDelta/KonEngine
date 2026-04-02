// 20_safe_member.ks — ?. safe member access on nullable types
// cfg?.field returns field-type? (optional).
// Chain with ?? to unwrap: cfg?.name ?? "default"
// NOTE: do NOT call methods directly on ?.  result without ?? unwrap first.
//   OK:   cfg?.name ?? ""
//   BAD:  cfg?.name.len()  (calls .len() on optional<Str>, not Str)

struct Config {
    let name:  Str;
    let value: I32;
}

struct Node {
    let next: Config?;
    let data: I32;
}

func getName(cfg: Config?) -> Str {
    return cfg?.name ?? "unknown";
}

func getValue(cfg: Config?) -> I32 {
    return cfg?.value ?? -1;
}

func main() {
    let cfg:  Config? = Some(Config { name: "speed", value: 200 });
    let none: Config? = null;

    let name1: Str = getName(cfg);
    let name2: Str = getName(none);
    let val1:  I32 = getValue(cfg);
    let val2:  I32 = getValue(none);

    let name3: Str = cfg?.name ?? "fallback";
    let val3:  I32 = cfg?.value ?? 0;
}
