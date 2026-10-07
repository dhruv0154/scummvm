#ifndef MURPHY3D_ANIMATED_CURSOR_H
#define MURPHY3D_ANIMATED_CURSOR_H

#include "common/scummsys.h"
#include "common/array.h"
#include "graphics/surface.h"

namespace Murphy3d {

enum class CursorType {
	kArrow = 0,
	kNote = 1,
	kCrosshair = 2,
	kDiskette = 3,
	kLook = 4,
	kMove = 5,
	kGrab = 6,
	kOnOff = 7,
	kTalk = 8,
	kHint = 9,
	kOpen = 10,
	kLoading = 11,
	kSpecial = 12
};

class AnimatedCursor {
public:
	AnimatedCursor();
	~AnimatedCursor();

	void setIcons(CursorType type, const Common::Array<Graphics::Surface *> &icons);
	void dispose();

	void render();
	void applyToCursorMan();

	bool hasIcons() {
		return _icons.size() > 0;
	}

	CursorType getType() {
		return _type;
	}

private:
	Common::Array<Graphics::Surface *> _icons;

	int _hotspotX, _hotspotY;
	uint32 _lastChange;
	int _currentFrame;
	int _direction;
	uint32 _interval;
	uint32 _forwardInterval;
	uint32 _reverseInterval;
	uint32 _loopDelay;
	uint32 _loopDelayCounter;

	CursorType _type;
};

} // End of namespace Murphy3d
#endif // MURPHY3D_ANIMATED_CURSOR_H
