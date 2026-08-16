#ifndef MURPHY3D_FONT_H
#define MURPHY3D_FONT_H

#include "common/scummsys.h"
#include "common/stream.h"
#include "common/str.h"
#include "graphics/screen.h"

namespace Murphy3d {
class Font {
public:
	Font();
	~Font();

	bool load(Common::SeekableReadStream *stream);

	void draw(Graphics::Screen *screen, int x, int y, const Common::String &text, const byte *colorMap);

	int measureString(const Common::String &text, int horizontalAdjustment = 0) const;
	int getHeight() { return _fontHeight; }

	int _bitsPerPixel;
private:
	byte *_fontData;
	uint32 _fontDataSize;

	int _fontHeight;

	byte *_fontMap[256];

	int readBits(const byte *data, int bitsPerPixel, int &bitOffset) const;
};

} // End of namespace Murphy3d

#endif
