#include "murphy3d/font.h"
#include "common/debug.h"
#include "common/file.h"
#include "graphics/surface.h"
#include "image/png.h"

namespace Murphy3d {

Font::Font() : _texture(nullptr), _height(0.0f), _y1(0.0f), _y2(0.0f) {
	memset(_widths, 0, sizeof(_widths));
}

Font::~Font() {
	if (_texture)
		delete _texture;
}

bool Font::load(const Common::String &fileName) {
	Common::File file;
	if (!file.open(Common::Path(fileName))) {
		warning("Murphy3d::Font: Could not open font file '%s'", fileName.c_str());
		return false;
	}

	Image::PNGDecoder decoder;
	if (!decoder.loadStream(file)) {
		warning("Murphy3d::Font: Failed to decode PNG font atlas");
		return false;
	}

	const Graphics::Surface *surface = decoder.getSurface();
	Graphics::PixelFormat rgbaFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);
	Graphics::Surface *rgbaSurf = surface->convertTo(rgbaFormat);

	_texture = new Texture(rgbaSurf->w, rgbaSurf->h, (const byte *)rgbaSurf->getPixels());

	int charWidth = 16;
	int charHeight = 16;
	int offset = 0;
	int tallest = 0;
	const byte *pData = (const byte *)rgbaSurf->getPixels();

	for (int c = 0; c < 224; c++) {
		int fp = charHeight;
		int lp = 0;
		int cw = -1;

		for (int y = 0; y < charHeight; y++) {
			for (int x = 0; x < charWidth; x++) {
				for (int l = 0; l < 4; l++) {
					uint32 pixelIndex = ((y + l * charHeight) * rgbaSurf->w + x) * 4;
					int a = pData[offset + pixelIndex + 3];

					if (a > 0) {
						if (x > cw)
							cw = x;
						if (y < fp)
							fp = y;
						if (y > lp)
							lp = y;
					}
				}
			}
		}

		if (cw < 0)
			cw = 0;
		_widths[c] = (float)(cw + 1);
		if ((lp - fp) > tallest)
			tallest = lp - fp;

		offset += charWidth * 4;
	}

	_widths[0] = 5.0f;
	_height = tallest + 1.0f;
	_y1 = 0.75f;
	_y2 = (3.0f * charHeight + _height) / (float)rgbaSurf->h;

	rgbaSurf->free();
	delete rgbaSurf;
	return true;
}

float Font::getPixelWidth(char ch) const {
	// the alphabet array starts at the space character (ASCII 0x20 or 32)
	// we subtract 0x20 from the ASCII value of the character to get its index in our array
	if (ch >= 0x20 && (unsigned char)ch <= 0x7f) {
		return _widths[(unsigned char)ch - 0x20];
	}
	return 0.0f;
}

float Font::getPixelWidth(const Common::String &text) const {
	float pixels = 0.0f;
	// loop through every character in the string and add its width to the total
	for (uint i = 0; i < text.size(); i++) {
		pixels += getPixelWidth(text[i]);
	}
	return pixels;
}

} // End of namespace Murphy3d
