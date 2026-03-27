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

#include "amber/amber.h"
#include "graphics/framelimiter.h"
#include "amber/detection.h"
#include "amber/console.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "engines/util.h"
#include "graphics/paletteman.h"
#include "common/file.h"
#include "common/memstream.h"
#include "archive.h"
#include "graphics/cursorman.h"
#include "character_creator.h"
#include "amber_map.h"
#include "ambermoon_person.h"

namespace Amber {

AmberEngine *g_engine;

bool AmberPlayer::loadGraphics(AmberEngine *engine, uint16 playerGfxIndex) {
	AmberArchive archive;

	if (!archive.open(Common::Path("Party_gfx.amb"))) {
		warning("AmberPlayer: Party_gfx.amb not found");
		return false;
	}

	Common::Path gfxFileId(Common::String::format("%d", playerGfxIndex));
	Common::SeekableReadStream *stream = archive.createReadStreamForMember(gfxFileId);

	// party graphics use 3 frames per walking direction
	// Up = 0,1,2, Right = 3,4,5, Down = 6,7,8, Left = 9,10,11
	// after frame 11, the file contains sitting and sleeping frames
	int targetFrames[4] = {0, 3, 6, 9};
	int currentFrame = 0;

	for (int dir = 0; dir < 4; dir++) {
		if (stream->eos())
			break;

		// move the stream to the exact start of the next directional animation
		while (currentFrame < targetFrames[dir]) {
			stream->skip(320);
			currentFrame++;
		}

		// decode the frame for this direction
		sprites[dir] = engine->decodePlanarGraphic(stream, 16, 32, 5, 0);

		// the decode planar graphic function consumes 320 bytes (1 frame)
		currentFrame++;
	}

	delete stream;
	archive.close();
	return true;
}

AmberEngine::AmberEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Amber") {
	g_engine = this;
	_font = new AmberFont();
	_cursor = new AmberCursor();
	_ui = new AmberUI();
	for (int i = 0; i < 6; ++i)
		_party[i] = nullptr;
}

AmberEngine::~AmberEngine() {
	delete _screen;
	delete _font;
	delete _cursor;
	delete _ui;
	for (int i = 0; i < 6; i++) {
		if (_party[i])
			delete _party[i];
	}
}

