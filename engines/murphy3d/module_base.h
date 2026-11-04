#ifndef MURPHY3D_MODULE_BASE_H
#define MURPHY3D_MODULE_BASE_H

#include "common/rect.h"
#include "common/scummsys.h"
#include "murphy3d/enums.h"

namespace Murphy3d {

class ModuleBase {
public:
	ModuleType _type;

	ModuleBase(ModuleType type);
	virtual ~ModuleBase();

	void init();
	virtual void resize(int width, int height) = 0;

	virtual void gotFocus();
	virtual void lostFocus();
	virtual void pause();
	virtual void resume();
	virtual void dispose();
	virtual void render();

	virtual void mouseMove(Common::Point pt);
	virtual void mouseDown(Common::Point pt, int btn);
	virtual void mouseUp(Common::Point pt, int btn);
	virtual void mouseWheel(int scroll);
	virtual void keyDown(uint32 key, uint32 lParam);
	virtual void keyUp(uint32 key, uint32 lParam);
	virtual void checkInput();

	virtual void cursor(float x, float y, bool relative);
	virtual void beginAction() {}
	virtual void endAction() {}
	virtual void back() {}
	virtual void cycle() {}
	virtual void moveForward(float v) {}
	virtual void moveBack(float v) {}
	virtual void moveLeft(float v) {}
	virtual void moveRight(float v) {}
	virtual void moveUp(float y) {}
	virtual void moveDown(float y) {}
	virtual void run(bool run) {}
	virtual void next() {}
	virtual void prev() {}
	virtual void inventory() {}
	virtual void travel() {}
	virtual void hints() {}

protected:
	virtual void initialize() = 0;

	void centerMouse();

	float _cursorPosX;
	float _cursorPosY;
	int _cursorMinX;
	int _cursorMaxX;
	int _cursorMinY;
	int _cursorMaxY;
	bool _initialized;

	Common::Rect _oldClippingArea;
	Common::Rect _currentClippingArea;
	bool _cursorIsClipped;
	bool _hasFocus;
};

} // End of namespace Murphy3d

#endif
