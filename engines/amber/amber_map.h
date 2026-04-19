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

#ifndef AMBER_MAP_H
#define AMBER_MAP_H

#include "common/array.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "graphics/surface.h"

namespace Amber {

class AmberEngine;

// 8-byte data struct for every single tile (from Icon_data.amb)
struct AmberTileInfo {
	uint32 flags;        // collision, transparency etc.
	uint16 graphicIndex; // 1 based pixel graphic index (0 = empty)
	uint8 numFrames;     // animation frames (1 = static)
	uint8 minimapColor;  // automap fallback color
};

class AmberTileset {
public:
	Common::Array<AmberTileInfo> _tileInfos;
	Common::Array<Graphics::Surface *> _graphics; // array of pointers to the raw pixels
	uint16 _playerSpriteIndex;

	AmberTileset();
	~AmberTileset();

	bool load(uint16 tilesetId, AmberEngine *engine);
	bool loadAmberstar(uint16 tilesetId, AmberEngine *engine);
	Graphics::Surface *getGraphic(uint16 tileIndex, uint32 currentTicks);
};

enum TravelType {
	TRAVEL_WALK = 0,
	TRAVEL_HORSE = 1,
	TRAVEL_RAFT = 2,
	TRAVEL_SHIP = 3,
	TRAVEL_MAGICAL_DISC = 4,
	TRAVEL_EAGLE = 5,
	TRAVEL_FLY = 6,
	TRAVEL_SWIM = 7,
	TRAVEL_WITCH_BROOM = 8,
	TRAVEL_SAND_LIZARD = 9,
	TRAVEL_SAND_SHIP = 10,
	TRAVEL_WASP = 11
};

// bitmasks used in the 32-bit IconData flags
enum TileFlags {
	FLAG_BLOCK_SIGHT = 0x02,          // bit 1
	FLAG_USE_BACKGROUND_FLAGS = 0x20, // bit 5
	FLAG_BLOCK_ALL_MOVEMENT = 0x80    // bit 7
};

// 16-bit map flags
enum MapFlags {
	MAP_INDOOR = 1 << 0,
	MAP_OUTDOOR = 1 << 1,
	MAP_DUNGEON = 1 << 2,
	MAP_AUTOMAPPER = 1 << 3,
	MAP_CAN_REST = 1 << 4,
	MAP_UNKNOWN_1 = 1 << 5,
	MAP_SKY = 1 << 6,
	MAP_NO_SLEEP_DAWN = 1 << 7,
	MAP_STATIONARY_GFX = 1 << 8,
	MAP_UNKNOWN_2 = 1 << 9,
	MAP_WORLD_SURFACE = 1 << 10,
	MAP_CAN_USE_MAGIC = 1 << 11
};

// the 12-byte header at the very start of every map file
// this maps 1:1 to the very first 12 bytes of every Map_data.amb file
// by reading this first, the engine knows exactly how much memory to allocate for the rest of the map
struct AmberMapHeader {
	uint16 flags; // bit mask flags defined in MapFlags enum
	uint8 type; // tells the engine which renderer to use (1 = 3D raycaster, 2 = 2D top down)
	uint8 musicIndex; // which amiga SonicArranger tracker module to play
	uint8 width; // map width
	uint8 height; // map height
	uint8 tileset; // the ID of the graphical asset file to load, for 2D: tileset index (1-8), for 3D: lab data index
	uint8 npcGfxIndex; // which sprite sheet to load for characters
	uint8 labBgIndex; // for 3D maps, this determines the skybox or background
	uint8 paletteIndex; // which 32-color amiga palette to apply to the screen
	uint8 world; // 0: lyramion, 1: forest moon, 2: morag
	uint8 padding; // always 0 for cpu alignment
};

// directly following the map header, ambermoon always stores exactly 32 of these (taking up 320 bytes)
// it tells the engine where the player spawns and what NPCs exist on this specific map
struct AmberCharacterRef {
	uint8 index; // party member, NPC, monster group, or map text index
	uint8 collisionClass; // usually matches travel type, determines what this entity can walk through (water, mountains, walls)
	uint8 typeAndFlags; // the lower 2 bits tell us if it's an NPC, monster, or player, the upper 6 bits tell us if it moves randomly, stands still, etc.
	uint8 eventIndex; // if you interact with this entity, this points to the conversation/action script
	uint16 graphicIndex; // which specific frame of animation to draw on the map
	uint32 tileFlags; // special rules that override the ground they are standing on
};

// 4-byte struct for a 2D tile
// this represents a single grid square
struct AmberMapTile2D {
	uint8 backTileIndex; // the floor (grass, stone, water), max 255 unique tiles
	uint8 mapEventId; // if > 0, stepping on (or clicking) this tile triggers a script (chest, door, trap)
	uint16 frontTileIndex; // the object covering the floor (table, tree, wall), can hold up to 65,535 unique tiles
};

class AmberMap {
public:
	AmberMapHeader header;
	Common::Array<AmberCharacterRef> characterReferences; // always 32
	Common::Array<AmberMapTile2D> tiles2D; // size will be width * height

	bool allowMovement(int x, int y, AmberTileset *tileset, TravelType travelType, bool isAmberstar);

	AmberMap();
	~AmberMap();

	bool load(uint16 mapId);
	bool loadAmberstar(uint16 mapId);
};

} // End of namespace Amber

#endif // AMBER_MAP_H
