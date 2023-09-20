#include "tgaimage.h"
#include "model.h"
#include "gl.h"
#include <memory>
#include <string>
#include <algorithm>
#include <assert.h>
const TGAColor white = TGAColor{ 255, 255, 255, 255 };
const TGAColor red = TGAColor{ 255,0,0,255 };
const TGAColor green = TGAColor{ 0,255,0,255 };
const TGAColor blue = TGAColor{ 0,0,255,255 };
const std::string DEFAULT_OBJ_PATH = std::string("obj/african_head/african_head.obj");
const int IMG_WIDTH = 1200;
const int IMG_HEIGHT = 1200;
std::unique_ptr<Model> model;
std::unique_ptr<std::vector<float>> z_buffer;
std::unique_ptr<std::vector<float>> shadow_buffer;

std::unique_ptr<TGAImage> image, depth;

Vec3f light_dir = { -1,-1,-1 };
Vec3f camera_pos = { 0,0,3 };
Vec3f look_at = { 0,0,-1 };
Vec3f up = { 0,1,0 }; // prep to look_at
Vec2f nf = { -1,-10 };
float fovY_deg = 50;

struct gouraud_shader_t : shader_t
{
	Vec3f varying_intensity; // written by vertex shader, read by fragment shader
	mat<2, 3, float> varying_uv;

	Vec4f vertex(int iface, int nthvert) final {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec3f v = model->vert(iface, nthvert);
		Vec4f gl_vertex = vec3f_to_vec4f(v);
		Vec4f after_view = view * gl_vertex;
		Vec4f after_persp = persp * after_view;
		Vec4f after_ortho = ortho * after_persp;
		gl_vertex = viewport * projection * view * gl_vertex;
		// do not divide gl_vertex[3] here, delay it to triangle()
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir * -1);
		return gl_vertex;
	}
	bool fragment(Vec3f bar, TGAColor& color) final {
		float intensity = varying_intensity * bar;
		Vec2f uv = varying_uv * bar;
		color = model->diffuse(uv);
		color = TGAColor(color.r * intensity, color.g * intensity, color.b * intensity, 255);
		return false;                              // no, we do not discard this pixel
	}
};

struct toon_shader_t : shader_t
{
	Vec3f varying_intensity; // written by vertex shader, read by fragment shader
	mat<2, 3, float> varying_uv;

	Vec4f vertex(int iface, int nthvert) final {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec3f v = model->vert(iface, nthvert);
		Vec4f gl_vertex = vec3f_to_vec4f(v);
		gl_vertex = viewport * projection * view * gl_vertex;
		// do not divide gl_vertex[3] here, delay it to triangle()
		varying_intensity[nthvert] = std::max(0.f, model->normal(iface, nthvert) * light_dir * -1);
		return gl_vertex;
	}
	bool fragment(Vec3f bar, TGAColor& color) final {
		float intensity = varying_intensity * bar;
		if (intensity > .85) intensity = 1;
		else if (intensity > .60) intensity = .80;
		else if (intensity > .45) intensity = .60;
		else if (intensity > .30) intensity = .45;
		else if (intensity > .15) intensity = .30;
		else intensity = 0;
		color = TGAColor(255, 155, 0) * intensity;
		return false;
	}
};

struct normal_mapping_shader_t : shader_t
{
	mat<2, 3, float> varying_uv;
	mat<4, 4, float> uniform_M;   //  Projection*View
	mat<4, 4, float> uniform_MIT; // (Projection*View).invert_transpose()

