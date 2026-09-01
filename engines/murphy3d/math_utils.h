// murphy3d/math_utils.h
#ifndef MURPHY3D_MATH_UTILS_H
#define MURPHY3D_MATH_UTILS_H

#include "math/matrix4.h"
#include "math/vector3d.h"
#include "math/angle.h"
#include <math.h>

namespace Murphy3d {
namespace MathUtils {
inline Math::Matrix4 lookAtLH(const Math::Vector3d &eye, const Math::Vector3d &at, const Math::Vector3d &up) {
	Math::Vector3d zaxis = (at - eye);
	zaxis.normalize();

	Math::Vector3d xaxis = Math::Vector3d::crossProduct(up, zaxis);
	xaxis.normalize();

	Math::Vector3d yaxis = Math::Vector3d::crossProduct(zaxis, xaxis);

	Math::Matrix4 mat;

	// Row 0
	mat(0, 0) = xaxis.x();
	mat(0, 1) = yaxis.x();
	mat(0, 2) = zaxis.x();
	mat(0, 3) = 0.0f;

	// Row 1
	mat(1, 0) = xaxis.y();
	mat(1, 1) = yaxis.y();
	mat(1, 2) = zaxis.y();
	mat(1, 3) = 0.0f;

	// Row 2
	mat(2, 0) = xaxis.z();
	mat(2, 1) = yaxis.z();
	mat(2, 2) = zaxis.z();
	mat(2, 3) = 0.0f;

	// Row 3
	mat(3, 0) = -Math::Vector3d::dotProduct(xaxis, eye);
	mat(3, 1) = -Math::Vector3d::dotProduct(yaxis, eye);
	mat(3, 2) = -Math::Vector3d::dotProduct(zaxis, eye);
	mat(3, 3) = 1.0f;

	return mat;
}

inline Math::Matrix4 orthographicLH(float w, float h, float zn, float zf) {
	Math::Matrix4 mat;
	mat(0, 0) = 2.0f / w;
	mat(0, 1) = 0.0f;
	mat(0, 2) = 0.0f;
	mat(0, 3) = 0.0f;
	mat(1, 0) = 0.0f;
	mat(1, 1) = 2.0f / h;
	mat(1, 2) = 0.0f;
	mat(1, 3) = 0.0f;
	mat(2, 0) = 0.0f;
	mat(2, 1) = 0.0f;
	mat(2, 2) = 2.0f / (zf - zn);
	mat(2, 3) = 0.0f;
	mat(3, 0) = 0.0f;
	mat(3, 1) = 0.0f;
	mat(3, 2) = -(zf + zn) / (zf - zn);
	mat(3, 3) = 1.0f;
	return mat;
}

inline Math::Matrix4 perspectiveFovLH(float fovY, float aspect, float zn, float zf) {
	float yScale = 1.0f / tanf(fovY / 2.0f);
	float xScale = yScale / aspect;
	
	Math::Matrix4 mat;
	mat(0, 0) = xScale;
	mat(0, 1) = 0.0f;
	mat(0, 2) = 0.0f;
	mat(0, 3) = 0.0f;
	mat(1, 0) = 0.0f;
	mat(1, 1) = yScale;
	mat(1, 2) = 0.0f;
	mat(1, 3) = 0.0f;
	mat(2, 0) = 0.0f;
	mat(2, 1) = 0.0f;
	mat(2, 2) = (zf + zn) / (zf - zn);
	mat(2, 3) = 1.0f;
	mat(3, 0) = 0.0f;
	mat(3, 1) = 0.0f;
	mat(3, 2) = -(2.0f * zn * zf) / (zf - zn);
	mat(3, 3) = 0.0f;
	return mat;
}

inline Math::Matrix4 translation(float x, float y, float z) {
	Math::Matrix4 mat;
	mat.setToIdentity();
	mat(3, 0) = x;
	mat(3, 1) = y;
	mat(3, 2) = z;
	return mat;
}

inline Math::Matrix4 rotationX(float angle) {
	Math::Matrix4 mat;
	mat.setToIdentity();
	float c = cosf(angle);
	float s = sinf(angle);
	mat(1, 1) = c;
	mat(1, 2) = s;
	mat(2, 1) = -s;
	mat(2, 2) = c;
	return mat;
}

inline Math::Matrix4 rotationY(float angle) {
	Math::Matrix4 mat;
	mat.setToIdentity();
	float c = cosf(angle);
	float s = sinf(angle);
	mat(0, 0) = c;
	mat(0, 2) = -s;
	mat(2, 0) = s;
	mat(2, 2) = c;
	return mat;
}

inline Math::Matrix4 rotationZ(float angle) {
	Math::Matrix4 mat;
	mat.setToIdentity();
	float c = cosf(angle);
	float s = sinf(angle);
	mat(0, 0) = c;
	mat(0, 1) = s;
	mat(1, 0) = -s;
	mat(1, 1) = c;
	return mat;
}

inline bool intersectsTriangle(const Math::Vector3d &origin, const Math::Vector3d &direction,
							   const Math::Vector3d &v0, const Math::Vector3d &v1, const Math::Vector3d &v2, float &dist) {
	Math::Vector3d e1 = v1 - v0;
	Math::Vector3d e2 = v2 - v0;
	Math::Vector3d p = Math::Vector3d::crossProduct(direction, e2);
	float det = Math::Vector3d::dotProduct(e1, p);

	if (det > -0.000001f && det < 0.000001f)
		return false;
	float invDet = 1.0f / det;

	Math::Vector3d t = origin - v0;
	float u = Math::Vector3d::dotProduct(t, p) * invDet;
	if (u < 0.0f || u > 1.0f)
		return false;

	Math::Vector3d q = Math::Vector3d::crossProduct(t, e1);
	float v = Math::Vector3d::dotProduct(direction, q) * invDet;
	if (v < 0.0f || u + v > 1.0f)
		return false;

	dist = Math::Vector3d::dotProduct(e2, q) * invDet;
	if (dist > 0.000001f)
		return true;

	return false;
}

} // End of namespace MathUtils
} // End of namespace Murphy3d

#endif // MURPHY3D_MATH_UTILS_H
