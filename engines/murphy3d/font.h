#ifndef MURPHY3D_FONT_H
#define MURPHY3D_FONT_H

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/str.h"
#include "murphy3d/texture.h"

namespace Murphy3d {
class Font {
public:
	Font();
	~Font();

	bool load(const Common::String &fileName);

	float getPixelWidth(char ch) const;
	float getPixelWidth(const Common::String &text) const;

	Texture *getTexture() const { return _texture; }
	float getHeight() const { return _height; }
	float getY1() const { return _y1; }
	float getY2() const { return _y2; }

	float getWidthArray(int index) const { return _widths[index]; }

private:
	Texture *_texture;
	float _widths[224];
	float _height;
	float _y1;
	float _y2;

};

} // End of namespace Murphy3d

#endif
