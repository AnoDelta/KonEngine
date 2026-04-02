// Enum with payload (tagged union)

enum Shape {
    Circle(F64),
    Square(F64),
    Rect(F64),
}

func main() {
    let s: Shape = Circle(5.0);
}
