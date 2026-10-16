#include "murphy3d/elevation.h"
#include "common/endian.h"
#include "common/util.h"

namespace Murphy3d {

float Elevation::elevationModifier = 1.5f;
float Elevation::elevationCheckModifier = 1.5f;

Elevation::Elevation(const byte *data) {
	// coordinate data starts at offset 12.
	_x1 = (int32)READ_LE_UINT32(data + 12) / 65536.0f;
	_y1 = (int32)READ_LE_UINT32(data + 16) / 65536.0f;
	_z1 = (int32)READ_LE_UINT32(data + 20) / 65536.0f;
	_x2 = (int32)READ_LE_UINT32(data + 24) / 65536.0f;
	_y2 = (int32)READ_LE_UINT32(data + 28) / 65536.0f;
	_z2 = (int32)READ_LE_UINT32(data + 32) / 65536.0f;

	_stepHeight = (int32)READ_LE_UINT32(data + 36) / 65536.0f;
	_stepWidth = (int32)READ_LE_UINT32(data + 44) / 65536.0f;

	_numberOfSteps = (int32)READ_LE_UINT32(data + 48);
	_horizontalLength = (int32)READ_LE_UINT32(data + 52) / 65536.0f;
	_xDirection = (int32)READ_LE_UINT32(data + 56) / 65536.0f;
	_zDirection = (int32)READ_LE_UINT32(data + 60) / 65536.0f;

	// calculate the bounding box for this staircase
	float halfStepWidthX = fabs(_zDirection) * _stepWidth;
	float halfStepWidthZ = fabs(_xDirection) * _stepWidth;

	_x11 = _x1 + halfStepWidthX;
	_x12 = _x1 - halfStepWidthX;
	_x21 = _x2 + halfStepWidthX;
	_x22 = _x2 - halfStepWidthX;
	_z11 = _z1 + halfStepWidthZ;
	_z12 = _z1 - halfStepWidthZ;
	_z21 = _z2 + halfStepWidthZ;
	_z22 = _z2 - halfStepWidthZ;

	_xMin = MIN(MIN(_x11, _x12), MIN(_x21, _x22));
	_xMax = MAX(MAX(_x11, _x12), MAX(_x21, _x22));
	_zMin = MIN(MIN(_z11, _z12), MIN(_z21, _z22));
	_zMax = MAX(MAX(_z11, _z12), MAX(_z21, _z22));
	_yMin = MIN(_y1, _y2);
	_yMax = MAX(_y1, _y2);
}

bool Elevation::isPointInElevation(const Math::Vector3d &pos) const {
	// simple bounding box check to see if Tex is standing on this ramp
	return (pos.x() >= _xMin && pos.x() <= _xMax &&
			pos.z() >= _zMin && pos.z() <= _zMax &&
			(pos.y() + elevationCheckModifier) >= _yMin &&
			(pos.y() - elevationCheckModifier) <= _yMax);
}

float Elevation::getElevationFromXZPosition(const Math::Vector3d &pos) const {
	if (_horizontalLength == 0.0f || _numberOfSteps == 0) {
		return _y1 - elevationModifier;
	}

	// dot product of the position passed in and the end point of the staircase
	// since (_xDirection, _zDirection) is a unit vector which tells us the direction of the staircase
	// this gives us the scalar projection of the passed position onto the staircase direction vector
	double dotPosEnd = (_xDirection * (_x21 - pos.x())) + (_zDirection * (_z21 - pos.z()));
	// calculate how far along the staircase we are (0.0 to 1.0)
	double p = 1.0 - (fabs(dotPosEnd) / _horizontalLength);

	// sp tells us how much length one step occupies
	double sp = 1.0 / _numberOfSteps;
	// t is progress measured in steps covered
	double t = p / sp;
	// t2 removes the decimal part and gives us discrete steps
	// which tells us how many steps we have covered up until now
	double t2 = (double)((int)t);
	// t3 gives us the decimal part and removes the integer part
	// which tells us how far away we are in the current step
	double t3 = t - t2;

	// bend the smooth staircase into flat steps with steep rises
	if (t3 >= 0.2 && t3 < 0.5) {
		t2 += (t3 - 0.2) / 0.3;
	} else if (t3 >= 0.5) {
		t2 += 1.0;
	}

	double d = (_y1 + _stepHeight * t2) - elevationModifier;
	return (float)d;
}

float Elevation::getClosestElevation(float y) const {
	float dy1 = y - (_y1 - 1.0f);
	float dy2 = y - (_y2 - 1.0f);
	return (((dy1 * dy1) < (dy2 * dy2)) ? _y1 : _y2) - elevationModifier;
}

} // End of namespace Murphy3d
