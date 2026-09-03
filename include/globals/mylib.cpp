#include <cstdio>
#include <math.h>
#include <format>

#include "constants.cpp"
#include "mylib.hpp"

DeferThis::~DeferThis() {
	for (int i = to_run.size() - 1; i >= 0; i--)
		to_run[i]();
}

// An ArenaAllocator idea could be made, referenced hexmerge game for inspiration.
DeferThis& DeferThis::and_this(std::function<void()> run) {
	to_run.push_back(run);
	return *this;
}

void ShaderWithCheck::unload_shader() {
	load_unloads--;
	UnloadShader(*this);
}

void ShaderWithCheck::load_shader(const char* filename) {
	load_unloads++;
	Shader shader = LoadShader(0, TextFormat(filename, Constants::glsl_version));
	// Sure I guess that works
	id = shader.id;
	locs = shader.locs;
}

void TextureWithCheck::unload_texture() {
	load_unloads--;
	UnloadTexture(*this);
}

void TextureWithCheck::load_texture(const char* filename) {
	load_unloads++;
	Texture2D tex = LoadTexture(filename);
	format = tex.format;
	height = tex.height;
	id = tex.id;
	mipmaps = tex.mipmaps;
	width = tex.width;
}

void TextureWithCheck::load_texture_from_image(Image image) {
	load_unloads++;
	Texture2D tex = LoadTextureFromImage(image);
	format = tex.format;
	height = tex.height;
	id = tex.id;
	mipmaps = tex.mipmaps;
	width = tex.width;
}

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

float Rand::gen(float a, float b) {
	const float norm = dist(int_gen);
	return norm * (b - a) + a;
}

Vector2 Vector2I::to_vec2() {
	return { (float) x, (float) y };
}

void operator += (Vector2& a, const Vector2& b) {
	a.x += b.x;
	a.y += b.y;
}

Vector2 operator + (const Vector2& a, const Vector2& b) {
	return { a.x + b.x, a.y + b.y };
}

Vector3 operator + (const Vector3& a, const Vector3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}

Vector2 operator * (const Vector2& v, const float f) {
	return { v.x * f, v.y * f };
}

Vector3 operator * (const Vector3& v, const float f) {
	return { v.x * f, v.y * f, v.z * f };
}

Vector2 operator - (const Vector2& a, const Vector2& b) {
	return { a.x - b.x, a.y - b.y };
}

