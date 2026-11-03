#ifndef MURPHY3D_H
#define MURPHY3D_H

#include "common/error.h"
#include "common/fs.h"
#include "common/hash-str.h"
#include "common/random.h"
#include "common/scummsys.h"
#include "common/serializer.h"
#include "common/system.h"
#include "common/util.h"
#include "engines/engine.h"
#include "engines/savestate.h"
#include "graphics/screen.h"
#include "murphy3d/animated_cursor.h"
#include "murphy3d/detection.h"
#include "murphy3d/font.h"
#include "murphy3d/game_base.h"
#include "murphy3d/renderer.h"

namespace Murphy3d {

struct Murphy3dGameDescription;

class Murphy3dEngine : public Engine {
private:
	const ADGameDescription *_gameDescription;
	Common::RandomSource _randomSource;
	GameBase *_game = nullptr;

protected:
	Common::Error run() override;

public:
	Graphics::Screen *_screen = nullptr;
	Murphy3d::Renderer *_renderer = nullptr;
	Font *_font = nullptr;

	void startNewGame();
	void resumeGame();
	void showMainMenu();
	void quitGame() {
		Engine::quitGame();
	}

	void setActiveCursor(CursorType type);
	AnimatedCursor *getCursor(CursorType type);

public:
	Murphy3dEngine(OSystem *syst, const ADGameDescription *gameDesc);
	~Murphy3dEngine() override;
	void initializePath(const Common::FSNode &gamePath) override;

	uint32 getFeatures() const;
	Common::String getGameId() const;

	uint32 getRandomNumber(uint maxNum) {
		return _randomSource.getRandomNumber(maxNum);
	}

	bool hasFeature(EngineFeature f) const override {
		return (f == kSupportsLoadingDuringRuntime) ||
			   (f == kSupportsSavingDuringRuntime) ||
			   (f == kSupportsReturnToLauncher);
	}

	bool canLoadGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}
	bool canSaveGameStateCurrently(Common::U32String *msg = nullptr) override {
		return true;
	}

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

extern Murphy3dEngine *g_engine;
#define SHOULD_QUIT ::Murphy3d::g_engine->shouldQuit()

} // End of namespace Murphy3d

#endif // MURPHY3D_H
