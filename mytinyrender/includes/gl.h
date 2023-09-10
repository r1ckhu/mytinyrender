#pragma once
#include "geometry.h"
#include <memory>
void cal_view_transform(const Vec3f& camera, const Vec3f& lookat, const Vec3f& up, Matrix& view);
void cal_ortho_proj(float l, float r, float t, float b, float n, float f, Matrix& proj);
void cal_viewport_transform(float width, float height, Matrix& viewport);