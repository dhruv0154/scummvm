#ifndef MURPHY3D_ELEVATION_H
#define MURPHY3D_ELEVATION_H

#include "common/scummsys.h"
#include "math/vector3d.h"

namespace Murphy3d {

class Elevation {
public:
	Elevation(const byte *data);

	bool isPointInElevation(const Math::Vector3d &pos) const;
	float getElevationFromXZPosition(const Math::Vector3d &pos) const;
	float getClosestElevation(float y) const;

	static float elevationModifier;
	static float elevationCheckModifier;

protected:
	float _x1;
	float _y1;
	float _z1;
	float _x2;
	float _y2;
	float _z2;
	float _stepHeight;
	float _stepWidth;
	int _numberOfSteps;
	float _horizontalLength;
	float _xDirection;
	float _zDirection;

	float _x11;
	float _x12;
	float _x21;
	float _x22;
	float _z11;
	float _z12;
	float _z21;
	float _z22;

	float _xMin;
	float _xMax;
	float _yMin;
	float _yMax;
	float _zMin;
	float _zMax;
};

} // End of namespace Murphy3d

#endif
