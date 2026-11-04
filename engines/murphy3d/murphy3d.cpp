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
#include "common/compression/access_lzw.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/file.h"
#include "common/scummsys.h"
#include "common/stream.h"
#include "common/system.h"
#include "common/translation.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/framelimiter.h"
#include "graphics/paletteman.h"
#include "murphy3d/archive.h"
#include "murphy3d/console.h"
#include "murphy3d/detection.h"
#include "murphy3d/item.h"
#include "murphy3d/ptf_decoder.h"

#include "murphy3d/location.h"
#include "murphy3d/math_utils.h"
#include "murphy3d/renderer.h"
#include "murphy3d/uakm_map.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/uakm_game.h"

namespace Murphy3d {

Murphy3dEngine *g_engine;

Murphy3dEngine::Murphy3dEngine(OSystem *syst, const ADGameDescription *gameDesc)
	: Engine(syst), _gameDescription(gameDesc), _randomSource("Murphy3d"),
	  _game(nullptr), _screen(nullptr), _renderer(nullptr), _font(nullptr) {
	g_engine = this;
}

Murphy3dEngine::~Murphy3dEngine() {
	delete _game;
	delete _font;
	delete _screen;
	delete _renderer;
}

void Murphy3dEngine::resumeGame() {
	ModuleBase *mainMenu = ModuleController::get(ModuleType::kMainMenu);
	if (mainMenu != nullptr) {
		ModuleController::sendToBack(mainMenu);
	}
}

void Murphy3dEngine::showMainMenu() {
	ModuleBase *mainMenu = ModuleController::get(ModuleType::kMainMenu);
	if (mainMenu != nullptr) {
		ModuleController::sendToFront(mainMenu);
	}
}

void Murphy3dEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
	// Chapter 1 is present on disc 2 and contains the initial Tex's Office
	// location. Support both a copied disc root and a multi-disc directory.
	SearchMan.addSubDirectoryMatching(gamePath, "MASTERTX/CHAP01", 0, 4);
	SearchMan.addSubDirectoryMatching(gamePath, "DISK2/MASTERTX/CHAP01", 0, 4);
}

uint32 Murphy3dEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String Murphy3dEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error Murphy3dEngine::run() {
	if (!g_system->hasFeature(OSystem::kFeatureShadersForGame))
		return Common::Error(Common::kUnknownError, _s("This game requires OpenGL with shaders, which is not supported on your system"));

	initGraphics3d(640, 480);
	_screen = new Graphics::Screen();

	debug(0, "Murphy 3d engine starting..");

	// Set the engine's debugger console
	setDebugger(new Console());

	// If a savegame was selected from the launcher, load it
	int saveSlot = ConfMan.getInt("save_slot");
	if (saveSlot != -1)
		(void)loadGameState(saveSlot);

	_renderer = new Renderer();
	if (!_renderer->init()) {
		warning("Murphy3d: Failed to initialize OpenGL 3D Renderer!");
		return Common::Error(Common::kUnknownError, _s("Failed to initialize the OpenGL renderer"));
	}

	_font = new Font();
	_font->load("UAKMFont.png");

	GameController::init();
	_game = new UAKMGame();
	GameController::startGame(_game);

	Common::Event e;
	Graphics::FrameLimiter limiter(g_system, 60);

	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
			switch (e.type) {
			case Common::EVENT_QUIT:
			case Common::EVENT_RETURN_TO_LAUNCHER:
				return Common::kNoError;

			case Common::EVENT_MOUSEMOVE:
				ModuleController::mouseMove(e.mouse);
				break;

			case Common::EVENT_LBUTTONDOWN:
				ModuleController::mouseDown(e.mouse, 1);
				break;

			case Common::EVENT_LBUTTONUP:
				ModuleController::mouseUp(e.mouse, 1);
				break;

			case Common::EVENT_RBUTTONDOWN:
				ModuleController::mouseDown(e.mouse, 2);
				break;

			case Common::EVENT_RBUTTONUP:
				ModuleController::mouseUp(e.mouse, 2);
				break;

			case Common::EVENT_WHEELUP:
				ModuleController::mouseWheel(1);
				break;

			case Common::EVENT_WHEELDOWN:
				ModuleController::mouseWheel(-1);
				break;

			case Common::EVENT_KEYDOWN:
				ModuleController::keyDown((uint32)e.kbd.keycode, (uint32)e.kbd.flags);
				break;

			case Common::EVENT_KEYUP:
				ModuleController::keyUp((uint32)e.kbd.keycode, (uint32)e.kbd.flags);
				break;

			default:
				break;
			}
		}

		ModuleController::render();

		// Delay for a bit. All events loops should have a delay
		// to prevent the system being unduly loaded
		limiter.delayBeforeSwap();
		g_system->updateScreen();
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
