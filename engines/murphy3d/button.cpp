#include "murphy3d/button.h"
#include "common/file.h"
#include "image/png.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/math_utils.h"

namespace Murphy3d {

Texture *Button::_texBackground = nullptr;
Texture *Button::_texMouseOver = nullptr;

Button::Button(const Common::String &text, float w, float h, void (*onClick)(void *), void *data, Font *font, float scale)
	: Control(), _clicked(onClick), _data(data), _vbo(0), _scale(scale) {

	_text = new Text(font);
	_text->setText(text, {0, 0, 1000, 1000}, kAlignLeft, scale);

	float textW = font->getPixelWidth(text) * scale;

	_x = 0.0f;
	_y = 0.0f;
	_w = (w > 0.0f) ? w : textW;
	_h = 40.0f * scale; // original engine locks button height to 40 scaled pixels

	Common::Array<TEXTURED_VERTEX> vertices;

	// corners remain 16px, middle stretches to _w
	float x1 = 0.0f;
	float x2 = 16.0f * scale;
	float x3 = 16.0f * scale + _w;
	float x4 = x3 + 16.0f * scale;

	float y1 = 0.0f;
	float y2 = -16.0f * scale;
	float y3 = -24.0f * scale;
	float y4 = -40.0f * scale;

	// texture UV mapping
	float u1 = 0.0f, u2 = 0.25f, u3 = 0.75f, u4 = 1.0f;
	float v1 = 1.0f, v2 = 0.75f, v3 = 0.25f, v4 = 0.0f;

	setQuadVertex(vertices, x1, x2, y1, y2, u1, u2, v1, v2);
	setQuadVertex(vertices, x2, x3, y1, y2, u2, u3, v1, v2);
	setQuadVertex(vertices, x3, x4, y1, y2, u3, u4, v1, v2);

	setQuadVertex(vertices, x1, x2, y2, y3, u1, u2, v2, v3);
	setQuadVertex(vertices, x2, x3, y2, y3, u2, u3, v2, v3);
	setQuadVertex(vertices, x3, x4, y2, y3, u3, u4, v2, v3);

	setQuadVertex(vertices, x1, x2, y3, y4, u1, u2, v3, v4);
	setQuadVertex(vertices, x2, x3, y3, y4, u2, u3, v3, v4);
	setQuadVertex(vertices, x3, x4, y3, y4, u3, u4, v3, v4);

	if (g_engine && g_engine->_renderer) {
		_vbo = g_engine->_renderer->createVertexBuffer(vertices.data(), vertices.size() * sizeof(TEXTURED_VERTEX), false);
	}

	// calculate text centering offset
	_textX = 16.0f * scale + (_w - textW) / 2.0f;
	_w += 32.0f * scale;

	_type = kButton;
}

Button::~Button() {
	if (_text)
		delete _text;
	if (_vbo && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(_vbo);
	}
	disposeTextures();
}

void Button::initTextures(const Common::String &bgPath, const Common::String &hoverPath) {
	Common::File file;
	Image::PNGDecoder decoder;

	if (file.open(Common::Path(bgPath)) && decoder.loadStream(file)) {
		const Graphics::Surface *surf = decoder.getSurface();
		_texBackground = new Texture(surf->w, surf->h, (const byte *)surf->getPixels());
	}
	file.close();
	decoder.destroy();

	if (file.open(Common::Path(hoverPath)) && decoder.loadStream(file)) {
		const Graphics::Surface *surf = decoder.getSurface();
		_texMouseOver = new Texture(surf->w, surf->h, (const byte *)surf->getPixels());
	}
}

void Button::disposeTextures() {
	if (_texBackground) {
		delete _texBackground;
		_texBackground = nullptr;
	}
	if (_texMouseOver) {
		delete _texMouseOver;
		_texMouseOver = nullptr;
	}
}

void Button::setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2) {
	vertices.push_back({x1, y1, -0.5f, u1, v1});
	vertices.push_back({x2, y1, -0.5f, u2, v1});
	vertices.push_back({x2, y2, -0.5f, u2, v2});

	vertices.push_back({x1, y1, -0.5f, u1, v1});
	vertices.push_back({x2, y2, -0.5f, u2, v2});
	vertices.push_back({x1, y2, -0.5f, u1, v2});
}

void Button::render() {
	if (!_vbo || !g_engine || !g_engine->_renderer)
		return;

	Math::Matrix4 worldMat = MathUtils::translation(_x, -_y, 0.0f);

	Texture *activeTex = _mouseOver ? _texMouseOver : _texBackground;
	if (activeTex) {
		g_engine->_renderer->drawUITextured(_vbo, 54, activeTex->getId(), worldMat);
	}

	if (_text) {
		// change color based on hover state
		if (_mouseOver && _enabled) {
			_text->setColours(0xFFFFFFFF); 
		} else {
			_text->setColours(0x80FFFFFF);
		}
		_text->render(g_engine->_renderer, _textX + _x, _y + 12.0f * _scale);
	}
}

void Button::mouseEnter() {
	if (_enabled)
		setMouseOver(true);
}
void Button::mouseLeave() {
	if (_enabled)
		setMouseOver(false);
}
void Button::mouseButtonUp() {
	if (_enabled && _mouseOver)
		click();
}
void Button::click() {
	if (_clicked)
		_clicked(_data);
}
void Button::setMouseOver(bool mouseOver) { _mouseOver = mouseOver; }

} // End of namespace Murphy3d
