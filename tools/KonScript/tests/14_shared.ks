// 14_shared.ks — shared types (included by 14_multifile.ks)
// NOTE: cannot name a struct 'Vec2' — that is a built-in keyword.

struct Point {
    let mut x: F64;
    let mut y: F64;
}

struct AABB {
    let x: F64;
    let y: F64;
    let w: F64;
    let h: F64;
}

func pointAdd(a: Point, b: Point) -> Point {
    return Point { x: a.x + b.x, y: a.y + b.y };
}

func pointLength(v: Point) -> F64 {
    return Sqrt(v.x * v.x + v.y * v.y);
}

func aabbOverlaps(a: AABB, b: AABB) -> Bool {
    return a.x < (b.x + b.w) && (a.x + a.w) > b.x &&
           a.y < (b.y + b.h) && (a.y + a.h) > b.y;
}
