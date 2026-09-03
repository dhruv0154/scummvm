#ifndef MURPHY3D_PLAYER_H
#define MURPHY3D_PLAYER_H

#include "common/scummsys.h"
#include "math/matrix4.h"

namespace Murphy3d {

class Player {
public:
	Player();
	~Player() {}

	void spawn(float x, float y, float z, float yaw, float pitch);

	void setMovement(bool fwd, bool back, bool left, bool right);
	void setSpeed(bool isRunning);
	void addRotation(float deltaYaw, float deltaPitch);

	void update();

	Math::Matrix4 getWorldMatrix() const;

	float getX() const { return _x; }
	float getY() const { return _y; }
	float getZ() const { return _z; }

private:
	float _x, _y, _z;
	float _yaw, _pitch;

	bool _moveFwd, _moveBack, _moveLeft, _moveRight;
	float _speed;

	float _smoothMoveX, _smoothMoveZ;
};

} // End of namespace Murphy3d

#endif
