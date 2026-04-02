class Counter {
    let mut count: I32 = 0;

    func increment(mut self) {
        self.count += 1;
    }
    func reset(mut self) {
        self.count = 0;
    }
    func value(self) -> I32 {
        return self.count;
    }
}

interface Resettable {
    func reset(mut self);
}

class Timer implements Resettable {
    let mut ticks: I32 = 0;
    func tick(mut self) { self.ticks += 1; }
    func reset(mut self) { self.ticks = 0; }
    func elapsed(self) -> I32 { return self.ticks; }
}

func main() {
    let mut c: Counter = Counter { count: 0 };
    c.increment();
    c.increment();
    c.increment();
    Print(c.value());
}
