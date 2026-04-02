// 14_multifile.ks — multi-file include test
#include "14_shared.ks"

func distanceBetween(a: Point, b: Point) -> F64 {
    let diff: Point = Point { x: b.x - a.x, y: b.y - a.y };
    return pointLength(diff);
}

func main() {
    let a: Point = Point { x: 1.0, y: 2.0 };
    let b: Point = Point { x: 4.0, y: 6.0 };
    let sum:  Point = pointAdd(a, b);
    let dist: F64   = distanceBetween(a, b);

    let r1: AABB = AABB { x: 0.0,  y: 0.0,  w: 10.0, h: 10.0 };
    let r2: AABB = AABB { x: 5.0,  y: 5.0,  w: 10.0, h: 10.0 };
    let r3: AABB = AABB { x: 20.0, y: 20.0, w: 5.0,  h: 5.0  };

    let hit1: Bool = aabbOverlaps(r1, r2);
    let hit2: Bool = aabbOverlaps(r1, r3);
}
