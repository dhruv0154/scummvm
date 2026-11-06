#ifndef MURPHY3D_ITEM_H
#define MURPHY3D_ITEM_H

#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/stream.h"
#include "graphics/opengl/system_headers.h"
#include "graphics/screen.h"
#include "murphy3d/text.h"
#include "murphy3d/texture.h"

namespace Murphy3d {

class Item {
public:
	Item();
	~Item();

	static bool init();
	static void dispose();

	static void renderItemImage(int id, float x, float y, bool highlight = false);
	static void renderItemName(int id, float x, float y, bool highlight = false);

	bool load(Common::SeekableReadStream *stream);
	void draw(Graphics::Screen *screen, int x, int y);

	uint16 getWidth() const { return _width; }
	uint16 getHeight() const { return _height; }

private:
	struct InventoryEntry {
		int id;
		GLuint vbo;
		Text text;

		InventoryEntry() : id(0), vbo(0) {}
		~InventoryEntry();
	};

	static Texture *_atlasTexture;
	static Common::HashMap<int, InventoryEntry *> _items;

	uint16 _width;
	uint16 _height;
	byte *_data;
	uint32 _dataSize;
};

} // End of namespace Murphy3d

#endif
