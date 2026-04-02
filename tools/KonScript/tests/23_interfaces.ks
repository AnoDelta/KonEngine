interface Printable {
    func toString(self) -> Str;
}

interface Shape {
    func area(self) -> F64;
    func perimeter(self) -> F64;
}

class Circle implements Shape {
    let radius: F64;
    func area(self) -> F64 { return 3.14159 * self.radius * self.radius; }
    func perimeter(self) -> F64 { return 2.0 * 3.14159 * self.radius; }
}

class Dog implements Printable {
    let name: Str;
    func toString(self) -> Str { return self.name; }
}

func main() {
    let d: Dog = Dog { name: "Rex" };
    Print(d.toString());
}
