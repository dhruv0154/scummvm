#include "murphy3d/item.h"
#include "common/endian.h"
#include "murphy3d/archive.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/math_utils.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/shader_structs.h"

namespace Murphy3d {

#define ITEMS_TEXTURE_PIXELS 128
#define ITEMS_TEXTURE_WIDTH (ITEMS_TEXTURE_PIXELS * 16)
#define ITEMS_TEXTURE_HEIGHT (ITEMS_TEXTURE_PIXELS * 20)

Texture *Item::_atlasTexture = nullptr;
Common::HashMap<int, Item::InventoryEntry *> Item::_items;

Item::InventoryEntry::~InventoryEntry() {
	if (vbo != 0 && g_engine && g_engine->_renderer) {
		g_engine->_renderer->deleteBuffer(vbo);
		vbo = 0;
	}
}

Item::Item() : _width(0), _height(0), _data(nullptr), _dataSize(0) {
}

Item::~Item() {
	delete[] _data;
}

bool Item::init() {
	dispose();

	if (!g_engine || !g_engine->_renderer)
		return false;

	Archive invArch;
	if (!invArch.open("INV.AP"))
		return false;

	Common::SeekableReadStream *palStream = invArch.getStream(0);
	if (!palStream || palStream->size() < 768) {
		delete palStream;
		return false;
	}

	byte palette[768];
	palStream->read(palette, 768);
	delete palStream;

	for (int i = 0; i < 768; i++) {
		palette[i] = static_cast<uint8>((palette[i] & 0x3f) * 4.04762f);
	}

	const int w = ITEMS_TEXTURE_WIDTH;
	const int h = ITEMS_TEXTURE_HEIGHT;
	byte *rgbaAtlas = new byte[w * h * 4];
	memset(rgbaAtlas, 0, w * h * 4);

	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 16; j++) {
			int id = i * 16 + j;
			uint entryIdx = 2 + id;

			Common::SeekableReadStream *s = invArch.getStream(entryIdx);
			if (!s)
				continue;

			uint32 len = s->size();
			if (len > 16) {
				byte *bd = new byte[len];
				s->read(bd, len);

				if (READ_LE_UINT16(bd) == 0x0100) {
					int imgW = READ_LE_UINT16(bd + 2);
					int imgH = READ_LE_UINT16(bd + 4);
					uint32 dataOffset = 16;

					for (int y = 0; y < imgH && y < ITEMS_TEXTURE_PIXELS && dataOffset + 4 <= len; y++) {
						int c1 = READ_LE_UINT16(bd + dataOffset);
						int c2 = READ_LE_UINT16(bd + dataOffset + 2);
						dataOffset += 4;

						for (int x = 0; x < imgW && x < c2 && dataOffset + x < len; x++) {
							byte colIx = bd[dataOffset + x];
							if (colIx != 0 && (x + c1) < ITEMS_TEXTURE_PIXELS) {
								int dst = ((i * ITEMS_TEXTURE_PIXELS + y) * w + (j * ITEMS_TEXTURE_PIXELS + x + c1)) * 4;
								rgbaAtlas[dst + 0] = palette[colIx * 3 + 0];
								rgbaAtlas[dst + 1] = palette[colIx * 3 + 1];
								rgbaAtlas[dst + 2] = palette[colIx * 3 + 2];
								rgbaAtlas[dst + 3] = 255;
							}
						}
						dataOffset += c2;
					}

					InventoryEntry *inv = new InventoryEntry();
					inv->id = id;

					float tx1 = (j * ITEMS_TEXTURE_PIXELS) / (float)w;
					float tx2 = ((j + 1) * ITEMS_TEXTURE_PIXELS) / (float)w;
					float ty1 = (i * ITEMS_TEXTURE_PIXELS) / (float)h;
					float ty2 = ((i + 1) * ITEMS_TEXTURE_PIXELS) / (float)h;

					float sz = (float)ITEMS_TEXTURE_PIXELS;
					TEXTURED_VERTEX quad[6] = {
						{0.0f, 0.0f, -0.5f, tx1, ty1},
						{sz, 0.0f, -0.5f, tx2, ty1},
						{sz, -sz, -0.5f, tx2, ty2},
						{0.0f, 0.0f, -0.5f, tx1, ty1},
						{sz, -sz, -0.5f, tx2, ty2},
						{0.0f, -sz, -0.5f, tx1, ty2}};

					inv->vbo = g_engine->_renderer->createVertexBuffer(quad, sizeof(quad), false);

					Common::String itemName = GameController::getItemName(id);
					if (!itemName.empty()) {
						inv->text.setColours(0xff000000, 0xffffffff, 0xffffffff, 0xff000000);
						inv->text.setText(itemName);
					}

					_items[id] = inv;
				}
				delete[] bd;
			}
			delete s;
		}
	}

	_atlasTexture = new Texture(w, h, rgbaAtlas);
	delete[] rgbaAtlas;
	return true;
}

void Item::dispose() {
	for (auto &it : _items) {
		delete it._value;
	}
	_items.clear();

	if (_atlasTexture) {
		delete _atlasTexture;
		_atlasTexture = nullptr;
	}
}

void Item::renderItemImage(int id, float x, float y, bool highlight) {
	if (!_atlasTexture || !g_engine || !g_engine->_renderer)
		return;

	if (_items.contains(id) && _items[id] != nullptr && _items[id]->vbo != 0) {
		Math::Matrix4 wm = MathUtils::translation(x, -y, -1.0f);
		g_engine->_renderer->drawUITextured(_items[id]->vbo, 6, _atlasTexture->getId(), wm);
	}
}

void Item::renderItemName(int id, float x, float y, bool highlight) {
	if (!g_engine || !g_engine->_renderer)
		return;

	if (_items.contains(id) && _items[id] != nullptr) {
		_items[id]->text.render(g_engine->_renderer, x, y);
	}
}

bool Item::load(Common::SeekableReadStream *stream) {
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

void Item::draw(Graphics::Screen *screen, int x, int y) {
	if (!_data || !screen)
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
