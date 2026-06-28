#ifndef MYLIB_H
#define MYLIB_H

#include <string>
#include <random>
#include <array>
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
Vector3 operator + (const Vector3& a, const Vector3& b);
Vector2 operator * (const Vector2& v, const float f);
Vector3 operator * (const Vector3& v, const float f);
Vector2 operator - (const Vector2& a, const Vector2& b);
Vector3 operator - (const Vector3& a, const Vector3& b);
Vector2 operator / (const Vector2& a, const Vector2& b);
Vector2 operator / (const Vector2& v, const float f);
Vector2 operator - (const Vector2& v);
Vector2 rotate(const Vector2& origin, const Vector2& pt, const float amt);
Vector2 normalize(const Vector2& v);
Vector2 perp_rhr(const Vector2& v);
Vector2 unit_vector(const float f);
Vector2 project_pt(const Vector2& pt, const std::array<const Vector2, 2>& onto);
Vector2 rel_dir(const Vector2& v, const Vector2& origin_v);

float direction_to_rotate(const Vector2& ahead, const Vector2& mobile);
float angle(const Vector2& v);
float dot(const Vector2& a, const Vector2& b);
Vector3 v2_to_v3(const Vector2& v, const float z);
Vector3 cross(const Vector3& a, const Vector3& b);
float angle_from(const Vector2& a, const Vector2& b);
float signed_angle_from(const Vector2& a, const Vector2& b);
float length(const Vector2& v);
float dist_pt_from_line(const Vector2& pt, const std::array<const Vector2, 2>& line);
bool pt_in_polygon(const Vector2& pt, const std::vector<Vector2>& polygon);
bool right_side(const Vector2& pt, const std::array<const Vector2, 2>& line);
float dist_from_pt_to_polygon(const Vector2& pt, const std::vector<Vector2>& polygon);
float rhr_sign(const Vector2& a, const Vector2& b, const Vector2& c);
bool is_inside(const Vector2& pt, const std::array<const Vector2, 2>& bounds, const float epsilon);

// Debug
std::string to_str(const Vector2& v, const int decimal_pts);

// Misc
bool pt_in_rect(const Vector2& pt, const Rectangle& rect);
bool is_overlap(const Rectangle& r1, const Rectangle& r2);
Rectangle full_texture(const Texture2D& tex);
Color mix(const Color& a, const Color& b, const float amt);
void set_shader_value(const ShaderWithCheck& shader_with, const char* uniform_name, const void* data, ShaderUniformDataType uniform_type); 
Rectangle to_rect(const Vector2& pos, const Vector2& dims);
struct PosDims {
	Vector2 pos, dims;
};
PosDims to_pos_dims(const Rectangle& rect);
Vector4 to_vec4(const Color& color);

// Math, non vector
float snap(const float f, const float by);

#endif
