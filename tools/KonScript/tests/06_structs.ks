// 06_structs.ks — structs as value types
// NOTE: cannot name a struct 'Vec2' — that is a built-in keyword type.

struct Pos2D {
    let mut x: F64;
    let mut y: F64;
}

struct Rect {
    let x: F64;
    let y: F64;
    let w: F64;
    let h: F64;
}

struct RGBA {
    let r: F64;
    let g: F64;
    let b: F64;
    let a: F64;
}

struct Actor {
    let mut pos: Pos2D;
    let mut hp:  I32;
    let name:    Str;
}

func makeRect(x: F64, y: F64, w: F64, h: F64) -> Rect {
    return Rect { x: x, y: y, w: w, h: h };
}

func rectArea(r: Rect) -> F64 {
    return r.w * r.h;
}

func rectContains(r: Rect, px: F64, py: F64) -> Bool {
    return px >= r.x && px <= (r.x + r.w) &&
           py >= r.y && py <= (r.y + r.h);
}

func main() {
    let r: Rect = makeRect(10.0, 20.0, 100.0, 50.0);
    let area: F64  = rectArea(r);
    let hit:  Bool = rectContains(r, 50.0, 40.0);

    let mut p: Actor = Actor { pos: Pos2D { x: 0.0, y: 0.0 }, hp: 100, name: "Hero" };
    p.hp -= 10;
    p.pos.x += 5.0;
}
