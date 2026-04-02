// 17_closures.ks — anonymous functions / closures
// Syntax: func(params) -> RetType { body }
// Captures surrounding scope by reference ([&] in generated C++).

func apply(f: func(I32) -> I32, x: I32) -> I32 {
    return f(x);
}

func makeAdder(n: I32) -> func(I32) -> I32 {
    return func(x: I32) -> I32 {
        return x + n;
    };
}

func makeCounter() -> func() -> I32 {
    let mut count: I32 = 0;
    return func() -> I32 {
        count += 1;
        return count;
    };
}

func main() {
    let twice: func(I32) -> I32 = func(x: I32) -> I32 {
        return x * 2;
    };
    let result: I32 = apply(twice, 5);

    let add10: func(I32) -> I32 = makeAdder(10);
    let val: I32 = add10(3);

    let mut total: I32 = 0;
    let accumulate: func(I32) = func(n: I32) {
        total += n;
    };
    accumulate(1);
    accumulate(2);
    accumulate(3);
}
