#include "murphy3d/label.h"
#include "murphy3d/murphy3d.h"

namespace Murphy3d {

Label::Label(const Common::String &text, Font *font, float scale) : Control() {
	_text = new Text(font);
	_text->setText(text, {0, 0, 1000, 1000}, kAlignLeft, scale);
	_text->setColours(0xFFFFFFFF);
	_type = kLabel;
}

Label::~Label() {
	if (_text)
		delete _text;
}

void Label::render() {
	if (_text && g_engine && g_engine->_renderer) {
		_text->render(g_engine->_renderer, _x, _y);
	}
}

void Label::setText(const Common::String &text) {
	if (_text)
		_text->setText(text, {0, 0, 1000, 1000});
}

void Label::setColours(uint32 c1, uint32 c2, uint32 c3, uint32 c4) {
	if (_text)
		_text->setColours(c1, c2, c3, c4);
}

} // End of namespace Murphy3d
