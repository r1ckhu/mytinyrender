#include "gl.h"
Matrix view, projection, viewport;
Matrix ortho, persp;
std::ofstream debug_log;
Vec4f vec3f_to_vec4f(Vec3f v, bool is_point) {
	Vec4f result;
	result[0] = v.x;
	result[1] = v.y;
	result[2] = v.z;
	result[3] = (is_point) ? 1 : 0;
	return result;
}

Vec3f vec4f_to_vec3f(Vec4f v) {
	return Vec3f{ v[0],v[1],v[2] };
}

void line(int x0, int y0, int x1, int y1, TGAImage& image, const TGAColor color) {
	// TODO: forget about optimization for now 
	bool steep = false;
	if (std::abs(x1 - x0) < std::abs(y1 - y0)) {
		std::swap(x0, y0);
		std::swap(x1, y1);
		steep = true;
	}
	if (x0 > x1) {
		std::swap(x0, x1);
		std::swap(y0, y1);
	}
	for (int x = x0; x <= x1; x++) {
		if (x1 - x0 == 0) {
			image.set(x, y0, color);
			continue;
		}
		float t = (x - x0) / (float)(x1 - x0);
		// if use "y0 * (1. - t) + y1 * t" will have problem with float
		int y = y0 + (y1 - y0) * t;
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
		}
	}
}

Vec3f world2screen(Vec4f v) {
	// v.x and v.y should be in [-1,1]
	return  vec4f_to_vec3f(viewport * v);
}

Vec3f barycentric(std::vector<Vec4f>& pts, Vec4f p) {
	Vec3f x{ (float)(pts[1] - pts[0])[0], (float)(pts[2] - pts[0])[0], (float)(pts[0] - p)[0] };
	Vec3f y{ (float)(pts[1] - pts[0])[1], (float)(pts[2] - pts[0])[1], (float)(pts[0] - p)[1] };
	Vec3f temp = cross(x, y); // result = k[u, v, 1];
	if (std::abs(temp.z) < 1) { // which means temp.z is zero
		// in this case the triangle is degenerate to a line
		// we will assume the point p is not in the triangle
		// so return any Vec3f that has a negative value in it
		return Vec3f(-1, 1, 1);
	}
	// the return value should be [1-u-v, u, v]
	float u = temp.x / temp.z;
	float v = temp.y / temp.z;
	Vec3f a = Vec3f{ 1.f - u - v,u,v };
	Vec3f b = Vec3f(1.f - (temp.x + temp.y) / temp.z, temp.x / temp.z, temp.y / temp.z);
	// theoretically a and b should be the same, however in some cases there will be a difference
	// between a value that is close to zero(but it is negative) and a value that is zero
	// which will create holes when filling the triangle
	return b;
}

void triangle(std::vector<Vec4f>& pts, shader_t& shader, std::vector<float>& z_buffer, TGAImage& image) {
	Vec2f bboxmin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
	Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
	Vec2f clamp(image.get_width() - 1, image.get_height() - 1);
	for (int i = 0; i < 3; i++) {
		// the outer std::max and std::min ignore the bounding box outside the screen
		bboxmin.x = std::max(0.f, std::min(bboxmin.x, pts[i][0]));
		bboxmin.y = std::max(0.f, std::min(bboxmin.y, pts[i][1]));

		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i][0]));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i][1]));
	}
	Vec2i p;
	TGAColor color;
	for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++) {
		for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++) {
			Vec4f pf; pf[0] = p.x; pf[1] = p.y; pf[2] = 1; pf[3] = 1;
			if (p.x == 555 && p.y == 935) {
				int debug = 1;
			}
			Vec3f bc_screen = barycentric(pts, pf);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
				continue;
			}
			float p_z = 0.f;
			// interpolate the z value
			//for (int i = 0; i < 3; i++) {
			//	p_z += pts[i][2] * bc_screen[i];
			//}
			p_z = Vec3f(pts[0][2], pts[1][2], pts[2][2]) * bc_screen;
			if (z_buffer[int(p.x + p.y * image.get_width())] < p_z) {
				z_buffer[int(p.x + p.y * image.get_width())] = p_z;
				bool discard = shader.fragment(bc_screen, color);
				if (!discard) {
					image.set(p.x, p.y, color);
				}
			}
		}
	}
}

