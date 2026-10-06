#ifndef MURPHY3D_TEXT_H
#define MURPHY3D_TEXT_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/font.h"
#include "murphy3d/renderer.h"
#include "murphy3d/shader_structs.h"

namespace Murphy3d {

enum TextAlignment {
	kAlignLeft = 0,
	kAlignCenter = 1,
	kAlignRight = 2,
	kAlignJustify = 3
};

struct Rect {
	float left, top, right, bottom;
};

class Text {
public:
	Text(Font *font);
	~Text();

	void setColours(uint32 c1, uint32 c2 = 0, uint32 c3 = 0, uint32 c4 = 0);

	// generates the geometry for the text
	void setText(const Common::String &text, const Rect &rect, TextAlignment alignment = kAlignLeft, float scale = 1.0f);

	void render(Renderer *renderer, float x, float y, float z = -1.0f);

	int getLines() const { return _lines; }
	float getWidth() const { return _width; }

private:
	Font *_font;
	Common::Array<MULTICOLOURED_FONT_VERTEX> _vertices;
	GLuint _vbo;

	int _lines;
	float _width;

	uint32 _colour1, _colour2, _colour3, _colour4;

	struct Word {
		Common::String text;
		float pixels;
		uint32 color;
	};

	void extractRGB(uint32 color, float &r, float &g, float &b);
};

} // End of namespace Murphy3d
#endif
