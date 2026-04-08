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

#ifndef AMBER_H
#define AMBER_H

#include "common/scummsys.h"
#include "common/system.h"
#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/serializer.h"
#include "common/util.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"

#include "amber/detection.h"
#include "font.h"
#include "cursor.h"
#include "amiga.h"
#include "ui.h"
#include "amber_person.h"
#include "amber_map.h"
#include "game_context.h"

namespace Amber {

enum CharacterDirection {
	DIR_UP = 0,
	DIR_RIGHT = 1,
	DIR_DOWN = 2,
	DIR_LEFT = 3
};

class AmberPlayer {
public:
	int mapX;
	int mapY;
	CharacterDirection facing;

	// 4 directional frames
	Graphics::Surface *sprites[4];


	AmberPlayer() : mapX(0), mapY(0), facing(DIR_DOWN) {
		for (int i = 0; i < 4; i++)
			sprites[i] = nullptr;
	}

	~AmberPlayer() {
		for (int i = 0; i < 4; i++) {
			if (sprites[i]) {
				sprites[i]->free();
				delete sprites[i];
			}
		}
	}

	// loads the default party leader graphic from Party_gfx.amb
	bool loadGraphics(AmberEngine *engine, uint16 playerGfxIndex = 1);
};

struct AmberGameDescription;

class AmberEngine : public Engine {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	GameContext* _context;

	AmberMap _map; // the current map data
	AmberTileset _tileset; // the graphics for the map
	AmberPlayer _player;
	int _pressedButtonIndex = -1;
	int _cameraTileX;
	int _cameraTileY;
	bool _isAmberstar; // toggle for collision testing

	bool initGame(); // loads core files like cursors, fonts, and ui
	void initWorld(); // runs character creator and loads the starting map
	void initAmberstarWorld();
	void handleInput();
	void renderAmberstarFrame();
	void renderFrame(); // draws the map, player, and ui to the screen

protected:
	// Engine APIs
	Common::Error run() override;
public:
	AmberFont *_font;
	AmberCursor *_cursor;
	AmberUI *_ui;
	Graphics::Screen *_screen = nullptr;
	AmberPerson *_party[6];

public:
	AmberEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~AmberEngine() override;

	void loadAmigaPalette(Common::SeekableReadStream *stream);
	Graphics::Surface *decodePlanarGraphic(Common::SeekableReadStream *stream, uint16 width,
										   uint16 height, uint8 planes, uint8 paletteOffset = 0,
										   bool isChunkInterleaved = false);
	Graphics::Surface *decodePlanarGraphic(const byte *planarData, uint16 width,
										   uint16 height, uint8 planes, uint8 paletteOffset = 0,
										   bool isChunkInterleaved = false);

	uint32 getFeatures() const;

	/**
	 * Returns the game Id
	 */
	Common::String getGameId() const;

	/**
	 * Gets a random number
	 */
	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return
		    (f == kSupportsLoadingDuringRuntime) ||
		    (f == kSupportsSavingDuringRuntime) ||
		    (f == kSupportsReturnToLauncher);
	};

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}

	/**
	 * Uses a serializer to allow implementing savegame
	 * loading and saving using a single method
	 */
	Common::Error syncGame(Common::Serializer &s);

	Common::Error saveGameStream(Common::WriteStream *stream, bool isAutosave = false) override {
		Common::Serializer s(nullptr, stream);
		return syncGame(s);
	}
	Common::Error loadGameStream(Common::SeekableReadStream *stream) override {
		Common::Serializer s(stream, nullptr);
		return syncGame(s);
	}
};

extern AmberEngine *g_engine;
#define SHOULD_QUIT ::Amber::g_engine->shouldQuit()

} // End of namespace Amber

#endif // AMBER_H
