#include "murphy3d/item.h"
#include "common/endian.h"

namespace Murphy3d {
Item::Item() : _width(0), _height(0), _data(nullptr), _dataSize(0) {

}

Item::~Item() {
	delete[] _data;
}

bool Item::load(Common::SeekableReadStream* stream) {
	delete[] _data; // we clear old data if we are reusing this object

	_dataSize = stream->size();
	_data = new byte[_dataSize];
	stream->read(_data, _dataSize);

	uint16 sig = READ_LE_UINT16(_data);
	// every sprite begins with this header
	if (sig != 0x0100)
		return false;

	_width = READ_LE_UINT16(_data + 2);
	_height = READ_LE_UINT16(_data + 4);

	return true;
}

void Item::draw(Graphics::Screen* screen, int x, int y) {
	if (!_data)
		return;
	// the actual image data begins after the header, width and height each of them take 2 bytes
	// and then we have padding of 10 bytes
	uint32 dataOffset = 16;

	for (int row = 0; row < _height; row++) {
		// c1 tells us the number of transparent pixels to skip on the left side
		uint16 c1 = READ_LE_UINT16(_data + dataOffset);
		// c2 tells us the number of solid pixels to draw
		uint16 c2 = READ_LE_UINT16(_data + dataOffset + 2);
		// since c1 and c2 both are 2 bytes we advance our dataOffset by 4
		dataOffset += 4;

		for (int col = 0; col < _width && col < c2; col++) {
			byte colorIndex = _data[dataOffset + col];
			// if color is not transparent we write our color index
			// into the memory address of the pixel we are drawing to
			if (colorIndex != 0) {
				byte *pixel = (byte *)screen->getBasePtr(x + c1 + col, y + row);
				*pixel = colorIndex;
			}
		}
		dataOffset += c2;
	}
}

} // End of namespace Murphy3d
