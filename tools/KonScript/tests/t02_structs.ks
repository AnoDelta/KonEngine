// t02_structs.ks — struct creation, field read/write
// Note: Vec2 is a reserved lexer keyword (engine built-in), so use Point2
struct Point2 {
    let x: F32;
    let y: F32;
}

func dot(a: Point2, b: Point2) -> F32 {
    return a.x * b.x + a.y * b.y;
}

func lenSq(v: Point2) -> F32 {
    return dot(v, v);
}

func main() {
    let a: Point2 = Point2 { x: 3.0, y: 4.0 };
    let b: Point2 = Point2 { x: 1.0, y: 2.0 };
    Print(a.x);          // 3.0
    Print(a.y);          // 4.0
    Print(dot(a, b));    // 11.0
    Print(lenSq(a));     // 25.0
}
