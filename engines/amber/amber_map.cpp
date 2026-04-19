/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "amber_map.h"
#include "archive.h"
#include "common/debug.h"
#include "common/stream.h"
#include "graphics/paletteman.h"
#include "decoders.h"
#include "amber.h"

namespace Amber {

AmberTileset::AmberTileset() {
}

AmberTileset::~AmberTileset() {
	_tileInfos.clear();

	for (uint i = 0; i < _graphics.size(); i++) {
		if (_graphics[i]) {
			_graphics[i]->free();
			delete _graphics[i];
		}
	}
	_graphics.clear();
}

bool AmberTileset::load(uint16 tilesetId, AmberEngine *engine) {
	AmberArchive archive;
	Common::SeekableReadStream *stream = nullptr;
	Common::Path tilesetFileId(Common::String::format("%d", tilesetId));

	// all tileset is stored in a single file called "Icon_data.amb"
	if (archive.open(Common::Path("Icon_data.amb"))) {
		if (archive.hasFile(tilesetFileId)) {
			stream = archive.createReadStreamForMember(tilesetFileId);
		}
	}

	if (!stream) {
		warning("AmberTileset: Failed to locate or decrypt tileset %d inside Icon_data.amb", tilesetId);
		archive.close();
		return false;
	}

	// the very first 2 bytes tell us exactly
	// how many tiles are defined in this specific tileset
	uint16 numEntries = stream->readUint16BE();
	_tileInfos.reserve(numEntries);

	// parse the tile rules (8 Bytes per tile)
	for (uint16 i = 0; i < numEntries; i++) {
		AmberTileInfo info;

		info.flags = stream->readUint32BE();
		info.graphicIndex = stream->readUint16BE();
		info.numFrames = stream->readByte();
		info.minimapColor = stream->readByte();

		_tileInfos.push_back(info);
	}

	// find the maximum graphic index needed
	// graphic indices are 1 based, we need to find the highest graphic index used by this tileset
	// so we know how big to make our _graphics array, we also have to account for animations
	// if a tile uses graphic 10 and has 4 frames, it will also use graphics 11, 12, and 13
	uint16 maxGraphicIndex = 0;
	for (uint16 i = 0; i < numEntries; i++) {
		if (_tileInfos[i].graphicIndex > 0) {
			// numFrames is at least 1 for a static tile
			uint16 frames = _tileInfos[i].numFrames;
			uint16 highestNeeded = _tileInfos[i].graphicIndex + frames - 1;

			if (highestNeeded > maxGraphicIndex)
				maxGraphicIndex = highestNeeded;
		}
	}

	// we size it to maxGraphicIndex + 1 because the indices are 1 based (index 0 is unused)
	_graphics.resize(maxGraphicIndex + 1);
	for (uint16 i = 0; i <= maxGraphicIndex; i++)
		_graphics[i] = nullptr;

	const char *gfxArchives[] = {"1Icon_gfx.amb", "2Icon_gfx.amb", "3Icon_gfx.amb"};
	int loadedGraphicsCount = 0;

	Common::Path gfxFileId(Common::String::format("%d", tilesetId));
	Common::SeekableReadStream *gfxStream = nullptr;

	for (int a = 0; a < 3; a++) {
		AmberArchive gfxArchive;
		if (gfxArchive.open(Common::Path(gfxArchives[a]))) {
			if (gfxArchive.hasFile(gfxFileId)) {
				gfxStream = gfxArchive.createReadStreamForMember(gfxFileId);
				gfxArchive.close();
				break;
			}
			gfxArchive.close();
		}
	}

	if (gfxStream) {
		// graphic indices are 1 based, so we start i at 1
		// each call to decodePlanarGraphic consumes exactly 160 bytes and moves the stream forward
		for (uint16 i = 1; i <= maxGraphicIndex; i++) {
			if (!gfxStream->eos()) {
				_graphics[i] = engine->decodePlanarGraphic(gfxStream, 16, 16, 5, 0);
				loadedGraphicsCount++;
			} else {
				warning("AmberTileset: Reached end of graphic stream at index %d", i);
				break;
			}
		}
		delete gfxStream;
	} else {
		warning("AmberTileset: Failed to find graphics for tileset %d", tilesetId);
	}

	delete stream;
	archive.close();

	return true;
}

bool AmberTileset::loadAmberstar(uint16 tilesetId, AmberEngine *engine) {
	AmberArchive archive;
	// ICON_DAT.AMB is an AMPC archive containing tilesets 1 (world) and 2 (indoors)
	if (!archive.open(Common::Path("ICON_DAT.AMB"))) {
		warning("AmberTileset: Failed to open ICON_DAT.AMB");
		return false;
	}

	Common::Path tilesetFileId(Common::String::format("%d", tilesetId));
	Common::SeekableReadStream *stream = archive.createReadStreamForMember(tilesetFileId);

	if (!stream) {
		warning("AmberTileset: Failed to extract subfile %d from ICON_DAT.AMB", tilesetId);
		archive.close();
		return false;
	}

	_playerSpriteIndex = stream->readUint16BE();

	// resize tile infos to exactly 250 (amberstar limit)
	_tileInfos.resize(250);

	// frame counts
	for (int i = 0; i < 250; i++)
		_tileInfos[i].numFrames = stream->readByte();

	// image indices
	stream->seek(0x00FC);
	uint16 maxGraphicIndex = 0;
	for (int i = 0; i < 250; i++) {
		_tileInfos[i].graphicIndex = stream->readUint16BE();

		// calculate the highest graphic index we need to allocate memory for
		if (_tileInfos[i].graphicIndex > 0) {
			uint16 highestNeeded = _tileInfos[i].graphicIndex + _tileInfos[i].numFrames - 1;
			if (highestNeeded > maxGraphicIndex)
				maxGraphicIndex = highestNeeded;
		}
	}
	// tile flags
	stream->seek(0x02F0);
	for (int i = 0; i < 250; i++)
		_tileInfos[i].flags = stream->readUint32BE();

	// minimap colors
	stream->seek(0x06D8);
	for (int i = 0; i < 250; i++)
		_tileInfos[i].minimapColor = stream->readByte();

	// wide palette
	stream->seek(0x07D2);
	uint16 numColors = stream->readUint16BE();
	byte tilePalette[32 * 3] = {0}; // max 32 colors, though amberstar usually uses 16
	stream->seek(0x07D2);
	decodeWidePalette(stream, tilePalette);

	g_system->getPaletteManager()->setPalette(tilePalette, 32, numColors);

	// sprites
	stream->seek(0x0814);

	_graphics.resize(maxGraphicIndex + 1);
	for (uint16 i = 0; i <= maxGraphicIndex; i++)
		_graphics[i] = nullptr;

	for (uint16 i = 1; i <= maxGraphicIndex; i++) {
		if (stream->eos())
			break;
		// there is a 6 byte header before every sprite
		uint16 width_m1 = stream->readUint16BE();
		uint16 height_m1 = stream->readUint16BE();
		uint16 numBitplanes = stream->readUint16BE();

		uint16 actualWidth = width_m1 + 1;
		uint16 actualHeight = height_m1 + 1;
		_graphics[i] = engine->decodePlanarGraphic(stream, actualWidth, actualHeight, numBitplanes, 32, true);
	}

	delete stream;
	archive.close();
	return true;
}

Graphics::Surface *AmberTileset::getGraphic(uint16 tileIndex, uint32 currentTicks) {
	// index 0 means transparent/empty
	if (tileIndex == 0 || tileIndex > _tileInfos.size())
		return nullptr;

	// extract the rules for this specific tile (1 based indexing)
	AmberTileInfo &info = _tileInfos[tileIndex - 1];

	if (info.graphicIndex == 0 || info.graphicIndex >= _graphics.size())
		return nullptr;

	uint16 actualGraphicIndex = info.graphicIndex;

	// basic animation (e.g. water, torches etc.)
	if (info.numFrames > 1) {
		// change the frame every 10 ticks
		uint32 frame = (currentTicks / 10) % info.numFrames;
		actualGraphicIndex += frame;
	}

	return _graphics[actualGraphicIndex];
}

AmberMap::AmberMap() {
	header.flags = 0;
	header.type = 0;
	header.musicIndex = 0;
	header.width = 0;
	header.height = 0;
	header.tileset = 0;
	header.npcGfxIndex = 0;
	header.labBgIndex = 0;
	header.paletteIndex = 0;
	header.world = 0;
	header.padding = 0;
}

AmberMap::~AmberMap() {
	characterReferences.clear();
	tiles2D.clear();
}

bool AmberMap::load(uint16 mapId) {
	AmberArchive archive;
	Common::SeekableReadStream *stream = nullptr;
	Common::String id = Common::String::format("%d", mapId);
	Common::Path mapFileId(id);

	// 2D maps are spread across 1Map_data.amb, 2Map_data.amb, and 3Map_data.amb
	// we check each archive until we find the one containing our specific mapId
	const char *mapArchives[] = {"1Map_data.amb", "2Map_data.amb", "3Map_data.amb"};

	for (int i = 0; i < 3; i++) {
		if (archive.open(mapArchives[i])) {
			if (archive.hasFile(mapFileId)) {
				stream = archive.createReadStreamForMember(mapFileId);
				break;
			}
			archive.close();
		}
	}

	if (!stream) {
		warning("AmberMap: Failed to locate or decrypt map %d in any map archive.", mapId);
		return false;
	}

	// header parsing (12 Bytes)
	// we read exactly 12 bytes in the exact order defined by the specifications
	header.flags = stream->readUint16BE();
	header.type = stream->readByte();
	header.musicIndex = stream->readByte();
	header.width = stream->readByte();
	header.height = stream->readByte();
	header.tileset = stream->readByte();
	header.npcGfxIndex = stream->readByte();
	header.labBgIndex = stream->readByte();
	header.paletteIndex = stream->readByte();
	header.world = stream->readByte();
	header.padding = stream->readByte();

	// we loop exactly 32 times because ambermoon maps always reserve 32 slots for entities
	// even if the map is completely empty
	for (int i = 0; i < 32; i++) {
		AmberCharacterRef charRef;

		charRef.index = stream->readByte();
		charRef.collisionClass = stream->readByte();
		charRef.typeAndFlags = stream->readByte();
		charRef.eventIndex = stream->readByte();
		charRef.graphicIndex = stream->readUint16BE();
		charRef.tileFlags = stream->readUint32BE();

		characterReferences.push_back(charRef);
	}

	// read 2D tiles
	if (header.type == 2) {
		int numTiles = header.width * header.height;
		tiles2D.reserve(numTiles);

		for (int i = 0; i < numTiles; i++) {
			AmberMapTile2D tile;
			tile.backTileIndex = stream->readByte();
			tile.mapEventId = stream->readByte();
			tile.frontTileIndex = stream->readUint16BE();

			tiles2D.push_back(tile);
		}
	}

	delete stream;
	archive.close();

	return true;
}

bool AmberMap::loadAmberstar(uint16 mapId) {
	AmberArchive archive;
	if (!archive.open(Common::Path("MAP_DATA.AMB"))) {
		warning("AmberMap: Failed to open MAP_DATA.AMB");
		return false;
	}

	Common::Path mapFileId(Common::String::format("%d", mapId));
	Common::SeekableReadStream *stream = archive.createReadStreamForMember(mapFileId);

	if (!stream) {
		warning("AmberMap: Failed to extract map %d from MAP_DATA.AMB", mapId);
		archive.close();
		return false;
	}

	// magic number (must be 0xFF)
	uint8 magic = stream->readByte();
	if (magic != 0xFF) {
		warning("AmberMap: Invalid magic number for Amberstar map %d", mapId);
		delete stream;
		archive.close();
		return false;
	}

	// fill byte (0x00)
	stream->readByte();

	// tileset background id (2 bytes)
	header.tileset = stream->readUint16BE();
	// map type (00 = 2D map, 01 = 3D map)
	header.type = stream->readByte();
	// map flags (light, wilderness, etc)
	header.flags = stream->readByte();
	// active song id
	header.musicIndex = stream->readByte();
	// width and height in tiles
	header.width = stream->readByte();
	header.height = stream->readByte();

	// we only care about top down 2d maps right now (type 0)
	if (header.type != 0) {
		warning("AmberMap: Map %d is a 3D labyrinth, skipping for now", mapId);
		delete stream;
		archive.close();
		return false;
	}

	// allocate the grid memory
	int numTiles = header.width * header.height;
	tiles2D.resize(numTiles);

	// maps.md explicitly states that 2d map data starts at offset 0xAC5
	// this allows us to safely bypass the 30-byte name and complex npc tables
	stream->seek(0xAC5);

	// the background floor tiles
	for (int i = 0; i < numTiles; i++)
		tiles2D[i].backTileIndex = stream->readByte();

	// the foreground overlay tiles
	for (int i = 0; i < numTiles; i++)
		tiles2D[i].frontTileIndex = stream->readByte();

	// map locations to event ids
	for (int i = 0; i < numTiles; i++)
		tiles2D[i].mapEventId = stream->readByte();

	delete stream;
	archive.close();

	return true;
}

bool AmberMap::allowMovement(int x, int y, AmberTileset *tileset, TravelType travelType, bool isAmberstar) {
	// map boundary collision
	if (x < 0 || x >= header.width || y < 0 || y >= header.height) {
		return false;
	}

	// 32-bit tile flags from IconData
	AmberMapTile2D &tile = tiles2D[y * header.width + x];

	uint32 backFlags = 0;
	uint32 frontFlags = 0;

	// extract background flags
	if (tile.backTileIndex > 0 && tile.backTileIndex <= tileset->_tileInfos.size())
		backFlags = tileset->_tileInfos[tile.backTileIndex - 1].flags;

	// extract foreground flags
	if (tile.frontTileIndex > 0 && tile.frontTileIndex <= tileset->_tileInfos.size())
		frontFlags = tileset->_tileInfos[tile.frontTileIndex - 1].flags;

	// determine the active collision flags
	// If there is no front tile, or the front tile says to use the
	// background's collision data (bit 5), we use the backFlags
	uint32 activeFlags = frontFlags;
	if (tile.frontTileIndex == 0 || (frontFlags & FLAG_USE_BACKGROUND_FLAGS))
		activeFlags = backFlags;

	// hard block check
	if (activeFlags & FLAG_BLOCK_ALL_MOVEMENT)
		return false;

	// travel type allowance starts at bit 8
	// walk (0) = bit 8, horse (1) = bit 9, raft (2) = bit 10, etc.
	// we shift a '1' by (8 + the travel type) to create our mask
	uint32 travelMask = 1 << (8 + travelType);

	if ((activeFlags & travelMask) == 0)
		return false; // the bit is 0, meaning this travel type is blocked here

	return true; // the tile is safe for this specific travel type
}

} // End of namespace Amber
