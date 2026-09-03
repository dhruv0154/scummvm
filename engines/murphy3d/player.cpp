#include "murphy3d/player.h"
#include "murphy3d/math_utils.h"
#include <math.h>

namespace Murphy3d {

Player::Player() {
	_x = _y = _z = 0.0f;
	_yaw = _pitch = 0.0f;

	_moveFwd = _moveBack = _moveLeft = _moveRight = false;
	_speed = 0.2f;

	_smoothMoveX = 0.0f;
	_smoothMoveZ = 0.0f;
}

void Player::spawn(float x, float y, float z, float yaw, float pitch) {
	_x = x;
	_y = y;
	_z = z;
	_yaw = yaw;
	_pitch = pitch;
}

void Player::setMovement(bool fwd, bool back, bool left, bool right) {
	_moveFwd = fwd;
	_moveBack = back;
	_moveLeft = left;
	_moveRight = right;
}

void Player::setSpeed(bool isRunning) {
	_speed = isRunning ? 0.5f : 0.2f;
}

void Player::addRotation(float deltaYaw, float deltaPitch) {
	_yaw += deltaYaw;
	_pitch += deltaPitch;

	if (_pitch > 1.5f)
		_pitch = 1.5f;
	if (_pitch < -1.5f)
		_pitch = -1.5f;
}

void Player::update() {
	float targetX = 0.0f;
	float targetZ = 0.0f;

	if (_moveLeft)
		targetX -= 1.0f;
	if (_moveRight)
		targetX += 1.0f;
	if (_moveFwd)
		targetZ += 1.0f;
	if (_moveBack)
		targetZ -= 1.0f;

	if (targetX != 0.0f || targetZ != 0.0f) {
		float len = sqrt(targetX * targetX + targetZ * targetZ);
		targetX = (targetX / len) * _speed;
		targetZ = (targetZ / len) * _speed;
	}

	_smoothMoveX = (_smoothMoveX * 0.85f) + (targetX * 0.15f);
	_smoothMoveZ = (_smoothMoveZ * 0.85f) + (targetZ * 0.15f);

	if (fabs(_smoothMoveX) < 0.01f && targetX == 0.0f)
		_smoothMoveX = 0.0f;
	if (fabs(_smoothMoveZ) < 0.01f && targetZ == 0.0f)
		_smoothMoveZ = 0.0f;

	_x += (_smoothMoveZ * sin(_yaw) - _smoothMoveX * cos(_yaw));
	_z -= (_smoothMoveZ * cos(_yaw) + _smoothMoveX * sin(_yaw));
}

Math::Matrix4 Player::getWorldMatrix() const {
	Math::Matrix4 tm = MathUtils::translation(_x, _y, _z);
	Math::Matrix4 rm2 = MathUtils::rotationY(_yaw);
	Math::Matrix4 rm1 = MathUtils::rotationX(_pitch);

	return tm * rm2 * rm1;
}

} // End of namespace Murphy3d
