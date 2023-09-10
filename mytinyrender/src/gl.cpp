#include "gl.h"

void cal_view_transform(const Vec3f& camera, const Vec3f& lookat, const Vec3f& up, Matrix& view)
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

void cal_ortho_proj(float l, float r, float t, float b, float n, float f, Matrix& proj)
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

	proj = scale * trans;
}

void cal_viewport_transform(float width, float height, Matrix& viewport)
{
	// Transform in xy plane: [-1, 1]^2 to [0, width] x [0, height]
	viewport[0][0] = width / 2; viewport[0][1] = 0; viewport[0][2] = 0; viewport[0][3] = width / 2;
	viewport[1][0] = 0; viewport[1][1] = height / 2; viewport[1][2] = 0; viewport[1][3] = height / 2;
	viewport[2][0] = 0; viewport[2][1] = 0; viewport[2][2] = 1; viewport[2][3] = 0;
	viewport[3][0] = 0; viewport[3][1] = 0; viewport[3][2] = 0; viewport[3][3] = 1;
}