	Vec4f vertex(int iface, int nthvert) final {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_vertex = vec3f_to_vec4f(model->vert(iface, nthvert));
		gl_vertex = viewport * projection * view * gl_vertex;
		// do not divide gl_vertex[3] here, delay it to triangle()
		return gl_vertex;
	}
	bool fragment(Vec3f bar, TGAColor& color) final {
		Vec2f uv = varying_uv * bar;
		Vec3f n = vec4f_to_vec3f(uniform_MIT * vec3f_to_vec4f(model->normal(uv))).normalize();
		Vec3f l = vec4f_to_vec3f(uniform_M * vec3f_to_vec4f(light_dir)).normalize();
		float intensity = std::max(0.f, n * l * -1);
		color = model->diffuse(uv) * intensity;
		return false;                              // no, we do not discard this pixel
	}
};

struct phone_shader_t : shader_t
{
	mat<2, 3, float> varying_uv;
	mat<4, 4, float> uniform_M;   //  Projection*View
	mat<4, 4, float> uniform_MIT; // (Projection*View).invert_transpose()

	Vec4f vertex(int iface, int nthvert) final {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_vertex = vec3f_to_vec4f(model->vert(iface, nthvert));
		gl_vertex = viewport * projection * view * gl_vertex;
		// do not divide gl_vertex[3] here, delay it to triangle()
		return gl_vertex;
	}
	bool fragment(Vec3f bar, TGAColor& color) final {
		Vec2f uv = varying_uv * bar;
		Vec3f n = vec4f_to_vec3f(uniform_MIT * vec3f_to_vec4f(model->normal(uv))).normalize();
		Vec3f l = vec4f_to_vec3f(uniform_M * vec3f_to_vec4f(light_dir)).normalize();
		Vec3f h = vec4f_to_vec3f(uniform_M * vec3f_to_vec4f((light_dir + look_at) * -1)).normalize();
		float specular = std::pow(std::max(0.f, n * h), model->specular(uv));
		float diffuse = std::max(0.f, n * l * -1);
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) {
			color[i] = std::min<float>(5 + color[i] * (diffuse + .8 * specular), 255);
		}
		return false;                              // no, we do not discard this pixel
	}
};

float min_z = std::numeric_limits<float>::max(), max_z = std::numeric_limits<float>::lowest();
struct shadow_shader_t : shader_t
{
	mat<3, 3, float> varying_tri;
	Vec4f vertex(int iface, int nthvert) {
		Vec4f gl_vertex = embed<4>(model->vert(iface, nthvert)); // read the vertex from .obj file
		Vec4f after_view = view * gl_vertex;
		min_z = std::min(min_z, after_view[2]);
		max_z = std::max(max_z, after_view[2]);
		Vec4f after_projection = projection * after_view;
		Vec4f after_viewport = viewport * after_projection;
		gl_vertex = viewport * projection * view * gl_vertex;          // transform it to screen coordinates
		varying_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));
		return gl_vertex;
	}

	bool fragment(Vec3f bar, TGAColor& color) {
		Vec3f p = varying_tri * bar;
		color = TGAColor(255, 255, 255) * ((p.z + 1.) / 2.);
		// debug_log << p.z << ' ' << color.r << std::endl;
		return false;
	}
};