void cal_view_transform(const Vec3f& camera, const Vec3f& lookat, const Vec3f& up)
{
	Matrix trans; // translation matrix
	trans[0][0] = 1; trans[0][1] = 0; trans[0][2] = 0; trans[0][3] = -camera.x;
	trans[1][0] = 0; trans[1][1] = 1; trans[1][2] = 0; trans[1][3] = -camera.y;
	trans[2][0] = 0; trans[2][1] = 0; trans[2][2] = 1; trans[2][3] = -camera.z;
	trans[3][0] = 0; trans[3][1] = 0; trans[3][2] = 0; trans[3][3] = 1;

	Vec3f g_t = cross(lookat, up);

	Matrix ro; // rotation matrix
	ro[0][0] = g_t.x; ro[0][1] = g_t.y; ro[0][2] = g_t.z; ro[0][3] = 0;
	ro[1][0] = up.x; ro[1][1] = up.y; ro[1][2] = up.z; ro[1][3] = 0;
	ro[2][0] = -lookat.x; ro[2][1] = -lookat.y; ro[2][2] = -lookat.z; ro[2][3] = 0;
	ro[3][0] = 0; ro[3][1] = 0; ro[3][2] = 0; ro[3][3] = 1;

	view = ro * trans;
}

void cal_ortho_proj(float l, float r, float t, float b, float n, float f)
{
	Matrix trans; // translation matrix
	trans[0][0] = 1; trans[0][1] = 0; trans[0][2] = 0; trans[0][3] = -(r + l) / 2;
	trans[1][0] = 0; trans[1][1] = 1; trans[1][2] = 0; trans[1][3] = -(t + b) / 2;
	trans[2][0] = 0; trans[2][1] = 0; trans[2][2] = 1; trans[2][3] = -(n + f) / 2;
	trans[3][0] = 0; trans[3][1] = 0; trans[3][2] = 0; trans[3][3] = 1;

	Matrix scale;
	scale[0][0] = 2 / (r - l); scale[0][1] = 0; scale[0][2] = 0; scale[0][3] = 0;
	scale[1][0] = 0; scale[1][1] = 2 / (t - b); scale[1][2] = 0; scale[1][3] = 0;
	scale[2][0] = 0; scale[2][1] = 0; scale[2][2] = 2 / (n - f); scale[2][3] = 0;
	scale[3][0] = 0; scale[3][1] = 0; scale[3][2] = 0; scale[3][3] = 1;

	ortho = scale * trans;
}

void cal_persp_proj(float n, float f)
{
	persp[0][0] = n; persp[0][1] = 0; persp[0][2] = 0; persp[0][3] = 0;
	persp[1][0] = 0; persp[1][1] = n; persp[1][2] = 0; persp[1][3] = 0;
	persp[2][0] = 0; persp[2][1] = 0; persp[2][2] = n + f; persp[2][3] = -n * f;
	persp[3][0] = 0; persp[3][1] = 0; persp[3][2] = 1; persp[3][3] = 0;
}

void cal_viewport_transform(float width, float height)
{
	// Transform in xy plane: [-1, 1]^2 to [0, width] x [0, height]
	viewport[0][0] = width / 2; viewport[0][1] = 0; viewport[0][2] = 0; viewport[0][3] = width / 2;
	viewport[1][0] = 0; viewport[1][1] = height / 2; viewport[1][2] = 0; viewport[1][3] = height / 2;
	viewport[2][0] = 0; viewport[2][1] = 0; viewport[2][2] = 1; viewport[2][3] = 0;
	viewport[3][0] = 0; viewport[3][1] = 0; viewport[3][2] = 0; viewport[3][3] = 1;
}

void init_camera(Vec3f camera, Vec3f look_at, Vec3f up, float fovY_deg, Vec2f nf, Vec2f aspect, Vec2i screen, bool is_persp)
{
	const float PI = 3.14159265;
	cal_view_transform(camera, look_at, up);
	float b = std::tan(fovY_deg / 2 * PI / 180.0) * nf[0];
	float t = -b;
	float r = t / aspect[1] * aspect[0];
	float l = -r;
	cal_ortho_proj(l, r, t, b, nf[0], nf[1]);
	projection = ortho;
	if (is_persp) {
		cal_persp_proj(nf[0], nf[1]);
		projection = ortho * persp;
	}
	cal_viewport_transform(screen[0], screen[1]);
}


