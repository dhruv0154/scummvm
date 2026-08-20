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

#include "murphy3d/murphy3d.h"
#include "graphics/framelimiter.h"
#include "murphy3d/detection.h"
#include "murphy3d/console.h"
#include "common/compression/access_lzw.h"
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/system.h"
#include "common/file.h"
#include "common/stream.h"
#include "murphy3d/archive.h"
#include "murphy3d/item.h"
#include "murphy3d/ptf_decoder.h"
#include "engines/util.h"
#include "graphics/paletteman.h"

namespace Murphy3d {

Murphy3dEngine *g_engine;

Murphy3dEngine::Murphy3dEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Murphy3d") {
	g_engine = this;
}

Murphy3dEngine::~Murphy3dEngine() {
	delete _screen;
}

uint32 Murphy3dEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String Murphy3dEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error Murphy3dEngine::run() {
	initGraphics(640, 480);
	_screen = new Graphics::Screen();

	debug(0, "Murphy 3d engine starting..");

	// Set the engine's debugger console
	setDebugger(new Console());

	// If a savegame was selected from the launcher, load it
	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	// Simple event handling loop
	Common::Event e;
	int offset = 0;

	Archive titleArchive;
	PTFDecoder *decoder = nullptr;

	if (titleArchive.open("TITLE.AP")) {
		Common::SeekableReadStream *videoStream = titleArchive.getStream(0);
		if (videoStream) {
			decoder = new PTFDecoder();
			if (decoder->loadStream(videoStream)) {
				decoder->start();
			} else {
				delete decoder;
				decoder = nullptr;
			}
		}
	}

	Graphics::FrameLimiter limiter(g_system, 60);
	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
		}

		if (decoder && !decoder->endOfVideo()) {
			if (decoder->needsUpdate()) {
				const Graphics::Surface *frame = decoder->decodeNextFrame();

				if (frame) {
					if (decoder->hasDirtyPalette()) {
						const byte *pal = decoder->getPalette();
						g_system->getPaletteManager()->setPalette(pal, 0, 256);

						byte blackIndex = 0;
						for (int i = 0; i < 256; i++) {
							if (pal[i * 3] == 0 && pal[i * 3 + 1] == 0 && pal[i * 3 + 2] == 0) {
								blackIndex = i;
								break;
							}
						}

						g_system->fillScreen(blackIndex);
					}

					int drawX = (640 - frame->w) / 2;
					int drawY = (480 - frame->h) / 2;

					g_system->copyRectToScreen(frame->getPixels(), frame->pitch, drawX, drawY, frame->w, frame->h);
				}
			}
		}

		
		// Delay for a bit. All events loops should have a delay
		// to prevent the system being unduly loaded
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
	}

	if (decoder) {
		delete decoder;
	}

	return Common::kNoError;
}

Common::Error Murphy3dEngine::syncGame(Common::Serializer &s) {
	// The Serializer has methods isLoading() and isSaving()
	// if you need to specific steps; for example setting
	// an array size after reading it's length, whereas
	// for saving it would write the existing array's length
	int dummy = 0;
	s.syncAsUint32LE(dummy);

	return Common::kNoError;
}

} // End of namespace Murphy3d
