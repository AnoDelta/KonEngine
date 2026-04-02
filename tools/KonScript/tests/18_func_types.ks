// 18_func_types.ks — function types as first-class type annotations
// func(I32) -> Bool   maps to   std::function<bool(int32_t)>

func filter(items: [I32], pred: func(I32) -> Bool) -> [I32] {
    let mut result: [I32] = [];
    for item: I32 in items {
        if pred(item) { result.push(item); }
    }
    return result;
}

func compose(f: func(I32) -> I32, g: func(I32) -> I32) -> func(I32) -> I32 {
    return func(x: I32) -> I32 {
        return f(g(x));
    };
}

func applyTwice(f: func(I32) -> I32, x: I32) -> I32 {
    return f(f(x));
}

func main() {
    let isEven: func(I32) -> Bool = func(n: I32) -> Bool {
        return (n % 2) == 0;
    };
    let nums: [I32] = [1, 2, 3, 4, 5, 6];
    let evens: [I32] = filter(nums, isEven);

    let triple: func(I32) -> I32 = func(x: I32) -> I32 { return x * 3; };
    let inc:    func(I32) -> I32 = func(x: I32) -> I32 { return x + 1; };
    let tripleInc: func(I32) -> I32 = compose(triple, inc);
    let result: I32 = tripleInc(3);

    let quadruple: I32 = applyTwice(func(x: I32) -> I32 { return x * 2; }, 3);
}
