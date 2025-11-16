#include "raylib.h"

struct Vector2I {
    int x, y;
    Vector2I(Vector2 v);
    Vector2I(int x, int y);
    Vector2I();

    const Vector2 to_vec2();
};

Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator * (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& a, const Vector2& b);

std::string to_str(const Vector2& v, const int& decimal_pts);