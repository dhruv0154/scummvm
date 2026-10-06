#include "murphy3d/control.h"

namespace Murphy3d {

Control::Control() : _x(0.0f), _y(0.0f), _w(0.0f), _h(0.0f),
					 _visible(true), _enabled(true), _mouseOver(false),
					 _focus(false), _type(kUndefined) {
}

Control::~Control() {
}

Control *Control::hitTest(float x, float y) {
	if (_enabled && _visible && x >= _x && y >= _y && x < (_x + _w) && y < (_y + _h)) {
		return this;
	}
	return nullptr;
}

} // End of namespace Murphy3d
