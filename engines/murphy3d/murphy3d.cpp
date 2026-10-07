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
#include "common/translation.h"
#include "murphy3d/archive.h"
#include "murphy3d/item.h"
#include "murphy3d/ptf_decoder.h"
#include "engines/util.h"
#include "graphics/paletteman.h"
#include "graphics/cursorman.h"

#include "murphy3d/renderer.h"
#include "murphy3d/player.h"
#include "murphy3d/location.h"
#include "murphy3d/uakm_map.h"
#include "murphy3d/math_utils.h"

namespace Murphy3d {

Murphy3dEngine *g_engine;

Murphy3dEngine::Murphy3dEngine(OSystem *syst, const ADGameDescription *gameDesc)
	: Engine(syst), _gameDescription(gameDesc), _randomSource("Murphy3d"),
	  _state(kStateMainMenu), _font(nullptr), _mainMenu(nullptr),
	  _currentLocation(nullptr), _player(nullptr), _activeCursor(CursorType::kArrow) {
	g_engine = this;
}

Murphy3dEngine::~Murphy3dEngine() {
	delete _mainMenu;
	delete _font;
	delete _currentLocation;
	delete _player;
	delete _screen;
	delete _renderer;
}

void Murphy3dEngine::startNewGame() {
	_state = kStateInGame;

	if (_mainMenu) {
		_mainMenu->enableResume(true);
		_mainMenu->enableSave(true);
	}

	g_system->lockMouse(true);
	CursorMan.showMouse(false);
}

void Murphy3dEngine::resumeGame() {
	if (_currentLocation) {
		_state = kStateInGame;
		g_system->lockMouse(true);
		CursorMan.showMouse(false);
	}
}

void Murphy3dEngine::showMainMenu() {
	_state = kStateMainMenu;
	g_system->lockMouse(false);
	CursorMan.showMouse(true);
}

void Murphy3dEngine::setActiveCursor(CursorType type) {
	if (type < CursorType::kArrow || type > CursorType::kSpecial)
		return;

	_activeCursor = type;
	int idx = (int)_activeCursor;

	if (_cursors[idx].hasIcons()) {
		_cursors[idx].applyToCursorMan();
	} else {
		CursorMan.showMouse(true);
	}
}

AnimatedCursor *Murphy3dEngine::getCursor(CursorType type) {
	int idx = (int)type;
	if (idx >= 0 && idx <= (int)CursorType::kSpecial)
		return &_cursors[idx];
	return nullptr;
}

bool Murphy3dEngine::loadCursors() {
	Archive gfxArchive;
	if (!gfxArchive.open("GRAPHICS.AP")) {
		warning("Murphy3d: Failed to open GRAPHICS.AP for cursor palette");
		return false;
	}

	Common::SeekableReadStream *palStream = gfxArchive.getStream(0);
	if (!palStream)
		return false;

	byte basePalette[768];
	palStream->read(basePalette, 768);
	delete palStream;
	gfxArchive.close();

	Common::File iconFile;
	if (!iconFile.open("ICONS.LZ")) {
		warning("Murphy3d: Failed to open ICONS.LZ");
		return false;
	}

	uint32 fileSize = iconFile.size();
	byte *compressedData = new byte[fileSize];
	iconFile.read(compressedData, fileSize);
	iconFile.close();

	byte *iconsData = nullptr;
	uint32 iconsSize = Common::decompressAccessDBE(compressedData, &iconsData);
	delete[] compressedData;

	if (!iconsData || iconsSize == 0)
		return false;

	uint16 cursorCount = READ_LE_UINT16(iconsData);
	Graphics::PixelFormat rgbaFormat(4, 8, 8, 8, 8, 0, 8, 16, 24);

	for (int i = 0; i < (cursorCount - 2) && i < 13; i++) {
		uint32 offset1 = READ_LE_UINT32(iconsData + 2 + i * 4);
		uint32 offset2 = READ_LE_UINT32(iconsData + 6 + i * 4);
		uint32 size = offset2 - offset1;

		if (size <= 1)
			continue;

		byte *pIcon = iconsData + offset1;
		byte *pEnd = pIcon + size;

		// each cursor provides 7 custom colors
		byte localPalette[768];
		memcpy(localPalette, basePalette, 768);
		memcpy(localPalette + 3, pIcon, 21);
		pIcon += 21;

		Common::Array<Graphics::Surface *> frames;

		while (pIcon < pEnd) {
			uint16 w = READ_LE_UINT16(pIcon + 2);
			uint16 h = READ_LE_UINT16(pIcon + 4);
			uint32 dataSize = READ_LE_UINT32(pIcon + 9);

			Graphics::Surface *surf = new Graphics::Surface();
			surf->create(w, h, rgbaFormat);
			memset(surf->getPixels(), 0, w * h * 4); // transparent background

			byte *pSrc = pIcon + 16;
			for (int y = 0; y < h; y++) {
				uint16 offsetX = READ_LE_UINT16(pSrc);
				uint16 length = READ_LE_UINT16(pSrc + 2);

				for (int x = 0; x < length; x++) {
					byte colorIndex = pSrc[4 + x];
					if (colorIndex > 0 && (offsetX + x) < w) {
						byte r = (localPalette[colorIndex * 3 + 0] * 255) / 63;
						byte g = (localPalette[colorIndex * 3 + 1] * 255) / 63;
						byte b = (localPalette[colorIndex * 3 + 2] * 255) / 63;

						byte *pixel = (byte *)surf->getBasePtr(offsetX + x, y);
						pixel[0] = r;
						pixel[1] = g;
						pixel[2] = b;
						pixel[3] = 255;
					}
				}
				pSrc += 4 + length;
			}
			frames.push_back(surf);
			pIcon += dataSize;
		}

		_cursors[i].setIcons((CursorType)i, frames);

		for (uint f = 0; f < frames.size(); f++) {
			frames[f]->free();
			delete frames[f];
		}
	}

	delete[] iconsData;

	setActiveCursor(CursorType::kArrow);
	CursorMan.showMouse(true);
	return true;
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

	if (!loadCursors()) {
		warning("Murphy3d: Failed to load cursor graphics. Using OS default.");
	}

	_mainMenu = new MainMenu(_font);
	_mainMenu->init();

	UAKMMap gameMap;
	if (!gameMap.init()) {
		warning("Murphy3d: Failed to parse MAP.LZ!");
	}

	_currentLocation = new Location();
	if (!_currentLocation->load("TEXOFF.AP"))
		return Common::Error(Common::kNoGameDataFoundError, _s("Could not load Location"));

	_currentLocation->buildBuffers(_renderer);

	Math::Vector3d eyePos(0.0f, 0.0f, 0.0f);
	Math::Vector3d eyeAt(0.0f, 0.0f, 1.0f);
	Math::Vector3d upVec(0.0f, 1.0f, 0.0f);
	Math::Matrix4 viewMat = MathUtils::lookAtLH(eyePos, eyeAt, upVec);

	float fov = 3.141592654f / 4.0f / 0.95f;
	Math::Matrix4 projMat = MathUtils::perspectiveFovLH(fov, 640.0f / 480.0f, 0.1f, 1000.0f);

	_player = new Player();

	for (int i = 0; i < 64; i++) {
		MapData *md = gameMap.get(i);
		if (md && md->locationFileIndex == 48 && md->startupPositions.size() > 0) {
			StartupPosition sp = md->startupPositions[0];
			_player->spawn(-sp.x, sp.elevation + sp.initialEyeLevel, -sp.z, sp.angle, 0.0f);
			break;
		}
	}

	Common::Event e;

	Graphics::FrameLimiter limiter(g_system, 60);

	bool moveFwd = false, moveBack = false, moveLeft = false, moveRight = false;
	bool isRunning = false;

	showMainMenu();

	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_QUIT || e.type == Common::EVENT_RETURN_TO_LAUNCHER) {
				return Common::kNoError;
			}

