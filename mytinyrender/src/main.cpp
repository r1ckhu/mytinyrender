#include "tgaimage.h"
#include "model.h"
#include <memory>
#include <string>
const TGAColor white = TGAColor{ 255, 255, 255, 255 };
const TGAColor red = TGAColor{ 255,0,0,255 };
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
		float t = (x - x0) / (float)(x1 - x0);
		int y = y0 * (1. - t) + y1 * t;
		if (steep) {
			image.set(y, x, color);
		}
		else {
			image.set(x, y, color);
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