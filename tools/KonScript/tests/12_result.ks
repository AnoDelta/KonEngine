// 12_result.ks — Result<T> from File I/O
// NOTE: Result<T>::Ok(...) constructor syntax not supported by parser.
//       Results are obtained from File/stdlib operations and accessed via .ok/.value/.error

func readAndProcess(path: Str) -> Str {
    let res: Result<Str> = File.read(path);
    if res.ok {
        return res.value;
    }
    return f"error: {res.error}";
}

func countLines(path: Str) -> I32 {
    let res: Result<[Str]> = File.lines(path);
    if !res.ok { return -1; }
    return res.value.len();
}

func fileExists(path: Str) -> Bool {
    return File.exists(path);
}

func main() {
    let content: Str = readAndProcess("config.txt");
    let lines:   I32 = countLines("config.txt");
    let exists:  Bool = fileExists("data.txt");

    let r: Result<Str> = File.write("out.txt", "hello");
    if r.ok {
        let confirm: Str = "wrote ok";
    }
}
