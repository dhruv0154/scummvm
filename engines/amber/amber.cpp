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
		uint32 magic = testFile.readUint16BE();

		if (magic == 0x4A48) { // JH encryption 
			uint16 key = testFile.readUint16BE() ^ magic; // read the starting key
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
			Common::MemoryReadStream stream(data, dataSize);

			uint32 magicLOB = stream.readUint32BE();
			if (magicLOB == 0x014C4F42) {
				debug("LOB Compression!");

				// read the next 4 bytes (LOB header)
				uint32 lobHeader = stream.readUint32BE();

				// extract the size and type using pyrdacor's c# logic
				uint32 decodedSize = lobHeader & 0x00FFFFFF;
				uint8 lobType = lobHeader >> 24;

				uint32 compressedSize = stream.readUint32BE();

				debug("Algorithm Type: 0x%02X", lobType);
				debug("Uncompressed File Size: %d bytes", decodedSize);
				debug("Compressed Size: %d bytes", compressedSize);

				byte *decodedData = (byte *)malloc(decodedSize);
				uint32 decodeIndex = 0;

				while (decodeIndex < decodedSize && !stream.eos()) {
					// the control byte
					// read 1 byte, the 8 bits inside tell us what the next 8 items are [literal or (distance, length)]
					uint8 header = stream.readByte();

					// process all the 8 items for this group
					for (int i = 0; i < 8 && decodeIndex < decodedSize; i++) {

						// we check the msb of the header (if it is 0 the current item is a pointer else it is just a literal)
						if ((header & 0x80) == 0) {
							// the next 2 bytes are containing distance and length
							// in the format: 12 bits for distance 4 bits for length
							// byte1: first 4 bits have distance and next 4 bits have length
							// byte2: contains distance

							// read the first byte
							uint32 matchOffset = stream.readByte();

							// the LOB compression assumes that the minimum length of a compressed string will be 3
							// because it is not worth compressing 1 or 2 characters.
							uint32 matchLength = (matchOffset & 0x0F) + 3;

							// move the top 4 bits of byte1 to the left
							matchOffset <<= 4;

							// mask off the length data in the middle so we are left with distance
							matchOffset &= 0xFF00;

							// read byte2 and merge it in to complete the 12 bit distance number
							matchOffset |= stream.readByte();

							// calculate where in our history to look
							uint32 matchIndex = decodeIndex - matchOffset;

							// copy the old letters forward, one by one, until we hit the length
							while (matchLength-- != 0 && decodeIndex < decodedSize)
								decodedData[decodeIndex++] = decodedData[matchIndex++];
						} else {
							// it's just a regular letter, read 1 byte and save it
							decodedData[decodeIndex++] = stream.readByte();
						}
						// shift the control byte left by 1
						header <<= 1;
					}
				}

				debug("Decompression Complete!");

				char preview[10001];
				for (uint32 i = 0; i < 1000; i++)
					preview[i] = (decodedData[i] >= 32 && decodedData[i] <= 126) ? decodedData[i] : '.';

				preview[1000] = '\0';
				debug("%s", preview);

				free(decodedData);
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
