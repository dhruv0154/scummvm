#include "murphy3d/module_base.h"
#include "common/system.h"

namespace Murphy3d {

ModuleBase::ModuleBase(ModuleType type) {
	_type = type;
	_initialized = false;
	_cursorPosX = 0.0f;
	_cursorPosY = 0.0f;
	_cursorMinX = 0;
	_cursorMaxX = g_system ? g_system->getWidth() : 640;
	_cursorMinY = 0;
	_cursorMaxY = g_system ? g_system->getHeight() : 480;
	_cursorIsClipped = false;
	_hasFocus = false;
}

ModuleBase::~ModuleBase() {
}

void ModuleBase::init() {
	if (!_initialized) {
		_cursorMaxX = g_system->getWidth();
		_cursorMaxY = g_system->getHeight();
		_cursorPosX = _cursorMaxX / 2.0f;
		_cursorPosY = _cursorMaxY / 2.0f;
		initialize();
		_initialized = true;
	}
}

void ModuleBase::gotFocus() {
	_hasFocus = true;
}

void ModuleBase::lostFocus() {
	_hasFocus = false;
}

void ModuleBase::pause() {
}

void ModuleBase::resume() {
}

void ModuleBase::dispose() {
}

void ModuleBase::render() {
}

void ModuleBase::mouseMove(Common::Point pt) {
}

void ModuleBase::mouseDown(Common::Point pt, int btn) {
}

void ModuleBase::mouseUp(Common::Point pt, int btn) {
}

void ModuleBase::mouseWheel(int scroll) {
}

void ModuleBase::keyDown(uint32 key, uint32 lParam) {
}

void ModuleBase::keyUp(uint32 key, uint32 lParam) {
}

void ModuleBase::checkInput() {
}

void ModuleBase::cursor(float x, float y, bool relative) {
	if (relative) {
		_cursorPosX += x;
		_cursorPosY += y;
	} else {
		_cursorPosX = x;
		_cursorPosY = y;
	}

	if (_cursorPosX < _cursorMinX) {
		_cursorPosX = (float)_cursorMinX;
	} else if (_cursorPosX > _cursorMaxX) {
		_cursorPosX = (float)_cursorMaxX;
	}

	if (_cursorPosY < _cursorMinY) {
		_cursorPosY = (float)_cursorMinY;
	} else if (_cursorPosY > _cursorMaxY) {
		_cursorPosY = (float)_cursorMaxY;
	}
}

} // End of namespace Murphy3d
