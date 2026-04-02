// Tests BUG-11: switch on enum should emit case State::Idle: not case Idle:

enum State {
    Idle,
    Walking,
    Running,
}

func getLabel(s: State) -> Str {
    switch s {
        case Idle:
            return "idle";
        case Walking:
            return "walk";
        case Running:
            return "run";
        default:
            return "unknown";
    }
}

func main() {
    let s: State = Walking;
    let label: Str = getLabel(s);
}
