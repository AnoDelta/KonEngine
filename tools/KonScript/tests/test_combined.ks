enum Phase {
    Init,
    Running,
    Done,
}

struct Game {
    let mut score: I32;
    let mut phase: Phase;
}

func tick(g: Game) -> Game {
    switch g.phase {
        case Init:
            g.phase = Running;
        case Running:
            g.score += 1;
        case Done:
            g.score = 0;
        default:
            g.score = -1;
    }
    return g;
}

func main() {
    let mut g: Game = Game { score: 0, phase: Init };
}
