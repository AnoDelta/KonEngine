// 13_classes.ks — class declarations and methods

class Counter {
    let mut value: I32 = 0;
    let step: I32 = 1;

    func increment() {
        value += step;
    }

    func reset() {
        value = 0;
    }

    func get() -> I32 {
        return value;
    }
}

class Stack {
    let mut items: [I32] = [];

    func push(n: I32) {
        items.push(n);
    }

    func pop() -> I32? {
        if items.isEmpty() { return null; }
        return Some(items.pop());
    }

    func size() -> I32 {
        return items.len();
    }

    func isEmpty() -> Bool {
        return items.isEmpty();
    }
}

func main() {
    let mut c: Counter = Counter();
    c.increment();
    c.increment();
    let v: I32 = c.get();
    c.reset();

    let mut s: Stack = Stack();
    s.push(10);
    s.push(20);
    s.push(30);
    let top: I32? = s.pop();
    let sz:  I32  = s.size();
}
