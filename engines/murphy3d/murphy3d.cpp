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

#include "murphy3d/renderer.h"
#include "murphy3d/location.h"
#include "murphy3d/uakm_map.h"
#include "murphy3d/math_utils.h"

namespace Murphy3d {

Murphy3dEngine *g_engine;

Murphy3dEngine::Murphy3dEngine(OSystem *syst, const ADGameDescription *gameDesc) : Engine(syst),
	_gameDescription(gameDesc), _randomSource("Murphy3d") {
	g_engine = this;
}

Murphy3dEngine::~Murphy3dEngine() {
	delete _screen;
	delete _renderer;
}

uint32 Murphy3dEngine::getFeatures() const {
	return _gameDescription->flags;
}

Common::String Murphy3dEngine::getGameId() const {
	return _gameDescription->gameId;
}

Common::Error Murphy3dEngine::run() {
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
	}

	UAKMMap gameMap;
	if (!gameMap.init()) {
		warning("Murphy3d: Failed to parse MAP.LZ!");
	}

	Location texOffice;
	if (texOffice.load("TEXOFF.AP")) {
		texOffice.buildBuffers(_renderer);
	}

	Math::Vector3d eyePos(0.0f, 0.0f, 0.0f);
	Math::Vector3d eyeAt(0.0f, 0.0f, 1.0f);
	Math::Vector3d upVec(0.0f, 1.0f, 0.0f);
	Math::Matrix4 viewMat = MathUtils::lookAtLH(eyePos, eyeAt, upVec);

	float fov = 3.141592654f / 4.0f / 0.95f;
	Math::Matrix4 projMat = MathUtils::perspectiveFovLH(fov, 640.0f / 480.0f, 0.1f, 1000.0f);

	float x = 0.0f, y = 0.0f, z = 0.0f;
	float yaw = 0.0f, pitch = 0.0f;

	for (int i = 0; i < 64; i++) {
		MapData *md = gameMap.get(i);
		if (md && md->locationFileIndex == 48 && md->startupPositions.size() > 0) {
			StartupPosition sp = md->startupPositions[0];

			x = sp.x;
			y = sp.y + sp.initialEyeLevel;
			z = sp.z;
			yaw = sp.angle;
			pitch = 0.0f;
			break;
		}
	}

	Common::Event e;

	Graphics::FrameLimiter limiter(g_system, 60);
	while (!shouldQuit()) {
		while (g_system->getEventManager()->pollEvent(e)) {
			if (e.type == Common::EVENT_QUIT || e.type == Common::EVENT_RETURN_TO_LAUNCHER) {
				return Common::kNoError;
			}
		}

		Math::Matrix4 tm = MathUtils::translation(x, y, z);
		Math::Matrix4 rm2 = MathUtils::rotationY(yaw);
		Math::Matrix4 rm1 = MathUtils::rotationX(pitch);

		Math::Matrix4 worldMat = tm * rm2 * rm1;

		_renderer->updateMatrices(worldMat, viewMat, projMat);

		_renderer->clear(0.1f, 0.1f, 0.3f);
		texOffice.render(_renderer);

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
