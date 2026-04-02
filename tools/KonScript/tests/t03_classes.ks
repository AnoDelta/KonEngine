// t03_classes.ks — class methods, self access, mutation
class Counter {
    let mut value: I32 = 0;

    func inc(mut self) {
        self.value += 1;
    }

    func add(mut self, n: I32) {
        self.value += n;
    }

    func get(self) -> I32 {
        return self.value;
    }

    func reset(mut self) {
        self.value = 0;
    }
}

func main() {
    let mut c: Counter = Counter { value: 0 };
    c.inc();
    c.inc();
    c.inc();
    Print(c.get());     // 3
    c.add(10);
    Print(c.get());     // 13
    c.reset();
    Print(c.get());     // 0
}
