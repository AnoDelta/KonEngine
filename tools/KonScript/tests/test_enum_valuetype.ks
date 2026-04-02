// Tests that enum fields in structs/nodes are not turned into pointers

enum Dir {
    North,
    South,
    East,
    West,
}

struct Agent {
    let name: Str;
    let mut facing: Dir;
}

func main() {
    let mut a: Agent = Agent { name: "Bob", facing: North };
    a.facing = South;
}
