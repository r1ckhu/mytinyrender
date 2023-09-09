#include "tgaimage.h"
#include "model.h"
#include <memory>
#include <string>
#include <algorithm>
const TGAColor white = TGAColor{ 255, 255, 255, 255 };
const TGAColor red = TGAColor{ 255,0,0,255 };
const TGAColor green = TGAColor{ 0,255,0,255 };
const TGAColor blue = TGAColor{ 0,0,255,255 };
const std::string DEFAULT_OBJ_PATH = std::string("obj/african_head/african_head.obj");
const int IMG_WIDTH = 1200;
const int IMG_HEIGHT = 1200;

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

Vec3f barycentric(std::vector<Vec2i>& pts, Vec2i p) {
	Vec3f x{ (float)(pts[1] - pts[0]).x, (float)(pts[2] - pts[0]).x, (float)(pts[0] - p).x };
	Vec3f y{ (float)(pts[1] - pts[0]).y, (float)(pts[2] - pts[0]).y, (float)(pts[0] - p).y };
	Vec3f temp = x ^ y; // result = k[u, v, 1];
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

void triangle(std::vector<Vec2i>& pts, TGAImage& image, TGAColor color) {
	Vec2i bboxmin = Vec2i(image.get_width() - 1, image.get_height() - 1);
	Vec2i bboxmax = Vec2i(0, 0);
	Vec2i clamp = bboxmin;
	for (int i = 0; i < 3; i++) {
		// the outer std::max and std::min ignore the bounding box outside the screen
		bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
		bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

		bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
		bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
	}
	Vec2i p;
	for (p.x = bboxmin.x; p.x <= bboxmax.x; p.x++) {
		for (p.y = bboxmin.y; p.y <= bboxmax.y; p.y++) {
			Vec3f bc_screen = barycentric(pts, p);
			if (bc_screen.x < 0 || bc_screen.y < 0 || bc_screen.z < 0) {
				continue;
			}
			image.set(p.x, p.y, color);
		}
	}
}

int main(int argc, char** argv)
{
	std::unique_ptr<Model> model;
	if (argc == 2) {
		model = std::make_unique<Model>(argv[1]);
	}
	else {
		model = std::make_unique<Model>(DEFAULT_OBJ_PATH.c_str());
	}
	TGAImage image{ IMG_WIDTH,IMG_HEIGHT,TGAImage::RGB };

	for (int i = 1; i < model->nfaces(); i++) {
		auto face = model->face(i);
		for (int j = 0; j < 3; j++) {
			auto v0 = model->vert(face[j]);
			auto v1 = model->vert(face[(j + 1) % 3]);
			// transform v0 and v1 in [-1,-1]~[1,1] to [0,0]~[IMG_WIDTH,IMG_HEIGHT]
			int x0 = (v0.x + 1.0) * IMG_WIDTH / 2.0;
			int y0 = (v0.y + 1.0) * IMG_HEIGHT / 2.0;
			int x1 = (v1.x + 1.0) * IMG_WIDTH / 2.0;
			int y1 = (v1.y + 1.0) * IMG_HEIGHT / 2.0;
			line(x0, y0, x1, y1, image, white);
		}
	}

	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}