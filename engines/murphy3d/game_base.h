#ifndef MURPHY3D_GAME_BASE_H
#define MURPHY3D_GAME_BASE_H

#include "common/hashmap.h"
#include "common/rect.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/animated_cursor.h"
#include "murphy3d/hint.h"
#include "murphy3d/location.h"
#include "murphy3d/map.h"
#include "murphy3d/script_base.h"
#include "murphy3d/script_state.h"

namespace Murphy3d {

class HintModule;
class LocationModule;

class GameBase {
public:
	GameBase();
	virtual ~GameBase();

	virtual void render() = 0;
	virtual void mouseMove(Common::Point pt) = 0;
	virtual void mouseDown(Common::Point pt, int btn) = 0;
	virtual void keyDown(uint32 key, uint32 lParam) = 0;
	virtual void keyUp(uint32 key, uint32 lParam) = 0;

	virtual void loadFromDMap(int entry);
	virtual void loadFromMap(int entry, int startupPosition);

	virtual void start();

	virtual ScriptBase *getScriptEngine() = 0;
	virtual ScriptState *getScriptState() = 0;

	virtual void loadGame(const char *fileName) = 0;
	virtual void saveGame(const char *fileName) = 0;
	virtual void newGame() = 0;

	virtual int getSaveCommentOffset() = 0;
	virtual int getSaveCommentLength() = 0;

	virtual uint8 getParameter(int index) = 0;
	virtual void setParameter(int index, uint8 value) = 0;

	virtual uint8 getData(int offset) = 0;
	virtual void setData(int offset, uint8 value) = 0;
	virtual void setData(int offset, const char *text) = 0;
	virtual uint8 *getDataPointer() { return _gameData; }
	virtual int getWord(int offset, bool signExtend = false) = 0;
	virtual void setWord(int offset, int value) = 0;
	virtual void copy(uint8 *source, int destinationOffset, int length) = 0;

	virtual uint8 getAskAboutState(int index) = 0;
	virtual void setAskAboutState(int index, uint8 value) = 0;
	virtual int getAskAboutCount() = 0;
	virtual int getAskAboutId(int index) = 0;

	virtual int getScore() = 0;
	virtual void addScore(int value) = 0;

	virtual int getItemCount() = 0;
	virtual int getItemId(int index) = 0;
	virtual int getItemState(int item) = 0;
	virtual void setItemState(int item, int state) = 0;
	virtual int getCurrentItemId() = 0;
	virtual void setCurrentItemId(int item) = 0;
	virtual int selectNextItem() = 0;
	virtual int selectPreviousItem() = 0;

	virtual int getItemState(int base, int item) = 0;
	virtual void setItemState(int base, int item, int state) = 0;

	virtual uint8 getHintState(int index) = 0;
	virtual void setHintState(int index, uint8 state, int score) = 0;
	virtual uint8 getHintCategoryState(int index) = 0;
	virtual void setHintCategoryState(int index, uint8 state) = 0;

	virtual void setTimer(int timer, int duration) = 0;
	virtual int getTimerState(int timer) = 0;
	virtual void resetTimers() = 0;
	virtual void tick(int ticks) = 0;

	virtual void setItemExamined(int itemId, int conditionalScore) = 0;

	bool _allowCancelVideo;
	virtual bool canCancelVideo() = 0;
	virtual void canCancelVideo(bool allow) { _allowCancelVideo = allow; }

	virtual int getLocationInitializationScriptId() = 0;
	virtual int getLocationEnvironmentScriptId() = 0;

	int getHintCategoryCount() { return _hintCategoryCount; }
	HintCategory *getHintCategory(int index);

	virtual HintModule *getHintModule() = 0;

	virtual void setSelectedItem(int item) { _selectedItem = item; }
	virtual int getSelectedItem(int item) { return _selectedItem; }

protected:
	virtual bool init() = 0;

	virtual bool loadIcons() { return false; }
	bool loadIcons(const byte *bdData, uint32 bdLength);

	uint8 *_gameData;

	int _timers[32];

	void readGameXMLInfo(const Common::String &xmlFilename);

	virtual LocationModule *getLocationModule(int entry, int startupPosition);

	Common::HashMap<int, HintCategory *> _hintCategories;
	int _hintCategoryCount;

	int _selectedItem;
};

} // End of namespace Murphy3d

#endif // MURPHY3D_GAME_BASE_H
