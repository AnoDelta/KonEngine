struct Vector2 {
    float x, y;

    Vector2(float x = 0, float y = 0);
    Vector2(const glm::vec2& v);
    operator glm::vec2() const;

    // Arithmetic: +, -, *, /, +=, -=, *=, /=, unary -
    // Comparison: ==, !=

    float   Length() const;
    float   LengthSq() const;
    Vector2 Normalized() const;
    float   Dot(const Vector2& other) const;
    float   Distance(const Vector2& other) const;
    float   DistanceSq(const Vector2& other) const;
    Vector2 Rotated(float angleRadians) const;
    Vector2 Reflected(const Vector2& normal) const;

    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t);

    static Vector2 Zero();
    static Vector2 One();
    static Vector2 Up();
    static Vector2 Down();
    static Vector2 Left();
    static Vector2 Right();
};
