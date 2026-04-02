// Structs as value types (not pointers)
// Note: Vec2 is a reserved keyword — user structs must use different names

struct Point {
    let x: F64;
    let y: F64;
}

struct Rect {
    let mut w: F64;
    let mut h: F64;
}

func area(r: Rect) -> F64 {
    return r.w * r.h;
}

func main() {
    let p: Point = Point { x: 1.0, y: 2.0 };
    let mut r: Rect = Rect { w: 10.0, h: 5.0 };
    r.w = 20.0;
    let a: F64 = area(r);
}
