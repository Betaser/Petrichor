#ifndef MYLIB_H
#define MYLIB_H

#include <string>
#include <random>
#include <raylib.h>
#include <array>

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
	int x = 0;
	int y = 0;
	Vector2I(Vector2 v);
	Vector2I(int x, int y);
	Vector2I();

	Vector2 to_vec2();
};

struct Rand {
	Rand(int seed); 

	int seed = 0;

	void set_seed(int seed);
	float gen(float a, float b);

	private:
	std::mt19937 int_gen;
	std::uniform_real_distribution<double> dist;
};

struct Circle {
	Vector2 pos {};
	float radius = 0;
};

void operator += (Vector2& a, const Vector2& b);
Vector2 operator + (const Vector2& a, const Vector2& b);
Vector2 operator * (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& v, const float& f);
Vector2 operator - (const Vector2& v);
Vector2 my_rotate(const Vector2& origin, const Vector2& pt, const float& amt);
Vector2 my_normalize(const Vector2& v);
Vector2 perp_rhr(const Vector2& v);
Vector2 unit_vector(const float& f);
Vector2 project_pt(const Vector2& pt, const std::array<const Vector2, 2>& onto);

float direction_to_rotate(const Vector2& ahead, const Vector2& mobile);
float my_angle(const Vector2& v);
float dot(const Vector2& a, const Vector2& b);
Vector3 v2_to_v3(const Vector2& v, const float z);
Vector3 cross(const Vector3& a, const Vector3& b);
float my_angle_from(const Vector2& a, const Vector2& b);
float my_length(const Vector2& v);
float dist_pt_from_line(const Vector2& pt, const std::array<const Vector2, 2>& line);
bool pt_in_polygon(const Vector2& pt, const std::vector<Vector2>& polygon);
bool right_side(const Vector2& pt, const std::array<const Vector2, 2>& line);
float dist_from_pt_to_polygon(const Vector2& pt, const std::vector<Vector2>& polygon);

// Debug
std::string to_str(const Vector2& v, const int& decimal_pts);

// Misc
bool pt_in_rect(const Vector2& pt, const Vector2& pos, const Vector2& dim);
bool is_overlap(const Rectangle& r1, const Rectangle& r2);
Rectangle full_texture(const Texture2D& tex);

// Math, non vector
float snap(const float& f, const float& by);

#endif
