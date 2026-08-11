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
	// Initialize 320x200 paletted graphics mode
	initGraphics(320, 200);
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

	Archive archive;

	if (archive.open("INV.AP")) {
		debug(0, "Successfully opened INV.AP!");

		Common::SeekableReadStream *palStream = archive.getStream(0);
		if (palStream) {
			byte palette[256 * 3];
			palStream->read(palette, 256 * 3);

			for (int i = 0; i < 256 * 3; i++) {
				palette[i] = (palette[i] * 255) / 63;
			}

			g_system->getPaletteManager()->setPalette(palette, 0, 256);

			delete palStream;
		}

		Common::SeekableReadStream *spriteStream = archive.getStream(5);
		Item sprite;
		sprite.load(spriteStream);

		int screenX = (320 - sprite.getWidth()) / 2;
		int screenY = (200 - sprite.getHeight()) / 2;

		sprite.draw(_screen, screenX, screenY);

	} else {
		debug(0, "Failed to open INV.AP.");
	}

	Graphics::FrameLimiter limiter(g_system, 60);
	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
		}

		
		// Delay for a bit. All events loops should have a delay
		// to prevent the system being unduly loaded
		limiter.delayBeforeSwap();
		_screen->update();
		limiter.startFrame();
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
