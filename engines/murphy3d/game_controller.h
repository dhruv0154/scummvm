#ifndef MURPHY3D_GAME_CONTROLLER_H
#define MURPHY3D_GAME_CONTROLLER_H

#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/game_base.h"

namespace Murphy3d {

class GameController {
public:
	static void init();
	static void destroy();
	static bool startGame(GameBase *pGame);

	static int getScore() { return _game ? _game->getScore() : 0; }
	static void addScore(int val) {
		if (_game)
			_game->addScore(val);
	}

	static uint8 getParameter(int index) { return _game ? _game->getParameter(index) : 0; }
	static void setParameter(int index, uint8 value) {
		if (_game)
			_game->setParameter(index, value);
	}

	static void setHintState(int index, uint8 state, int score) {
		if (_game)
			_game->setHintState(index, state, score);
	}
	static uint8 getHintState(int index) { return _game ? _game->getHintState(index) : 0; }
	static void setHintCategoryState(int index, uint8 state) {
		if (_game)
			_game->setHintCategoryState(index, state);
	}
	static uint8 getHintCategoryState(int index) { return _game ? _game->getHintCategoryState(index) : 0; }

	static void setTimer(int timer, int duration) {
		if (_game)
			_game->setTimer(timer, duration);
	}
	static int getTimerState(int timer) { return _game ? _game->getTimerState(timer) : 0; }
	static void resetTimers() {
		if (_game)
			_game->resetTimers();
	}
	static void tick(int ticks) {
		if (_game)
			_game->tick(ticks);
	}

	static ScriptBase *getScriptEngine() { return _game ? _game->getScriptEngine() : nullptr; }
	static ScriptState *getScriptState() { return _game ? _game->getScriptState() : nullptr; }

	static void setFileName(int index, const Common::String &name);
	static Common::String getFileName(int index);
	static void setItemName(int index, const Common::String &name);
	static Common::String getItemName(int index);
	static void setAskAboutName(int index, const Common::String &name);
	static Common::String getAskAboutName(int index);
	static void setSituationDescriptionL(int index, const Common::String &desc);
	static Common::String getSituationDescriptionL(int index);
	static void setSituationDescriptionD(int index, const Common::String &desc);
	static Common::String getSituationDescriptionD(int index);

	static int getSaveCommentOffset() { return _game ? _game->getSaveCommentOffset() : 0; }
	static int getSaveCommentLength() { return _game ? _game->getSaveCommentLength() : 0; }

	static uint8 getData(int offset) { return _game ? _game->getData(offset) : 0; }
	static void setData(int offset, uint8 value) {
		if (_game)
			_game->setData(offset, value);
	}
	static void setData(int offset, const char *text) {
		if (_game)
			_game->setData(offset, text);
	}
	static uint8 *getDataPointer() { return _game ? _game->getDataPointer() : nullptr; }
	static int getWord(int offset, bool signExtend = false) { return _game ? _game->getWord(offset, signExtend) : 0; }
	static void setWord(int offset, int value) {
		if (_game)
			_game->setWord(offset, value);
	}
	static void copy(uint8 *source, int destinationOffset, int length) {
		if (_game)
			_game->copy(source, destinationOffset, length);
	}

	static void transformItem(int fromId, int toId);
	static void setItemState(int item, int state) {
		if (_game)
			_game->setItemState(item, state);
	}
	static int getItemState(int item) { return _game ? _game->getItemState(item) : 0; }
	static void setItemState(int base, int item, int state) {
		if (_game)
			_game->setItemState(base, item, state);
	}
	static int getItemState(int base, int item) { return _game ? _game->getItemState(base, item) : 0; }
	static int getItemCount() { return _game ? _game->getItemCount() : 0; }
	static int getItemId(int index) { return _game ? _game->getItemId(index) : -1; }
	static int getCurrentItemId() { return _game ? _game->getCurrentItemId() : -1; }
	static void setCurrentItemId(int item) {
		if (_game)
			_game->setCurrentItemId(item);
	}
	static int selectNextItem() { return _game ? _game->selectNextItem() : -1; }
	static int selectPreviousItem() { return _game ? _game->selectPreviousItem() : -1; }
	static void setItemExamined(int itemId, int conditionalScore) {
		if (_game)
			_game->setItemExamined(itemId, conditionalScore);
	}

	static void setAskAboutState(int index, uint8 state) {
		if (_game)
			_game->setAskAboutState(index, state);
	}
	static uint8 getAskAboutState(int index) { return _game ? _game->getAskAboutState(index) : 0; }
	static int getAskAboutCount() { return _game ? _game->getAskAboutCount() : 0; }
	static int getAskAboutId(int index) { return _game ? _game->getAskAboutId(index) : -1; }

	static void loadFromDMap(int entry) {
		if (_game)
			_game->loadFromDMap(entry);
	}
	static void loadFromMap(int entry, int startupPosition) {
		if (_game)
			_game->loadFromMap(entry, startupPosition);
	}
	static void newGame() {
		if (_game)
			_game->newGame();
	}
	static void loadGame(const char *fileName) {
		if (_game)
			_game->loadGame(fileName);
	}
	static void saveGame(const char *fileName) {
		if (_game)
			_game->saveGame(fileName);
	}

	static bool canCancelVideo() { return _game ? _game->canCancelVideo() : true; }
	static void canCancelVideo(bool allow) {
		if (_game)
			_game->canCancelVideo(allow);
	}

	static int getLocationInitializationScriptId() { return _game ? _game->getLocationInitializationScriptId() : 0; }
	static int getLocationEnvironmentScriptId() { return _game ? _game->getLocationEnvironmentScriptId() : 0; }

	static int getHintCategoryCount() { return _game ? _game->getHintCategoryCount() : 0; }
	static HintCategory *getHintCategory(int index) { return _game ? _game->getHintCategory(index) : nullptr; }

	static HintModule *getHintModule() { return nullptr; }


	static GameBase *_game;

	static Common::HashMap<int, Common::String> _fileMap;
	static Common::HashMap<int, Common::String> _itemMap;
	static Common::HashMap<int, Common::String> _askAboutMap;
	static Common::HashMap<int, Common::String> _lSituations;
	static Common::HashMap<int, Common::String> _dSituations;

	static bool _itemsChanged;
	static bool _askAboutChanged;
	static bool _buyChanged;
	static bool _canCancelTravel;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_GAME_CONTROLLER_H
