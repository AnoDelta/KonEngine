// 21_generics.ks — user-defined generic functions, structs, classes
// func max<T>(a: T, b: T) -> T  →  template<typename T> T max(T a, T b)
// struct Pair<A, B>              →  template<typename A,typename B> struct Pair
// class Stack<T>                 →  template<typename T> class Stack
// T is inferred at call sites — no explicit <T> needed when calling.

func max<T>(a: T, b: T) -> T {
    if a > b { return a; }
    return b;
}

func min<T>(a: T, b: T) -> T {
    if a < b { return a; }
    return b;
}

func clamp<T>(val: T, lo: T, hi: T) -> T {
    if val < lo { return lo; }
    if val > hi { return hi; }
    return val;
}

func identity<T>(x: T) -> T {
    return x;
}

func swap<T>(a: T, b: T) -> (T, T) {
    return (b, a);
}

struct Pair<A, B> {
    let first:  A;
    let second: B;
}

struct Wrapper<T> {
    let mut value: T;
}

class Stack<T> {
    let mut items: [T] = [];
    func push(item: T)     { items.push(item); }
    func size()  -> I32    { return items.len(); }
    func isEmpty() -> Bool { return items.isEmpty(); }
}

func main() {
    let a: I32 = max(3, 7);
    let b: F64 = max(1.5, 2.5);
    let c: I32 = clamp(15, 0, 10);
    let d: I32 = identity(42);
    let e: Str = identity("hello");
    let p: (I32, I32) = swap(10, 20);
}
