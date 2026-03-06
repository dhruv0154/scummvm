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

namespace Amber {

AmberEngine *g_engine;

AmberEngine::AmberEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Amber") {
	g_engine = this;
}

AmberEngine::~AmberEngine() {
	delete _screen;
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

	Common::File testFile;

	if (testFile.open("Text.amb")) {
		uint32 fileSize = testFile.size();
		debug("File size: %d bytes", fileSize);

		// amiga works in big endian unlike the modern architectures which use little endian
		uint32 firstWord = testFile.readUint16BE();

		if (firstWord == 0x4A48) { // JH encryption 
			uint16 key = testFile.readUint16BE(); // read the starting key
			uint32 dataSize = testFile.size() - 4; // subtract 2 bytes for JH (0x4A48) and 2 bytes for the key just read

			byte *data = (byte *)malloc(dataSize);

			uint32 numWords = dataSize / 2; // 2 bytes for one word
			bool hasOddByte = (dataSize % 2) != 0; // check if there is 1 leftover byte

			uint16 d0 = key;
			uint16 d1 = 0;
			uint32 offset = 0;

			// read the full 16 bit words one by one
			for (uint32 i = 0; i < numWords; ++i) {
				uint16 value = testFile.readUint16BE();
				value ^= d0;

				data[offset++] = (value >> 8) & 0xFF;
				data[offset++] = value & 0xFF;

				d1 = d0;
				d0 <<= 4;
				d0 = (d0 + d1 + 87) & 0xFFFF;
			}

			// handle the edge case if the file has an odd number of bytes
			if (hasOddByte) {
				uint16 value = testFile.readByte() << 8;
				value ^= d0;
				data[offset++] = (value >> 8) & 0xFF;
			}

			char printBuffer[81]; // 80 characters + 1 null terminator
			uint32 bufferIndex = 0;

			for (uint32 i = 0; i < dataSize; i++) {
				// filter out unreadable bytes
				printBuffer[bufferIndex++] = (data[i] >= 32 && data[i] <= 126) ? (char)data[i] : '.';

				// when we reach 80 characters, or the very last byte of the file then we print and reset the buffer
				if (bufferIndex == 80 || i == dataSize - 1) {
					printBuffer[bufferIndex] = '\0';
					debug("%s", printBuffer);
					bufferIndex = 0;
				}
			}

			free(data);
		} else {
			warning("File is not JH encrypted!");
		}

		testFile.close();
	} else {
		warning("Could not open Text.amb.");
	}

	// If a savegame was selected from the launcher, load it
	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	// Draw a series of boxes on screen as a sample
	for (int i = 0; i < 100; ++i)
		_screen->frameRect(Common::Rect(i, i, 320 - i, 200 - i), i);
	_screen->update();

	// Simple event handling loop
	byte pal[256 * 3] = { 0 };
	Common::Event e;
	int offset = 0;

	Graphics::FrameLimiter limiter(g_system, 60);
	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
		}

		// Cycle through a simple palette
		++offset;
		for (int i = 0; i < 256; ++i)
			pal[i * 3 + 1] = (i + offset) % 256;
		g_system->getPaletteManager()->setPalette(pal, 0, 256);
		// Delay for a bit. All events loops should have a delay
		// to prevent the system being unduly loaded
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	return Common::kNoError;
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
