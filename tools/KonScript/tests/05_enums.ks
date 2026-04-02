// 05_enums.ks — simple enums, payload enums, switch qualification (BUG-11 fix)

// Simple enum
enum Direction {
    North,
    South,
    East,
    West,
}

// Enum with payloads (tagged union)
enum Shape {
    Circle(F64),
    Rectangle(F64),
    Triangle(F64),
}

enum GameState {
    Menu,
    Playing,
    Paused,
    GameOver,
}

func dirToStr(d: Direction) -> Str {
    switch d {
        case North:   return "north";
        case South:   return "south";
        case East:    return "east";
        case West:    return "west";
        default:      return "unknown";
    }
}

func opposite(d: Direction) -> Direction {
    switch d {
        case North:  return South;
        case South:  return North;
        case East:   return West;
        case West:   return East;
        default:     return North;
    }
}

func stateLabel(s: GameState) -> Str {
    switch s {
        case Menu:     return "main menu";
        case Playing:  return "in game";
        case Paused:   return "paused";
        case GameOver: return "game over";
        default:       return "unknown";
    }
}

func main() {
    let d: Direction = North;
    let label: Str = dirToStr(d);
    let opp: Direction = opposite(d);

    let mut state: GameState = Menu;
    let sLabel: Str = stateLabel(state);
    state = Playing;
}
