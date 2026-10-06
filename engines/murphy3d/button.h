#ifndef MURPHY3D_BUTTON_H
#define MURPHY3D_BUTTON_H

#include "murphy3d/control.h"
#include "murphy3d/text.h"
#include "murphy3d/texture.h"

namespace Murphy3d {

class Button : public Control {
public:
	Button(const Common::String &text, float w, float h, void (*onClick)(void *) = nullptr, void *data = nullptr, Font *font = nullptr, float scale = 1.0f);
	virtual ~Button() override;

	static void initTextures(const Common::String &bgPath, const Common::String &hoverPath);
	static void disposeTextures();

	void render() override;

	void mouseEnter() override;
	void mouseLeave() override;
	void mouseButtonUp() override;
	void click() override;

	void setMouseOver(bool mouseOver) override;

protected:
	void (*_clicked)(void *data);
	void *_data;

	static Texture *_texBackground;
	static Texture *_texMouseOver;

	Text *_text;
	float _textX, _textY;
	GLuint _vbo;
	float _scale;

	void setQuadVertex(Common::Array<TEXTURED_VERTEX> &vertices, float x1, float x2, float y1, float y2, float u1, float u2, float v1, float v2);
};

} // End of namespace Murphy3d
#endif
