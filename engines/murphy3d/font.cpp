#include "murphy3d/font.h"
#include "common/endian.h"

namespace Murphy3d {

Font::Font() : _fontData(nullptr), _fontDataSize(0), _bitsPerPixel(0), _fontHeight(0) {
	memset(_fontMap, 0, sizeof(_fontMap));
}

Font::~Font() {
	delete[] _fontData;
}

bool Font::load(Common::SeekableReadStream *stream) {
	delete[] _fontData;
	memset(_fontMap, 0, sizeof(_fontMap));

	_fontDataSize = stream->size();
	_fontData = new byte[_fontDataSize];
	stream->read(_fontData, _fontDataSize);

	int charCount = _fontData[0];
	_bitsPerPixel = _fontData[1];
	_fontHeight = _fontData[2];

	for (int i = 0; i < charCount; i++) {
		uint32 offset = READ_LE_INT32(_fontData + 3 + (i * 4));

		if (32 + i < 256) {
			_fontMap[32 + i] = _fontData + offset;
		}
	}
	return true;
}

int Font::readBits(const byte* data, int bitsPerPixel, int& bitOffset) const {
	int bytePos = bitOffset / 8;
	int bitRemainder = bitOffset % 8;

	int bitPos = 8 - bitsPerPixel - bitRemainder;
	int mask = (1 << bitsPerPixel) - 1;

	int pixel = (data[bytePos] >> bitPos) & mask;
	bitOffset += bitsPerPixel;
	return pixel;
}

void Font::draw(Graphics::Screen* screen, int x, int y, const Common::String& text, const byte* colorMap) {
	if (!_fontData)
		return;

	int currentX = x;
	int currentY = y;

	for (uint i = 0; i < text.size(); i++) {
		char character = text[i];

		if (character == '\r' || character == '\n') {
			currentX = x;
			currentY += _fontHeight;
			continue;
		}

		byte *pCharData = _fontMap[(byte)character];
		if (pCharData != nullptr) {
			int charWidth = pCharData[0];
			int bytesPerRow = (charWidth * _bitsPerPixel + 7) / 8;
			int byteOffset = 1;

			for (int fy = 0; fy < _fontHeight; fy++) {
				int bitOffset = byteOffset * 8;

				for (int fx = 0; fx < charWidth; fx++) {
					int pixel = readBits(pCharData, _bitsPerPixel, bitOffset);

					if (pixel != 0 && colorMap[pixel] != 0) {
						byte *screenPixel = (byte *)screen->getBasePtr(currentX + fx, currentY + fy);
						*screenPixel = colorMap[pixel];
					}
				}
				byteOffset += bytesPerRow;
			}
			currentX += charWidth;
		}
	}
}

int Font::measureString(const Common::String& text, int horizontalAdjustment) const {
	int width = 0;

	for (int i = 0; i < text.size(); i++) {
		byte *pCharData = _fontMap[(byte)text[i]];
		if (pCharData != nullptr) {
			width += pCharData[0] + horizontalAdjustment;
		}
	}

	return width;
}

} // End of namepsace Murphy3d
