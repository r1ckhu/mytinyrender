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

void triangle(Vec2i t0, Vec2i t1, Vec2i t2, TGAImage& image, TGAColor color) {
	std::vector<Vec2i> points{ t0,t1,t2 };
	std::sort(points.begin(), points.end(),
		[](Vec2i& p1, Vec2i& p2)->bool {return p1.y < p2.y; }
	);
	t0 = points[0]; t1 = points[1]; t2 = points[2];
	// draw the lower half of the triangle
	int total_height = t2.y - t0.y;
	for (int y = t0.y; y <= t1.y; y++) {
		int segment_height = t1.y - t0.y + 1;
		float alpha = (float)(y - t0.y) / total_height;
		float beta = (float)(y - t0.y) / segment_height; // be careful with divisions by zero 
		Vec2i A = t0 + (t2 - t0) * alpha;
		Vec2i B = t0 + (t1 - t0) * beta;
		line(A.x, y, B.x, y, image, color);
	}
	// draw the upper half of the triangle
	for (int y = t1.y; y <= t2.y; y++) {
		int segment_height = t2.y - t1.y + 1;
		float alpha = (float)(y - t0.y) / total_height;
		float beta = (float)(y - t1.y) / segment_height; // be careful with divisions by zero 
		Vec2i A = t2 + (t0 - t2) * (1 - alpha);
		Vec2i B = t2 + (t1 - t2) * (1 - beta);
		line(A.x, y, B.x, y, image, color);
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