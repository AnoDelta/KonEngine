// 22_coroutines.ks — cooperative coroutines (requires C++20)
// spawn f()  schedules f as a coroutine; wait N suspends for N game-seconds
// Drive from your engine Update(): _ks_sched.update(dt)  (C++ call)

func delay(msg: Str, seconds: F64) {
    wait seconds;
    Print(msg);
}

func countdown(from: I32) {
    let mut n: I32 = from;
    while n > 0 {
        Print(f"T-{n}");
        wait 1.0;
        n -= 1;
    }
    Print("Liftoff!");
}

func sequence(name: Str) {
    Print(f"{name}: step 1");
    wait 0.5;
    Print(f"{name}: step 2");
    wait 0.5;
    Print(f"{name}: step 3");
}

func main() {
    spawn delay("hello after 2s", 2.0);
    spawn countdown(3);
    spawn sequence("A");
    spawn sequence("B");
}