uint32 AmberEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String AmberEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error AmberEngine::run() {
	// Initialize 320x200 paletted graphics mode
	initGraphics(320, 200);
	_screen = new Graphics::Screen();

	// Set the engine's debugger console
	setDebugger(new Console());

	// If a savegame was selected from the launcher, load it
	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	initGame();
	initWorld();

	Graphics::FrameLimiter limiter(g_system, 60);

	while (!shouldQuit()) {
		handleInput();
		renderFrame();
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	return Common::kNoError;
}

bool AmberEngine::initGame() {
	// open the main executable file of the game
	// this file contains the core code, but also holds ui graphics,
	// cursors, and fonts packed inside its data chunks
	Common::File cpuFile;
	if (!cpuFile.open("AM2_CPU")) {
		warning("failed to open AM2_CPU");
		return false;
	}

	// load the file into our amiga executable parser
	// so we can read the individual memory blocks (hunks)
	AmigaExecutable exe;
	if (!exe.load(&cpuFile)) {
		warning("failed to parse AM2_CPU");
		return false;
	}

	// pass the parsed executable to our cursor class
	// it finds the cursor graphics and the base ui palette
	if (_cursor->load(exe, this)) {
		// the ui palette is 32 colors, we set it at index 0,
		// and we also make a copy of it at index 32 because later maps will load
		// their palette in the slots 0-31 which will override our ui palette,
		// so, we make a copy of it in empty slots to save it
		g_system->getPaletteManager()->setPalette(_cursor->getUIPalette(), 0, 32);
		g_system->getPaletteManager()->setPalette(_cursor->getUIPalette(), 32, 32);

		// grab the main sword cursor (index 0) and tell scummvm to use it
		CursorData *mousePointer = _cursor->getCursor(0);
		if (mousePointer && mousePointer->surface) {
			CursorMan.pushCursor(
				mousePointer->surface->getPixels(), mousePointer->surface->w, mousePointer->surface->h,
				mousePointer->hotspotX, mousePointer->hotspotY,
				24, false, &mousePointer->surface->format);
			CursorMan.showMouse(true);
		}
	}

	// pass the executable to our ui and font classes
	// so they can extract the window borders, buttons, and text characters
	_ui->load(exe, this);
	_font->load(exe);

	return true;
}

void AmberEngine::initWorld() {
	// launch the character creator screen first
	CharacterCreator cc(this);
	cc.execute();

	// slot 0: the leader created by the player in character creater
	Common::String leaderName = cc.getSelectedName();
	uint16 leaderPortrait = cc.getSelectedPortraitId();
	_party[0] = new AmbermoonPerson(leaderName, leaderPortrait, 50, 0, false);

	// slot 1: a female wizard for testing the ui
	_party[1] = new AmbermoonPerson("WIZARD", 31, 20, 40, true);

	// slot 2: a damaged warrior for testing the hp bars
	_party[2] = new AmbermoonPerson("WARRIOR", 25, 60, 0, false);
	_party[2]->_currentHP = 30;

	// tell the ui to load the graphics for the portraits of the party members we just created
	_ui->loadPartyPortraits(this);

	_screen->fillRect(Common::Rect(0, 0, 320, 200), 0);
	_screen->update();

	// load map 258, which is the starting grandfarther house map for ambermoon
	// if the map loads successfully, we also tell the tileset class to load
	// the specific tiles that this map requires
	if (_map.load(258)) {
		_tileset.load(_map.header.tileset, this);
	}

	// open the palette archive to find the specific colors for this map
	AmberArchive palArchive;
	if (palArchive.open(Common::Path("Palettes.amb"))) {
		Common::Path palFileId(Common::String::format("%d", _map.header.paletteIndex));
		if (palArchive.hasFile(palFileId)) {
			Common::SeekableReadStream *palStream = palArchive.createReadStreamForMember(palFileId);
			if (palStream) {
				// pass the file stream to our palette decoder to add the map colors
				loadAmigaPalette(palStream);
				delete palStream;
			}
		}
		palArchive.close();
	}

	if (!_player.loadGraphics(this, 1))
		warning("Failed to load player graphics. visuals may be missing.");

	_player.mapX = 15;
	_player.mapY = 15;
	_player.facing = DIR_DOWN;
	_cameraTileX = _player.mapX;
	_cameraTileY = _player.mapY;

	// load the graphical layout overlay that goes around the 2D map viewport
	_ui->loadExplorationLayout(this);

	// this flag can be toggled to test different collision rules later
	_isAmberstar = false;
}

void AmberEngine::handleInput() {
	Common::Event e;

	while (g_system->getEventManager()->pollEvent(e)) {
		int dx = 0;
		int dy = 0;
		if (e.type == Common::EVENT_KEYDOWN) {
			// check which arrow key was pressed and update the player's facing direction
			// also set the delta x (dx) or delta y (dy) to move 1 tile in that direction
			if (e.kbd.keycode == Common::KEYCODE_UP) {
				dy = -1;
				_player.facing = DIR_UP;
			} else if (e.kbd.keycode == Common::KEYCODE_DOWN) {
				dy = 1;
				_player.facing = DIR_DOWN;
			} else if (e.kbd.keycode == Common::KEYCODE_LEFT) {
				dx = -1;
				_player.facing = DIR_LEFT;
			} else if (e.kbd.keycode == Common::KEYCODE_RIGHT) {
				dx = 1;
				_player.facing = DIR_RIGHT;
			}
		} else if (e.type == Common::EVENT_LBUTTONDOWN) {
			Common::Point mousePos = g_system->getEventManager()->getMousePos();

			// check if click is inside the 3x3 movement pad (X: 208-304, Y: 143-194)
			if (mousePos.x >= 208 && mousePos.x < 304 && mousePos.y >= 143 && mousePos.y < 194) {
				int col = (mousePos.x - 208) / 32;
				int row = (mousePos.y - 143) / 17;
				_pressedButtonIndex = row * 3 + col;
			}
		} else if (e.type == Common::EVENT_MOUSEMOVE) {
			if (_pressedButtonIndex != -1) {
				Common::Point mousePos = g_system->getEventManager()->getMousePos();
				int col = _pressedButtonIndex % 3;
				int row = _pressedButtonIndex / 3;
				Common::Rect btnRect(208 + col * 32, 143 + row * 17, 208 + (col + 1) * 32, 143 + (row + 1) * 17);

				if (!btnRect.contains(mousePos)) {
					_pressedButtonIndex = -1; // release the button
				}
			}
		} else if (e.type == Common::EVENT_LBUTTONUP) {
			if (_pressedButtonIndex != -1) {
				Common::Point mousePos = g_system->getEventManager()->getMousePos();
				int col = _pressedButtonIndex % 3;
				int row = _pressedButtonIndex / 3;
				Common::Rect btnRect(208 + col * 32, 143 + row * 17, 208 + (col + 1) * 32, 143 + (row + 1) * 17);

				if (btnRect.contains(mousePos)) {
					// map the button index to movement directions
					if (_pressedButtonIndex == 0) {
						dx = -1;
						dy = -1;
						_player.facing = DIR_LEFT;
					} else if (_pressedButtonIndex == 1) {
						dy = -1;
						_player.facing = DIR_UP;
					} else if (_pressedButtonIndex == 2) {
						dx = 1;
						dy = -1;
						_player.facing = DIR_RIGHT;
					} else if (_pressedButtonIndex == 3) {
						dx = -1;
						_player.facing = DIR_LEFT;
					} else if (_pressedButtonIndex == 5) {
						dx = 1;
						_player.facing = DIR_RIGHT;
					} else if (_pressedButtonIndex == 6) {
						dx = -1;
						dy = 1;
						_player.facing = DIR_LEFT;
					} else if (_pressedButtonIndex == 7) {
						dy = 1;
						_player.facing = DIR_DOWN;
					} else if (_pressedButtonIndex == 8) {
						dx = 1;
						dy = 1;
						_player.facing = DIR_RIGHT;
					}
				}
				_pressedButtonIndex = -1;
			}
		}

		// if dx or dy is not zero, it means the player actually tried to move
		if (dx != 0 || dy != 0) {
			// calculate the tile the player wants to step onto
			int targetX = _player.mapX + dx;
			int targetY = _player.mapY + dy;

			// ask the map if this specific tile allows walking
			// if it does, we update the player's actual position
			if (_map.allowMovement(targetX, targetY, &_tileset, TRAVEL_WALK, _isAmberstar)) {
				_player.mapX = targetX;
				_player.mapY = targetY;

				// lock the camera to follow the player new position
				_cameraTileX = _player.mapX;
				_cameraTileY = _player.mapY;
			}
		}
	}
}

void AmberEngine::renderFrame() {
	// the standard ambermoon 2D viewport is 11 tiles wide by 9 tiles high
	int viewWidthTiles = 11;
	int viewHeightTiles = 9;

	// these are the exact screen coordinates where the top left corner of the map sits
	int startX = UIConstants::MAP_VIEW_X;
	int startY = UIConstants::MAP_VIEW_Y;

	// draw the static user interface elements
	_ui->drawExplorationLayout(_screen);
	_ui->drawPortraitBar(_screen, this);

	for (int i = 0; i < 9; i++) {
		int col = i % 3;
		int row = i / 3;
		int btnX = 208 + col * 32;
		int btnY = 143 + row * 17;
		bool isPressed = (_pressedButtonIndex == i);

		// draw the stone button background
		_ui->drawButton(_screen, btnX, btnY, isPressed);

		// draw the actual icon on top
		// the icon is shifted down by 2 pixels (or 4 if pressed)
		int iconYOffset = isPressed ? 4 : 2;

		// the icon uses color 24 as the transparency key
		if (_ui->_buttonIcons[i])
			_screen->transBlitFrom(*_ui->_buttonIcons[i], Common::Point(btnX, btnY + iconYOffset), 24);
	}

	// render the map grid
	// we loop through every visible row (y) and column (x) in the camera's view
	for (int y = 0; y < viewHeightTiles; y++) {
		for (int x = 0; x < viewWidthTiles; x++) {

			// calculate the actual map coordinate for this specific screen tile
			int mapX = _cameraTileX + x - (viewWidthTiles / 2);
			int mapY = _cameraTileY + y - (viewHeightTiles / 2);

			// calculate the exact pixel coordinate on the screen to draw this tile
			int screenX = startX + x * 16;
			int screenY = startY + y * 16;

			// check if this coordinate is actually inside the map boundaries
			if (mapX >= 0 && mapX < _map.header.width && mapY >= 0 && mapY < _map.header.height) {
				AmberMapTile2D &tile = _map.tiles2D[mapY * _map.header.width + mapX];

				// calculate the current animation tick to make basic animations work
				uint32 currentTicks = g_system->getMillis() / 10;

				// draw the floor graphic (background)
				Graphics::Surface *backGfx = _tileset.getGraphic(tile.backTileIndex, currentTicks);
				if (backGfx)
					_screen->transBlitFrom(*backGfx, Common::Point(screenX, screenY), 0);

				// draw the wall/furniture graphic (foreground)
				Graphics::Surface *frontGfx = _tileset.getGraphic(tile.frontTileIndex, currentTicks);
				if (frontGfx)
					_screen->transBlitFrom(*frontGfx, Common::Point(screenX, screenY), 0);
			} else {
				// if we are looking outside the map, draw a black 16x16 square
				_screen->fillRect(Common::Rect(screenX, screenY, screenX + 16, screenY + 16), 0);
			}
		}
	}

	// render the player character
	// find the center of the map viewport
	int playerScreenX = startX + (viewWidthTiles / 2) * 16;

	// the player sprite is 32 pixels tall, but tiles are only 16 pixels tall
	// shift the y position up by 16 pixels so the character's feet touch the ground
	int playerScreenY = startY + (viewHeightTiles / 2) * 16 - 16;

	Graphics::Surface *activeSprite = _player.sprites[_player.facing];
	if (activeSprite)
		_screen->transBlitFrom(*activeSprite, Common::Point(playerScreenX, playerScreenY), 0);
}

void AmberEngine::loadAmigaPalette(Common::SeekableReadStream *stream) {
	// 32 colors * 3 bytes per color = 96 bytes total
	// there are total 32 colors in Pallette.amb in game files
	// each color is packed in 2 bytes
	byte colorPalette[32 * 3];

	for (int i = 0; i < 32; ++i) {

		// reads the 16 bit 0x0RGB color
		// 0x0RGB because amiga could not read 12 bits it could only read (8, 16, 32)
		// so 4 bits are wasted due to amiga hardware limitations
		uint16 amigaColor = stream->readUint16BE();

		// we extract the 4 bit amiga RGB nibbles
		uint8 r = (amigaColor >> 8) & 0x0F;
		uint8 g = (amigaColor >> 4) & 0x0F;
		uint8 b = amigaColor & 0x0F;

		// scales the 4 bit amiga color (0-15) to an 8 bit color (0-255)
		// by shifting the bits left by 4 and bitwise OR them with the original value
		colorPalette[i * 3 + 0] = (r << 4) | r; // Red
		colorPalette[i * 3 + 1] = (g << 4) | g; // Green
		colorPalette[i * 3 + 2] = (b << 4) | b; // Blue
	}

	g_system->getPaletteManager()->setPalette(colorPalette, 0, 32);
}

Graphics::Surface *AmberEngine::decodePlanarGraphic(Common::SeekableReadStream *stream, uint16 width,
													uint16 height, uint8 planes, uint8 paletteOffset) {
	Graphics::Surface *surface = new Graphics::Surface();

	surface->create(width, height, Graphics::PixelFormat::createFormatCLUT8());

	// amiga packs 8 pixels into 1 byte, we divide width by 8 to find bytes per row
	// +7 is used for cases like if an image is 33 pixels wide 33/2 = 4 but we have 1 pixel left over
	// we need 5 bytes to hold 33 pixels (33+7)/8 = 5
	uint32 bytesPerRow = (width + 7) / 8;
	uint32 planeSize = bytesPerRow * height;   // total bytes for one single plane
	uint32 totalDataSize = planeSize * planes; // total bytes for all planes combined

	byte *planarData = (byte *)malloc(totalDataSize);
	stream->read(planarData, totalDataSize);

	// decode the image pixel by pixel
	for (uint16 y = 0; y < height; y++) {
		// grab a pointer to the start of the current row on our surface
		byte *row = (byte *)surface->getBasePtr(0, y);

		for (uint16 x = 0; x < width; x++) {
			// on which bit on this byte is our coordinate
			uint8 bitIndex = x % 8;
			byte paletteIndex = 0; // this will hold our final, stacked color index

			uint32 rowOffset = y * (bytesPerRow * planes);
			uint32 byteOffset = x / 8;

			// stack the planes to build the color since amiga uses plane graphics
			for (uint8 p = 0; p < planes; p++) {

				// jump to the correct plane's memory block, then find our specific byte
				uint32 planeOffset = p * bytesPerRow;
				uint32 dataOffset = rowOffset + planeOffset + byteOffset;

				// amiga reads bits visually from left to right (big endian)
				// so if we want pixel 0, we check bit 7, pixel 1 = bit 6,..
				if (planarData[dataOffset] & (1 << (7 - bitIndex))) {
					// if there is a 1 on this plane, add its value to our color
					paletteIndex |= (1 << p);
				}
			}

			row[x] = paletteIndex + paletteOffset;
		}
	}

	free(planarData);
	return surface;
}

Graphics::Surface *AmberEngine::decodePlanarGraphic(const byte *planarData, uint16 width,
													uint16 height, uint8 planes, uint8 paletteOffset) {
	Graphics::Surface *surface = new Graphics::Surface();
	surface->create(width, height, Graphics::PixelFormat::createFormatCLUT8());

	uint32 bytesPerRow = (width + 7) / 8;

	for (uint16 y = 0; y < height; y++) {
		byte *row = (byte *)surface->getBasePtr(0, y);

		for (uint16 x = 0; x < width; x++) {
			uint8 bitIndex = x % 8;
			byte paletteIndex = 0;

			uint32 rowOffset = y * (bytesPerRow * planes);
			uint32 byteOffset = x / 8;

			for (uint8 p = 0; p < planes; p++) {
				uint32 planeOffset = p * bytesPerRow;
				uint32 dataOffset = rowOffset + planeOffset + byteOffset;

				if (planarData[dataOffset] & (1 << (7 - bitIndex))) {
					paletteIndex |= (1 << p);
				}
			}
			row[x] = paletteIndex + paletteOffset;
		}
	}

	return surface;
}

Common::Error AmberEngine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace Amber
