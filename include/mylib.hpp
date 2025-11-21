#include <string>
#include "raylib.h"

struct Vector2I {
    int x, y;
    Vector2I(Vector2 v);
    Vector2I(int x, int y);
    Vector2I();

    const Vector2 to_vec2();
};

class Rand {
    private:
    std::mt19937 int_gen;
    std::uniform_real_distribution<double> dist;

    public:
    Rand(int seed); 

    int seed;

    void set_seed(int seed);
    const float gen(float a, float b);
};

Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator * (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& v);
Vector2 my_rotate(const Vector2& origin, const Vector2& pt, const float& amt);

float my_angle(const Vector2& v);
float dot(const Vector2& a, const Vector2& b);
float my_angle_from(const Vector2& a, const Vector2& b);
float my_length(const Vector2& v);
Vector2 my_normalize(const Vector2& v);
Vector2 perp_rhr(const Vector2& v);
Vector2 unit_vector(const float& f);

std::string to_str(const Vector2& v, const int& decimal_pts);
float snap(const float& f, const float& by);