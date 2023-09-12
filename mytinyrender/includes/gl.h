#pragma once
#include "geometry.h"
#include "tgaimage.h"
#include <memory>
#include <fstream>
extern std::ofstream debug_log;
extern Matrix view, projection, viewport;
extern Matrix ortho, persp;
class shader_t
{
public:
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	virtual bool fragment(Vec3f bar, TGAColor& color) = 0; // "bar" stands for barycentric
};

void cal_view_transform(const Vec3f& camera, const Vec3f& lookat, const Vec3f& up);
void cal_ortho_proj(float l, float r, float t, float b, float n, float f);
void cal_persp_proj(float n, float f);
void cal_viewport_transform(float width, float height);
void init_camera(Vec3f camera, Vec3f look_at, Vec3f up, float fovY_deg, Vec2f nf, Vec2f aspect, Vec2i screen, bool is_persp);
void triangle(std::vector<Vec4f>& pts, shader_t& shader, std::vector<float>& z_buffer, TGAImage& image);
Vec4f vec3f_to_vec4f(Vec3f v, bool is_point = true);
Vec3f vec4f_to_vec3f(Vec4f v);