			if (_state == kStateMainMenu) {
				if (e.type == Common::EVENT_MOUSEMOVE) {
					_mainMenu->handleMouseMove(e.mouse.x, e.mouse.y);
				} else if (e.type == Common::EVENT_LBUTTONDOWN) {
					_mainMenu->handleMouseDown(e.mouse.x, e.mouse.y);
				} else if (e.type == Common::EVENT_LBUTTONUP) {
					_mainMenu->handleMouseUp(e.mouse.x, e.mouse.y);
				}
				continue;
			} else if (_state == kStateInGame) {
				if (e.type == Common::EVENT_KEYDOWN && e.kbd.keycode == Common::KEYCODE_ESCAPE) {
					showMainMenu();
					continue;
				}

				if (e.type == Common::EVENT_KEYDOWN || e.type == Common::EVENT_KEYUP) {
					bool isDown = (e.type == Common::EVENT_KEYDOWN);
					switch (e.kbd.keycode) {
					case Common::KEYCODE_w:
						moveFwd = isDown;
						break;
					case Common::KEYCODE_s:
						moveBack = isDown;
						break;
					case Common::KEYCODE_a:
						moveLeft = isDown;
						break;
					case Common::KEYCODE_d:
						moveRight = isDown;
						break;
					case Common::KEYCODE_LSHIFT:
					case Common::KEYCODE_RSHIFT:
						isRunning = isDown;
						break;
					default:
						break;
					}

					_player->setMovement(moveFwd, moveBack, moveLeft, moveRight);
					_player->setSpeed(isRunning);
				} else if (e.type == Common::EVENT_MOUSEMOVE) {
					float deltaYaw = e.relMouse.x * -0.002f;
					float deltaPitch = e.relMouse.y * -0.002f;
					_player->addRotation(deltaYaw, deltaPitch);
				}
			}
		}

		_renderer->clear(0.1f, 0.1f, 0.3f);

		if (_state == kStateMainMenu) {
			_mainMenu->render(_renderer);
		} else if (_state == kStateInGame) {
			_player->update();
			Math::Matrix4 worldMat = _player->getWorldMatrix();

			_renderer->updateMatrices(worldMat, viewMat, projMat);
			_currentLocation->render(_renderer);
		}

		int cursorIdx = (int)_activeCursor;
		if (_activeCursor <= CursorType::kSpecial && _cursors[cursorIdx].hasIcons())
			_cursors[cursorIdx].render();

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
