/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MURPHY3D_MATH_UTILS_H
#define MURPHY3D_MATH_UTILS_H

#include "math/matrix4.h"
#include "math/vector3d.h"
#include "math/angle.h"


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

inline Math::Vector3d projectXZ(const Math::Vector3d &p1, const Math::Vector3d &p2, const Math::Vector3d &p3) {
	double px = p2.x() - p1.x();
	double pz = p2.z() - p1.z();
	double dAB = px * px + pz * pz;

	double u = ((p3.x() - p1.x()) * px + (p3.z() - p1.z()) * pz) / dAB;
	return Math::Vector3d(p1.x() + u * px, 0.0f, p1.z() + u * pz);
}

inline double lineSegmentsDistanceXZ(const Math::Vector3d &p1, const Math::Vector3d &p2,
									 const Math::Vector3d &p3, const Math::Vector3d &p4,
									 double &t, double &uc) {
	Math::Vector3d u = p2 - p1;
	Math::Vector3d v = p4 - p3;
	Math::Vector3d w = p1 - p3;

	double a = Math::Vector3d::dotProduct(u, u);
	double b = Math::Vector3d::dotProduct(u, v);
	double c = Math::Vector3d::dotProduct(v, v);
	double d = Math::Vector3d::dotProduct(u, w);
	double e = Math::Vector3d::dotProduct(v, w);
	double D = a * c - b * b;

	double sc, sN, sD = D;
	double tc, tN, tD = D;

	double SMALL_NUM = 0.00000001;

	if (D < SMALL_NUM) {
		// the lines are almost perfectly parallel.
		// force the check to point 0 on the first segment.
		sN = 0.0;
		sD = 1.0;
		// recalculate t by fixing s as 0
		tN = e;
		tD = c;
	} else {
		// get the closest points on the infinite lines
		sN = (b * e - c * d);
		tN = (a * e - b * d);

		// clamp the closest point to the bounds of segment 1 (0.0 to 1.0)
		if (sN < 0.0) {
			sN = 0.0;
			tN = e;
			tD = c; 
		} else if (sN > sD) {
			sN = sD;
			// recalculate t by fixing s as 1
			tN = e + b;
			tD = c;
		}
	}

	// clamp the closest point to the bounds of segment 2 (0.0 to 1.0)
	if (tN < 0.0) {
		tN = 0.0;
		if (-d < 0.0) {
			sN = 0.0;
		} else if (-d > a) {
			sN = sD;
		} else {
			sN = -d;
			sD = a;
		}
	} else if (tN > tD) {
		tN = tD;
		if ((-d + b) < 0.0) {
			sN = 0.0;
		} else if ((-d + b) > a) {
			sN = sD;
		} else {
			sN = (-d + b);
			sD = a;
		}
	}
	// calculate scale factors for both line segments
	sc = (fabs(sN) < SMALL_NUM ? 0.0 : sN / sD);
	tc = (fabs(tN) < SMALL_NUM ? 0.0 : tN / tD);

	// calculate the exact vector pointing from the closest point on line 1 to the closest point on line 2
	Math::Vector3d dP(
		(w.x() + (u.x() * sc) - (v.x() * tc)),
		0.0f,
		(w.z() + (u.z() * sc) - (v.z() * tc)));

	t = sc;
	uc = tc;

	// return the length of that vector (which is the shortest distance)
	return dP.length();
}

} // End of namespace MathUtils
} // End of namespace Murphy3d

#endif // MURPHY3D_MATH_UTILS_H
