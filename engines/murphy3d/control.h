#ifndef MURPHY3D_CONTROL_H
#define MURPHY3D_CONTROL_H

namespace Murphy3d {

class Control {
public:
	Control();
	virtual ~Control();

	virtual void render() = 0;

	virtual void mouseEnter() {}
	virtual void mouseMove() {}
	virtual void mouseLeave() {}
	virtual void mouseButtonDown() {}
	virtual void mouseButtonUp() {}
	virtual void click() {}

	virtual Control *hitTest(float x, float y);

	float getX() const { return _x; }
	float getY() const { return _y; }
	void setPosition(float x, float y) {
		_x = x;
		_y = y;
	}

	float getWidth() const { return _w; }
	float getHeight() const { return _h; }
	void setSize(float w, float h) {
		_w = w;
		_h = h;
	}

	bool getVisible() const { return _visible; }
	void setVisible(bool visible) { _visible = visible; }

	bool getEnabled() const { return _enabled; }
	virtual void setEnabled(bool enabled) { _enabled = enabled; }

	bool getMouseOver() const { return _mouseOver; }
	virtual void setMouseOver(bool mouseOver) { _mouseOver = mouseOver; }

	enum ControlType { kUndefined,
					   kButton,
					   kImageButton,
					   kLabel,
					   kCheckBox,
					   kSlider };
	ControlType getType() const { return _type; }

protected:
	float _x, _y, _w, _h;
	bool _visible, _enabled, _mouseOver, _focus;
	ControlType _type;
};

} // End of namespace Murphy3d
#endif
