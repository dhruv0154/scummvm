#include "murphy3d/murphy3d.h"
#include "common/config-manager.h"
#include "common/debug-channels.h"
#include "common/events.h"
#include "common/scummsys.h"
#include "common/system.h"
#include "common/translation.h"
#include "engines/util.h"
#include "graphics/cursorman.h"
#include "graphics/framelimiter.h"
#include "murphy3d/console.h"
#include "murphy3d/detection.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/renderer.h"
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

void Murphy3dEngine::startNewGame() {
	GameController::newGame();
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

void Murphy3dEngine::setActiveCursor(CursorType type) {
	if (type >= CursorType::kArrow && type <= CursorType::kSpecial) {
		int idx = (int)type;
		if (ModuleController::_cursors[idx].hasIcons()) {
			ModuleController::_cursors[idx].applyToCursorMan();
		}
	}
}

AnimatedCursor *Murphy3dEngine::getCursor(CursorType type) {
	int idx = (int)type;
	if (idx >= 0 && idx <= (int)CursorType::kSpecial)
		return &ModuleController::_cursors[idx];
	return nullptr;
}

void Murphy3dEngine::initializePath(const Common::FSNode &gamePath) {
	Engine::initializePath(gamePath);
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

	setDebugger(new Console());

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

		limiter.startFrame();
		ModuleController::render();
		limiter.delayBeforeSwap();
	}

	return Common::kNoError;
}

Common::Error Murphy3dEngine::syncGame(Common::Serializer &s) {
	int dummy = 0;
	s.syncAsUint32LE(dummy);
	return Common::kNoError;
}

} // End of namespace Murphy3d