Vector3 operator - (const Vector3& a, const Vector3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Vector2 operator - (const Vector2& v) {
	return { -v.x, -v.y };
}

Vector2 operator / (const Vector2& a, const Vector2& b) {
	return { a.x / b.x, a.y / b.y };
}

Vector2 operator / (const Vector2& v, const float f) {
	return { v.x / f, v.y / f };
}

Vector2 rotate(const Vector2& origin, const Vector2& pt, const float amt) {
	const Vector2 out = pt - origin;
	return Vector2 { 
		out.x * cos(amt) - out.y * sin(amt),
		out.x * sin(amt) + out.y * cos(amt)
	} + origin;
}

float angle(const Vector2& v) {
	const float ang = atan2f(v.y, v.x);
	if (ang < 0) {
		return ang + PI * 2.0;
	}
	return ang;
}

float dot(const Vector2& a, const Vector2& b) {
	return a.x * b.x + a.y * b.y;
}

Vector3 v2_to_v3(const Vector2& v, const float z) {
	return { v.x, v.y, z };
}

Vector3 cross(const Vector3& a, const Vector3& b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

float dist_pt_from_line(const Vector2& pt, const std::array<const Vector2, 2>& line) {
	const Vector2 to_point = line[0] - pt;
	const Vector2 out_v = perp_rhr(line[1] - line[0]);
	return abs(dot(to_point, out_v) / length(out_v));
}

bool pt_in_polygon(const Vector2& pt, const std::vector<Vector2>& polygon) {
	for (size_t i = 0; i < polygon.size(); i++) {
		const Vector2 a = polygon[i];
		const Vector2 b = polygon[(i + 1) % polygon.size()];
		if (rhr_sign(a, b, pt) <= 0)
			return false;
	}
	return true;
}

bool right_side(const Vector2& pt, const std::array<const Vector2, 2>& line) {
	return
		(line[1].x - line[0].x) * (pt.y - line[0].y) > (line[1].y - line[0].y) * (pt.x - line[0].x);
}

float dist_from_pt_to_polygon(const Vector2& pt, const std::vector<Vector2>& polygon) {
	if (pt_in_polygon(pt, polygon))
		return -1;

	for (size_t i = 0; i < polygon.size(); i++) {
		Vector2 a = polygon[i];
		Vector2 b = polygon[(i + 1) % polygon.size()];

		// Ensure that sidedness is correct
		if (right_side(pt, { a, b }))
			continue;

		Vector2 proj = project_pt(pt, { a, b });
		if (std::max(length(proj - a), length(proj - b)) > length(a - b))
			continue;

		return dist_pt_from_line(pt, { a, b });
	}
	float dist = INFINITY;
	for (size_t i = 0; i < polygon.size(); i++)
		dist = std::min(length(pt - polygon[i]), dist);
	return dist;
}

float rhr_sign(const Vector2& a, const Vector2& b, const Vector2& c) {
	const Vector3 v1 = v2_to_v3(a - b, 0);
	const Vector3 v2 = v2_to_v3(b - c, 0);
	return cross(v1, v2).z > 0 ? 1 : -1;
}

bool is_inside(const Vector2& pt, const std::array<const Vector2, 2>& bounds, const float epsilon) {
	return (fmin(bounds[0].x, bounds[1].x) + epsilon < pt.x) && (pt.x < fmax(bounds[0].x, bounds[1].x) - epsilon)
		&& (fmin(bounds[0].y, bounds[1].y) + epsilon < pt.y) && (pt.y < fmax(bounds[0].y, bounds[1].y) - epsilon);
}

// Indicates direction to rotate towards, either -1 or 1
float direction_to_rotate(const Vector2& ahead, const Vector2& mobile) {
	// Use cross product to figure out if we are on the left or right side
	return ahead.x * mobile.y - ahead.y * mobile.x < 0 ? 1 : -1;
}

// Does not work past 180 degrees
// Normally, this returns only a positive number
// If a ~= b can return NaN I think?
float angle_from(const Vector2& a, const Vector2& b) {
	const float cos_theta = dot(a, b) / length(a) / length(b);
	return acos(fmax(fmin(cos_theta, 1), 0));
}

// Hmm...
float signed_angle_from(const Vector2& a, const Vector2& b) {
	// 2D Cross product (gives us the directional sign)
    float cross = a.x * b.y - a.y * b.x; 
    // Dot product (gives us the magnitude cosine)
    float dot_prod = a.x * b.x + a.y * b.y; 

    // atan2 handles the magnitudes and prevents NaN entirely
    return atan2(cross, dot_prod);
}

float length(const Vector2& v) {
	return sqrtf((v.x * v.x) + (v.y * v.y));
}

Vector2 normalize(const Vector2& v) {
	const float len = length(v);
	return { v.x / len, v.y / len };
}

Vector2 perp_rhr(const Vector2& v) {
	return { -v.y, v.x };
}

Vector2 unit_vector(const float f) {
	return { cosf(f), sinf(f) };
}

Vector2 project_pt(const Vector2& pt, const std::array<const Vector2, 2>& onto) {
	Vector2 a = onto[1] - onto[0];
	Vector2 b = pt - onto[0];
	return a * dot(b, a) / (pow(length(a), 2)) + onto[0];
}

Vector2 rel_dir(const Vector2& v, const Vector2& origin_v) {
	// const float theta2 = angle_from(v, origin_v) * rhr_sign(v, { 0, 0 }, origin_v);
	const float theta = signed_angle_from(v, origin_v);
	// std::println("diff {}", theta2 - theta);
	return unit_vector(theta);
}

std::string to_str(const Vector2& v, const int decimal_pts) {
	return std::format("({:.{}f}, {:.{}f})", v.x, decimal_pts, v.y, decimal_pts);
}

Rectangle min_max_to_rect(const Vector2& min_boundary, const Vector2& max_boundary) {
	return {
		min_boundary.x,
		min_boundary.y,
		max_boundary.x - min_boundary.x,
		max_boundary.y - min_boundary.y
	};
}

bool pt_in_rect(const Vector2& pt, const Rectangle& rect) {
	return rect.x <= pt.x && pt.x <= rect.x + rect.width &&
		   rect.y <= pt.y && pt.y <= rect.y + rect.height;
}

bool is_overlap(const Rectangle& r1, const Rectangle& r2) {
	bool r1x_less = r1.x <= r2.x && r2.x <= r1.x + r1.width;
	bool r2x_less = r2.x <= r1.x && r1.x <= r2.x + r2.width;
	bool r1y_less = r1.y <= r2.y && r2.y <= r1.y + r1.height;
	bool r2y_less = r2.y <= r1.y && r1.y <= r2.y + r2.height;
	return (r1x_less || r2x_less) && (r1y_less || r2y_less);
}

Rectangle full_texture(const Texture2D& tex) {
	return {
		.x = 0,
		.y = 0,
		.width = (float) tex.width,
		.height = (float) tex.height
	};
}

Color lerp(const Color& a, const Color& b, const float amt) {
	return {
		.r = (unsigned char) (a.r + amt * (b.r - a.r)),
		.g = (unsigned char) (a.g + amt * (b.g - a.g)),
		.b = (unsigned char) (a.b + amt * (b.b - a.b)),
		.a = (unsigned char) (a.a + amt * (b.a - a.a)),
	};
}

void set_shader_value(const ShaderWithCheck& shader_with, const char* uniform_name, const void* data, ShaderUniformDataType uniform_type) {
	SetShaderValue(shader_with, GetShaderLocation(shader_with, uniform_name), data, uniform_type);
}

Rectangle to_rect(const Vector2& pos, const Vector2& dims) {
	return {
		.x = pos.x,
		.y = pos.y,
		.width = dims.x,
		.height = dims.y
	};
}

PosDims to_pos_dims(const Rectangle& rect) {
	return { 
		.pos = { rect.x, rect.y }, 
		.dims = { rect.width, rect.height }
	};
}

Vector4 to_vec4(const Color& color) {
	return {
		.x = (float) color.r / 255,
		.y = (float) color.g / 255,
		.z = (float) color.b / 255,
		.w = (float) color.a / 255
	};
}

template <typename T>
T get_or(std::optional<T> opt, std::function<T()> supplier) {
	if (!opt)
		return supplier();
	return opt.value();
}

float snap(const float f, const float by) {
	return (float) int(f / by) * by;
}

template <typename T>
T lerp(T start, T end, const float t) {
	return start + (end - start) * t;
}
