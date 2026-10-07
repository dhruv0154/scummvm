#include "murphy3d/animated_cursor.h"
#include "common/system.h"
#include "graphics/cursorman.h"

namespace Murphy3d {

AnimatedCursor::AnimatedCursor() {
	_hotspotX = 0;
	_hotspotY = 0;
	_lastChange = 0;
	_currentFrame = 0;
	_direction = 1;
	_interval = 50;
	_forwardInterval = 50;
	_reverseInterval = 50;
	_loopDelay = 0;
	_loopDelayCounter = 0;
	_type = CursorType::kArrow;
}

AnimatedCursor::~AnimatedCursor() {
	dispose();
}

void AnimatedCursor::dispose() {
	for (uint i = 0; i < _icons.size(); ++i) {
		if (_icons[i]) {
			_icons[i]->free();
			delete _icons[i];
		}
	}
	_icons.clear();
}

void AnimatedCursor::setIcons(CursorType type, const Common::Array<Graphics::Surface *> &icons) {
	dispose();

	_type = type;
	if (icons.empty())
		return;

	for (uint i = 0; i < icons.size(); ++i) {
		Graphics::Surface *surf = new Graphics::Surface();
		surf->copyFrom(*icons[i]);
		_icons.push_back(surf);
	}

	_direction = 1;
	_currentFrame = 0;
	_loopDelay = 0;
	_loopDelayCounter = 0;

	if (type == CursorType::kArrow) {
		_hotspotX = 0;
		_hotspotY = 0;
	} else if (type == CursorType::kCrosshair) {
		_hotspotX = 7;
		_hotspotY = 7;
	} else if (type == CursorType::kLook) {
		_loopDelay = 10;
		_hotspotX = 14;
		_hotspotY = 4;
	} else if (type == CursorType::kMove) {
		_hotspotX = 11;
		_hotspotY = 29;
		_loopDelay = 10;
	} else if (type == CursorType::kGrab) {
		_hotspotX = 13;
		_hotspotY = 25;
		_loopDelay = 10;
	} else if (type == CursorType::kOnOff) {
		_hotspotX = 10;
		_hotspotY = 4;
		_loopDelay = 10;
	} else if (type == CursorType::kTalk) {
		_hotspotX = 10;
		_hotspotY = 4;
		_loopDelay = 10;
	} else if (type == CursorType::kOpen) {
		_reverseInterval = 10;
		_loopDelay = 10;
		_hotspotX = 12;
		_hotspotY = 4;
	}
}

void AnimatedCursor::applyToCursorMan() {
	if (_icons.empty())
		return;

	if (_currentFrame < 0 || (uint)_currentFrame >= _icons.size())
		_currentFrame = 0;

	Graphics::Surface *surf = _icons[_currentFrame];

	// pass 0 for keycolor because the surface's alpha channel handles transparency
	CursorMan.replaceCursor(surf->getPixels(), surf->w, surf->h, _hotspotX, _hotspotY, 0, &surf->format);
}

void AnimatedCursor::render() {
	if (_icons.size() <= 1)
		return; // static cursor

	uint32 now = g_system->getMillis();
	if ((now - _lastChange) > _interval) {
		_lastChange = now;
		_currentFrame += _direction;

		int frameCount = (int)_icons.size();

		if (_currentFrame >= frameCount) {

			if (_type == CursorType::kLoading || _type == CursorType::kGrab) {
				if (_loopDelayCounter < _loopDelay) {
					_loopDelayCounter++;
					_currentFrame -= _direction; // keep the frame to the last one to stay static
				} else {
					_currentFrame = 0; // restart the animation
					_loopDelayCounter = 0;
				}
			} else {
				// play the animation in reverse
				_direction = -1;
				_currentFrame += _direction;
				_interval = _reverseInterval;
			} 
		} else if (_currentFrame < 0) {
			if (_loopDelayCounter < _loopDelay) {
				_loopDelayCounter++;
				_currentFrame -= _direction; // keep the frame to the first one to stay static
			} else {
				_direction = 1;
				_currentFrame += _direction;
				_interval = _forwardInterval;
				_loopDelayCounter = 0;
			}
		}
		applyToCursorMan();
	}
}

} // End of namespace Murphy3d
