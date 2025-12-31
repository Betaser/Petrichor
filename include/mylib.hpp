#ifndef MYLIB_H
#define MYLIB_H

#include <string>
#include <random>
#include "raylib.h"

struct Vector2I {
    int x, y;
    Vector2I(Vector2 v);
    Vector2I(int x, int y);
    Vector2I();

    Vector2 to_vec2();
};

class Rand {
    private:
    std::mt19937 int_gen;
    std::uniform_real_distribution<double> dist;

    public:
    Rand(int seed); 

    int seed;

    void set_seed(int seed);
    float gen(float a, float b);
};

#endif

void operator += (Vector2& a, const Vector2& b);
Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator * (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& v);
Vector2 my_rotate(const Vector2& origin, const Vector2& pt, const float& amt);

float direction_to_rotate(const Vector2& ahead, const Vector2& mobile);
float my_angle(const Vector2& v);
float dot(const Vector2& a, const Vector2& b);
float my_angle_from(const Vector2& a, const Vector2& b);
float my_length(const Vector2& v);
Vector2 my_normalize(const Vector2& v);
Vector2 perp_rhr(const Vector2& v);
Vector2 unit_vector(const float& f);

// Debug
std::string to_str(const Vector2& v, const int& decimal_pts);

// Misc
Texture2D load_dummy_tex();
bool pt_in_rect(const Vector2& pt, const Vector2& pos, const Vector2& dim);

// Math, non vector
float snap(const float& f, const float& by);
Color lerp(const Color& a, const Color& b, const float& amt);