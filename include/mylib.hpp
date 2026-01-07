#ifndef MYLIB_H
#define MYLIB_H

#include <string>
#include <random>
#include <raylib.h>

// Typedef it with shader if we are sure there's no issues
struct ShaderWithCheck : Shader {
	int load_unloads = 0;
};

void unload_shader(ShaderWithCheck& shader_with); 
void load_shader(ShaderWithCheck& shader_with, const char* filename);

struct TextureWithCheck : Texture2D {
	int load_unloads = 0;
};

void unload_texture(TextureWithCheck& texture_with);
void load_texture(TextureWithCheck& texture_with, const char* filename);
void load_texture_from_image(TextureWithCheck& texture_with, Image image);

struct Vector2I {
	int x, y;
	Vector2I(Vector2 v);
	Vector2I(int x, int y);
	Vector2I();

	Vector2 to_vec2();
};

struct Rand {
	Rand(int seed); 

	int seed;

	void set_seed(int seed);
	float gen(float a, float b);

	private:
	std::mt19937 int_gen;
	std::uniform_real_distribution<double> dist;
};

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
bool pt_in_rect(const Vector2& pt, const Vector2& pos, const Vector2& dim);
bool is_overlap(const Rectangle& r1, const Rectangle& r2);

// Math, non vector
float snap(const float& f, const float& by);

#endif