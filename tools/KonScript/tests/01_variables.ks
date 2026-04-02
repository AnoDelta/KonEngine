// 01_variables.ks — let, mut, const, all primitive types

const MAX_HP:    I32 = 100;
const GRAVITY:   F64 = 980.0;
const PI:        F64 = 3.14159;
const APP_NAME:  Str = "KonEngine";
const DEBUG:    Bool = false;

func main() {
    // Immutable
    let x: I32 = 42;
    let y: F64 = 1.5;
    let name: Str = "hello";
    let flag: Bool = true;

    // Mutable
    let mut count: I32 = 0;
    let mut speed: F64 = 200.0;
    let mut alive: Bool = true;

    count += 1;
    speed *= 1.1;
    alive = false;

    // All integer widths
    let a: I8  = 127;
    let b: I16 = 1000;
    let c: I32 = 100000;
    let d: I64 = 9999999999;
    let e: U8  = 255;
    let f: U16 = 65535;
    let g: U32 = 4000000000;

    // Float widths
    let h: F32 = 1.0;
    let i: F64 = 3.14159265358979;

    // Const in function scope
    const LOCAL_LIMIT: I32 = 50;
    let mut n: I32 = 0;
    while n < LOCAL_LIMIT {
        n += 1;
    }
}
