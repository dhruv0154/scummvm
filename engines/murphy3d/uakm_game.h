#ifndef MURPHY3D_UAKM_GAME_H
#define MURPHY3D_UAKM_GAME_H

#include "murphy3d/game_base.h"

namespace Murphy3d {

#define UAKM_SAVE_UNKNOWN1 0
#define UAKM_SAVE_PLAYER 2
#define UAKM_SAVE_LOCATION 0x1a
#define UAKM_SAVE_GAME_DAY 0x38
#define UAKM_SAVE_YEAR 0x39
#define UAKM_SAVE_MONTH 0x3b
#define UAKM_SAVE_DAY 0x3c
#define UAKM_SAVE_HOUR 0x3d
#define UAKM_SAVE_MINUTE 0x3e
#define UAKM_SAVE_SECOND 0x3f
#define UAKM_SAVE_COMMENT 0x40
#define UAKM_SAVE_PADDING1 0xd0
#define UAKM_SAVE_PADDING2 0x100
#define UAKM_SAVE_CRIMELINK_SELECTIONS 0x12c
#define UAKM_SAVE_MAP_ENTRY 0x14a
#define UAKM_SAVE_UNKNOWN2 0x14c
#define UAKM_SAVE_DMAP_ENTRY 0x150
#define UAKM_SAVE_UNKNOWN3 0x152
#define UAKM_SAVE_TIMERS_INITIAL 0x15a
#define UAKM_SAVE_TIMERS_CURRENT 0x19a
#define UAKM_SAVE_TIMERS 0x1da
#define UAKM_SAVE_PARAMETERS 0x1fa
#define UAKM_SAVE_ITEM_STATES 0x2fa
#define UAKM_SAVE_HINT_STATES 0x388
#define UAKM_SAVE_COLONELS_SAFE_CODE 0x431
#define UAKM_SAVE_CODED_MESSAGE 0x435
#define UAKM_SAVE_STASIS_SLIDER1 0x4c5
#define UAKM_SAVE_STASIS_SLIDER2 0x4c7
#define UAKM_SAVE_SCRIPT_ID 0x4c9
#define UAKM_SAVE_ITEMS_EXAMINED_FLAGS 0x4cb
#define UAKM_SAVE_STASIS_SETTINGS 0x4dd
#define UAKM_SAVE_STASIS_STAGES 0x4df
#define UAKM_SAVE_ASK_ABOUT_COUNT 0x4e1
#define UAKM_SAVE_CURRENT_ASK 0x4e3
#define UAKM_SAVE_UNKNOWN5 0x4e4
#define UAKM_SAVE_ASK_ABOUTS 0x4e6
#define UAKM_SAVE_ITEM_COUNT 0x518
#define UAKM_SAVE_CURRENT_ITEM 0x51a
#define UAKM_SAVE_UNKNOWN6 0x51b
#define UAKM_SAVE_INVENTORY 0x51d
#define UAKM_SAVE_HINT_CATEGORY_STATES 0x5ae
#define UAKM_SAVE_ASK_ABOUT_STATES 0x604
#define UAKM_SAVE_CHAPTER 0x636
#define UAKM_SAVE_AF_ACTIONS 0x638
#define UAKM_SAVE_DMAP_FLAG 0x69c
#define UAKM_SAVE_PUZZLE_DATA 0x69d
#define UAKM_SAVE_TRAVEL 0x89b
#define UAKM_SAVE_TRAVEL_BACKUP 0x8c2
#define UAKM_SAVE_UNKNOWN7 0x8e9
#define UAKM_SAVE_SCORE 0x8ea

#define UAKM_SAVE_SIZE 0x8ec
#define UAKM_MAX_ITEM_COUNT 142
#define TIMER_SCALE 1000

class UAKMGame : public GameBase {
protected:
	bool init() override;

public:
	UAKMGame();
	~UAKMGame() override;

	void render() override;

	void mouseMove(Common::Point pt) override;
	void mouseDown(Common::Point pt, int btn) override;
	void keyDown(uint32 key, uint32 lParam) override;
	void keyUp(uint32 key, uint32 lParam) override;

	ScriptBase *getScriptEngine() override { return nullptr; /* TODO Phase 3: new UAKMScript() */ }
	ScriptState *getScriptState() override { return nullptr; /* TODO Phase 3: new UAKMScriptState() */ }

	void loadGame(const char *fileName) override;
	void saveGame(const char *fileName) override;
	void newGame() override;

	int getSaveCommentOffset() override { return UAKM_SAVE_COMMENT; }
	int getSaveCommentLength() override { return UAKM_SAVE_PADDING1 - UAKM_SAVE_COMMENT; }

	uint8 getParameter(int index) override;
	void setParameter(int index, uint8 value) override;

	uint8 getData(int offset) override;
	void setData(int offset, uint8 value) override;
	void setData(int offset, const char *text) override;
	int getWord(int offset, bool signExtend = false) override;
	void setWord(int offset, int value) override;
	void copy(uint8 *source, int destinationOffset, int length) override {}

	uint8 getAskAboutState(int index) override;
	void setAskAboutState(int index, uint8 state) override;
	int getAskAboutCount() override;
	int getAskAboutId(int index) override;

	int getScore() override;
	void addScore(int value) override;

	int getItemCount() override;
	int getItemId(int index) override;
	int getItemState(int item) override;
	void setItemState(int item, int state) override;
	int getCurrentItemId() override;
	void setCurrentItemId(int item) override;
	int selectNextItem() override;
	int selectPreviousItem() override;

	int getItemState(int base, int item) override { return 0; }
	void setItemState(int base, int item, int state) override {}

	uint8 getHintState(int index) override;
	void setHintState(int index, uint8 state, int score) override;
	uint8 getHintCategoryState(int index) override;
	void setHintCategoryState(int index, uint8 state) override;

	void setTimer(int timer, int duration) override;
	int getTimerState(int timer) override;
	void resetTimers() override;
	void tick(int ticks) override;

	void setItemExamined(int itemId, int conditionalScore = 0) override;

	bool canCancelVideo() override { return (getParameter(247) == 0); }

	int getLocationInitializationScriptId() override { return 1000; }
	int getLocationEnvironmentScriptId() override { return 2000; }

	HintModule *getHintModule() override { return nullptr; /* TODO: new UAKMHintModule() */ }

	void setSelectedItem(int item) override {
		_selectedItem = item;
		setParameter(99, item);
	}

protected:
	int _lastDialoguePoint;
	int _frameTrigger;

	int indexOfItemId(int item);

	bool loadIcons() override;
};

} // End of namespace Murphy3d

#endif