struct shadow_mapping_shader_t : shader_t
{
	mat<2, 3, float> varying_uv;
	mat<3, 3, float> varying_tri; // triangle coordinates before Viewport transform, written by VS, read by FS
	mat<4, 4, float> uniform_M;   //  Projection*View
	mat<4, 4, float> uniform_MIT; // (Projection*View).invert_transpose()
	mat<4, 4, float> uniform_Mshadow; // transform framebuffer screen coordinates to shadowbuffer screen coordinates
	Vec4f vertex(int iface, int nthvert) final {
		varying_uv.set_col(nthvert, model->uv(iface, nthvert));
		Vec4f gl_vertex = vec3f_to_vec4f(model->vert(iface, nthvert));
		gl_vertex = viewport * projection * view * gl_vertex;
		varying_tri.set_col(nthvert, proj<3>(gl_vertex / gl_vertex[3]));
		// do not divide gl_vertex[3] here, delay it to triangle()
		return gl_vertex;
	}
	bool fragment(Vec3f bar, TGAColor& color) final {
		Vec4f sb_p = uniform_Mshadow * embed<4>(varying_tri * bar);
		sb_p = sb_p / sb_p[3]; // point on the shadow buffer
		int idx = int(sb_p[0]) + int(sb_p[1]) * IMG_WIDTH; // index in the shadowbuffer array
		float shadow = .3 + .7 * ((*shadow_buffer)[idx] < sb_p[2] + 43.34);

		Vec2f uv = varying_uv * bar;
		Vec3f n = vec4f_to_vec3f(uniform_MIT * vec3f_to_vec4f(model->normal(uv))).normalize();
		Vec3f l = vec4f_to_vec3f(uniform_M * vec3f_to_vec4f(light_dir)).normalize();
		Vec3f h = vec4f_to_vec3f(uniform_M * vec3f_to_vec4f((light_dir + look_at) * -1)).normalize();
		float specular = std::pow(std::max(0.f, n * h), model->specular(uv));
		float diffuse = std::max(0.f, n * l * -1);
		color = model->diffuse(uv);
		for (int i = 0; i < 3; i++) {
			color[i] = std::min<float>(20 + color[i] * shadow * (1.2 * diffuse + .8 * specular), 255);
		}
		return false;                              // no, we do not discard this pixel
	}
};

void construct_shadow_buffer()
{
	shadow_buffer = std::make_unique<std::vector<float>>(IMG_WIDTH * IMG_HEIGHT, std::numeric_limits<float>::lowest());
	init_camera(Vec3f(5, 5, 5), light_dir, Vec3f(-1, 2, -1), 30, Vec2f(-12, -17), Vec2f(1, 1), Vec2i(IMG_WIDTH, IMG_HEIGHT), true);

	shadow_shader_t depthshader;
	std::vector<Vec4f> shadow_buffer_coords(3);
	for (int i = 0; i < model->nfaces(); i++) {
		for (int j = 0; j < 3; j++) {
			shadow_buffer_coords[j] = depthshader.vertex(i, j);
		}
		triangle(shadow_buffer_coords, depthshader, *shadow_buffer, *depth);
	}
	depth->flip_vertically(); // to place the origin in the bottom left corner of the image
	depth->write_tga_file("depth.tga");
}


int main(int argc, char** argv)
{
	if (argc == 2) {
		model = std::make_unique<Model>(argv[1]);
	}
	else {
		model = std::make_unique<Model>(DEFAULT_OBJ_PATH.c_str());
	}
	image = std::make_unique<TGAImage>(IMG_WIDTH, IMG_HEIGHT, TGAImage::RGB);
	depth = std::make_unique<TGAImage>(IMG_WIDTH, IMG_HEIGHT, TGAImage::RGB);

	z_buffer = std::make_unique<std::vector<float>>(IMG_WIDTH * IMG_HEIGHT, std::numeric_limits<float>::lowest());
	//debug_log.open("log.txt");

	construct_shadow_buffer();
	std::cout << min_z << ' ' << max_z << std::endl;
	mat<4, 4, float> M = viewport * projection * view;

	init_camera(camera_pos, look_at, up, fovY_deg, nf, Vec2f{ 1, 1 }, Vec2i{ IMG_WIDTH, IMG_HEIGHT }, true);

	shadow_mapping_shader_t shader;
	shader.uniform_M = projection * view;
	shader.uniform_MIT = shader.uniform_M.invert_transpose();
	shader.uniform_Mshadow = M * (viewport * shader.uniform_M).invert();
	// toon_shader_t shader;
	for (int i = 0; i < model->nfaces(); i++) {
		std::vector<Vec4f> screen_coords(3);
		for (int j = 0; j < 3; j++) {
			screen_coords[j] = shader.vertex(i, j);
		}
		triangle(screen_coords, shader, *z_buffer, *image);
	}
	image->flip_vertically();
	image->write_tga_file("output.tga");
	// debug_log.close();
	return 0;
}