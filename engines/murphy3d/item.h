#ifndef MURPHY3D_ITEM_H
#define MURPHY3D_ITEM_H

#include "common/scummsys.h"
#include "common/stream.h"
#include "graphics/screen.h"

namespace Murphy3d {

class Item {
public:
	Item();
	~Item();

	bool load(Common::SeekableReadStream *stream);
	void draw(Graphics::Screen *screen, int x, int y);

	uint16 getWidth() const { return _width; }
	uint16 getHeight() const { return _height; }

private:
	uint16 _width;
	uint16 _height;
	byte *_data;
	uint32 _dataSize;
};

} // End of namespace Murphy3d

#endif
