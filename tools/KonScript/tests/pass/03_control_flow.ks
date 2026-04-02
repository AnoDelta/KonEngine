// if/else, while, loop/break, nested

func sign(n: I32) -> I32 {
    if n > 0 {
        return 1;
    } else if n < 0 {
        return -1;
    } else {
        return 0;
    }
}

func main() {
    let mut i: I32 = 0;
    while i < 10 {
        i += 1;
    }

    let mut j: I32 = 0;
    loop {
        j += 1;
        if j >= 5 { break; }
    }

    let s: I32 = sign(-3);
}
