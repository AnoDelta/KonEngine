// Result<T>: ok, value, error

func parseInt(s: Str) -> Result<I32> {
    if s.isEmpty() {
        return Result<I32>.Err("empty string");
    }
    return Result<I32>.Ok(s.toInt());
}

func main() {
    let r: Result<I32> = parseInt("42");
    if r.ok {
        let v: I32 = r.value;
    }
    let bad: Result<I32> = parseInt("");
    let errMsg: Str = bad.error;
}
