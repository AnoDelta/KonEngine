// Simple enum + switch (BUG-11 regression)

enum Direction {
    North,
    South,
    East,
    West,
}

func opposite(d: Direction) -> Direction {
    switch d {
        case North: return South;
        case South: return North;
        case East:  return West;
        case West:  return East;
        default:    return North;
    }
}

func main() {
    let d: Direction = North;
    let o: Direction = opposite(d);
}
