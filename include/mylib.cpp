#include <cstdio>
#include <math.h>
#include "mylib.hpp"

Vector2I::Vector2I(Vector2 v) {
    this->x = v.x;
    this->y = v.y;
}

Vector2I::Vector2I(int x, int y) {
    this->x = x;
    this->y = y;
}

Vector2I::Vector2I() {
    this->x = 0;
    this->y = 0;
}

Rand::Rand(int seed) {
    dist = std::uniform_real_distribution<>(0, 1);
    set_seed(seed);
}

void Rand::set_seed(int seed) {
    this->seed = seed;
    int_gen = std::mt19937(seed);
}

const float Rand::gen(float a, float b) {
    const float norm = dist(int_gen);
    return norm * (b - a) + a;
}

const Vector2 Vector2I::to_vec2() {
    return Vector2 { (float) x, (float) y };
}

Vector2 operator + (const Vector2& a, const Vector2& b) {
    return Vector2 { a.x + b.x, a.y + b.y };
}

Vector2 operator * (const Vector2& v, const float& f) {
    return Vector2 { v.x * f, v.y * f };
}

Vector2 operator - (const Vector2& a, const Vector2& b) {
    return Vector2 { a.x - b.x, a.y - b.y };
}

Vector2 operator - (const Vector2& v) {
    return Vector2 { -v.x, -v.y };
}

Vector2 operator / (const Vector2& a, const Vector2& b) {
    return Vector2 { a.x / b.x, a.y / b.y };
}

Vector2 operator / (const Vector2& v, const float& f) {
    return Vector2 { v.x / f, v.y / f };
}

Vector2 my_rotate(const Vector2& origin, const Vector2& pt, const float& amt) {
    const Vector2 out = pt - origin;
    return Vector2 { 
        out.x * cos(amt) - out.y * sin(amt),
        out.x * sin(amt) + out.y * cos(amt)
    } + origin;
}

float my_angle(const Vector2& v) {
    const float ang = atan2f(v.y, v.x);
    if (ang < 0) {
        return ang + PI * 2.0;
    }
    return ang;
}

float dot(const Vector2& a, const Vector2& b) {
    return a.x * b.x + a.y * b.y;
}

// Indicates direction to rotate towards, either -1 or 1
float direction_to_rotate(const Vector2& ahead, const Vector2& mobile) {
    // Use cross product to figure out if we are on the left or right side
    return ahead.x * mobile.y - ahead.y * mobile.x < 0 ? 1 : -1;
}

// Does not work past 180 degrees
float my_angle_from(const Vector2& a, const Vector2& b) {
    const float cos_theta = dot(a, b) / my_length(a) / my_length(b);
    return acos(cos_theta);
}

float my_length(const Vector2& v) {
    return sqrtf((v.x * v.x) + (v.y * v.y));
}

Vector2 my_normalize(const Vector2& v) {
    const float length = my_length(v);
    return Vector2 { v.x / length, v.y / length };
}

Vector2 perp_rhr(const Vector2& v) {
    const float vx = -v.y;
    const float vy = v.x;
    return Vector2 { vx, vy };
}

Vector2 unit_vector(const float& f) {
    return Vector2 { cosf(f), sinf(f) };
}

std::string to_str(const Vector2& v, const int& decimal_pts) {
    char buf[100];
    sprintf(buf, "(%f, %f)", v.x, v.y);
    return std::string(buf);
}