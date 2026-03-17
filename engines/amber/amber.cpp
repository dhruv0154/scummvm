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

namespace Amber {

AmberEngine *g_engine;

AmberEngine::AmberEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Amber") {
	g_engine = this;
	_font = new AmberFont();
	_cursor = new AmberCursor();
	_ui = new AmberUI();
}

AmberEngine::~AmberEngine() {
	delete _screen;
	delete _font;
	delete _cursor;
	delete _ui;
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

	// load the game exe file
	Common::File cpuFile;
	if (!cpuFile.open("AM2_CPU"))
		error("Failed to open AM2_CPU");

	AmigaExecutable exe;
	if (!exe.load(&cpuFile))
		error("Failed to parse AM2_CPU");

	// load cursor & main color palette
	if (_cursor->load(exe, this)) {
		g_system->getPaletteManager()->setPalette(_cursor->getUIPalette(), 0, 32);

		CursorData *mousePointer = _cursor->getCursor(0); // sword cursor
		if (mousePointer && mousePointer->surface) {
			CursorMan.pushCursor(
				mousePointer->surface->getPixels(), mousePointer->surface->w, mousePointer->surface->h,
				mousePointer->hotspotX, mousePointer->hotspotY,
				24, false, &mousePointer->surface->format);
			CursorMan.showMouse(true);
		}
	}

	// load UI borders and font
	_ui->load(exe, this);
	_font->load(exe);

	_screen->fillRect(Common::Rect(0, 0, 320, 200), 0);
	_screen->update();

	CharacterCreator creator(this);
	creator.execute();

	return Common::kNoError;
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
