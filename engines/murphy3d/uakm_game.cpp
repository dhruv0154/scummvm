#include "murphy3d/uakm_game.h"
#include "common/algorithm.h"
#include "common/str.h"
#include "common/compression/access_lzw.h"
#include "murphy3d/archive.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/item.h"
#include "murphy3d/main_menu.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/uakm_map.h"

namespace Murphy3d {

static const int kHintStatePairs[] = {
	3, 21, 56, 61, 263, 289, 12, 11, 264, 290, 434, 440, 434, 446, 234, 186, 343, 342};

UAKMGame::UAKMGame() {
	_gameData = new uint8[UAKM_SAVE_SIZE];
	memset(_gameData, 0, UAKM_SAVE_SIZE);

	_lastDialoguePoint = -1;
	_frameTrigger = -1;
}

UAKMGame::~UAKMGame() {
	delete[] _gameData;
	_gameData = nullptr;
}

bool UAKMGame::init() {
	readGameXMLInfo("UAKM.xml");

	// pass nullptr for dmap for now
	UAKMMap *map = new UAKMMap();
	if (ModuleController::init(map, nullptr) && loadIcons()) {
		ModuleController::push(new MainMenu());
		return true;
	}

	return false;
}

void UAKMGame::render() {
	ModuleController::render();
}

void UAKMGame::mouseDown(Common::Point pt, int btn) {
	ModuleController::mouseDown(pt, btn);
}

void UAKMGame::mouseMove(Common::Point pt) {
	ModuleController::mouseMove(pt);
}

void UAKMGame::keyDown(uint32 key, uint32 lParam) {
	ModuleController::keyDown(key, lParam);
}

void UAKMGame::keyUp(uint32 key, uint32 lParam) {
	ModuleController::keyUp(key, lParam);
}

void UAKMGame::newGame() {
	memset(_gameData, 0, UAKM_SAVE_SIZE);
	memset(_gameData + 2, ' ', UAKM_SAVE_PADDING1 - 2);

	_gameData[UAKM_SAVE_UNKNOWN1 + 1] = 1;
	setData(UAKM_SAVE_PLAYER, "TEX");

	_gameData[UAKM_SAVE_GAME_DAY] = 1;

	setData(UAKM_SAVE_CODED_MESSAGE, "YE UANE CIAFWBHED RIPB AEEIWALHEAL  YWLU CUAXLWLR AL  LUE XPWLE WA LUE  GIODEA GALE UILEO AL LUE PXPAO LWHE.LUE EAXXYIBD LIDARWX XWOWCIA.       ");

	setAskAboutState(0, 1);
	setAskAboutState(1, 1);
	setAskAboutState(2, 1);
	setAskAboutState(3, 1);
	setAskAboutState(4, 1);
	setAskAboutState(5, 1);
	setAskAboutState(17, 1);

	setData(UAKM_SAVE_TRAVEL + 5, 1);

	_gameData[UAKM_SAVE_CURRENT_ASK] = 0xff;
	_gameData[UAKM_SAVE_CURRENT_ITEM] = 0xff;
	_gameData[UAKM_SAVE_CHAPTER] = 1;
	_gameData[UAKM_SAVE_PARAMETERS + 99] = 0xff;
	_gameData[UAKM_SAVE_PARAMETERS + 251] = 2;
	_gameData[UAKM_SAVE_PARAMETERS + 250] = 1;
	_gameData[UAKM_SAVE_HINT_CATEGORY_STATES + 1] = 1;

	MainMenu *mainMenu = static_cast<MainMenu *>(ModuleController::get(ModuleType::kMainMenu));
	if (mainMenu != nullptr) {
		mainMenu->enableResume(true);
		mainMenu->enableSave(true);
	}

	int startEntry = 0;
	if (ModuleController::_pMap != nullptr) {
		for (int i = 0; i < 64; i++) {
			MapData *md = ModuleController::_pMap->get(i);
			if (md && md->locationFileIndex == 48 && md->startupPositions.size() > 0) {
				startEntry = i;
				break;
			}
		}
	}

	loadFromMap(startEntry, 0);
}

void UAKMGame::loadGame(const char *fileName) {
	// TODO: Hook to ScummVM SaveFileManager
}

void UAKMGame::saveGame(const char *fileName) {
	// TODO: Hook to ScummVM SaveFileManager
}

uint8 UAKMGame::getParameter(int index) {
	return (index >= 0 && index < 256) ? _gameData[UAKM_SAVE_PARAMETERS + index] : 0;
}

void UAKMGame::setParameter(int index, uint8 value) {
	if (index >= 0 && index < 256) {
		_gameData[UAKM_SAVE_PARAMETERS + index] = value;
	}
}

int UAKMGame::getWord(int offset, bool signExtend) {
	int result = (offset >= 0 && offset < (UAKM_SAVE_SIZE - 1)) ? (_gameData[offset + 1] << 8) | _gameData[offset] : 0;
	if (signExtend && (result & 0x8000)) {
		result |= ~0xffff;
	}
	return result;
}

void UAKMGame::setWord(int offset, int value) {
	if (offset >= 0 && offset < (UAKM_SAVE_SIZE - 1)) {
		_gameData[offset] = value & 0xff;
		_gameData[offset + 1] = (value >> 8) & 0xff;
	}
}

uint8 UAKMGame::getAskAboutState(int index) {
	return (index >= 0 && index < 45) ? _gameData[UAKM_SAVE_ASK_ABOUT_STATES + index] : 0;
}

void UAKMGame::setAskAboutState(int index, uint8 state) {
	if (index >= 0 && index <= 45) {
		int count = getAskAboutCount();
		if (state == 0 || state == 2) {
			for (int i = 0; i < count && i < 50; i++) {
				if (_gameData[UAKM_SAVE_ASK_ABOUTS + i] == index) {
					for (; i < count && i < 49; i++) {
						_gameData[UAKM_SAVE_ASK_ABOUTS + i] = _gameData[UAKM_SAVE_ASK_ABOUTS + i + 1];
					}
					_gameData[UAKM_SAVE_ASK_ABOUTS + count] = 0xff;
					_gameData[UAKM_SAVE_ASK_ABOUT_COUNT]--;
					break;
				}
			}
		} else if (state == 1) {
			for (int i = 0; i < count; i++) {
				if (_gameData[UAKM_SAVE_ASK_ABOUTS + i] == index) {
					return;
				}
			}
			_gameData[UAKM_SAVE_ASK_ABOUTS + count] = index;
			_gameData[UAKM_SAVE_ASK_ABOUT_COUNT]++;
		}
		_gameData[UAKM_SAVE_ASK_ABOUT_STATES + index] = state;
	}
}

int UAKMGame::getAskAboutCount() {
	return _gameData[UAKM_SAVE_ASK_ABOUT_COUNT];
}

int UAKMGame::getAskAboutId(int index) {
	return _gameData[UAKM_SAVE_ASK_ABOUTS + index];
}

int UAKMGame::getScore() {
	return getWord(UAKM_SAVE_SCORE, true);
}

void UAKMGame::addScore(int value) {
	setWord(UAKM_SAVE_SCORE, getWord(UAKM_SAVE_SCORE) + value);
}

int UAKMGame::getItemCount() {
	return _gameData[UAKM_SAVE_ITEM_COUNT];
}

int UAKMGame::getItemId(int index) {
	return (index >= 0 && index < UAKM_MAX_ITEM_COUNT) ? _gameData[UAKM_SAVE_INVENTORY + index] : 0;
}

int UAKMGame::getItemState(int item) {
	return _gameData[UAKM_SAVE_ITEM_STATES + item];
}

void UAKMGame::setItemState(int item, int state) {
	if (item >= 0 && item < UAKM_MAX_ITEM_COUNT) {
		_gameData[UAKM_SAVE_ITEM_STATES + item] = state;
		int count = _gameData[UAKM_SAVE_ITEM_COUNT];

		if (state == 0 || state == 2) {
			for (int i = 0; i < count && i < UAKM_MAX_ITEM_COUNT; i++) {
				if (_gameData[UAKM_SAVE_INVENTORY + i] == item) {
					for (int j = i + 1; j < count && j < UAKM_MAX_ITEM_COUNT; j++) {
						_gameData[UAKM_SAVE_INVENTORY + j - 1] = _gameData[UAKM_SAVE_INVENTORY + j];
					}
					_gameData[UAKM_SAVE_ITEM_COUNT]--;
					break;
				}
			}
			if (_gameData[UAKM_SAVE_CURRENT_ITEM] == item) {
				_gameData[UAKM_SAVE_CURRENT_ITEM] = 0xff;
			}
		} else if (state == 1) {
			bool alreadyInInventory = false;
			for (int i = 0; i < count; i++) {
				if (_gameData[UAKM_SAVE_INVENTORY + i] == item) {
					alreadyInInventory = true;
					break;
				}
			}
			if (!alreadyInInventory) {
				_gameData[UAKM_SAVE_INVENTORY + count] = item;
				_gameData[UAKM_SAVE_ITEM_COUNT]++;
				_gameData[UAKM_SAVE_CURRENT_ITEM] = item;
			}
		}
	}
}

int UAKMGame::getCurrentItemId() {
	return (_gameData[UAKM_SAVE_CURRENT_ITEM] == 0xff) ? -1 : _gameData[UAKM_SAVE_CURRENT_ITEM];
}

void UAKMGame::setCurrentItemId(int item) {
	_gameData[UAKM_SAVE_CURRENT_ITEM] = item;
}

int UAKMGame::selectNextItem() {
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	int currentItem = _gameData[UAKM_SAVE_CURRENT_ITEM];
	int newIndex = indexOfItemId(currentItem) + 1;
	if (newIndex >= count) {
		newIndex = -1;
	}
	_gameData[UAKM_SAVE_CURRENT_ITEM] = newIndex < 0 ? 0xff : _gameData[UAKM_SAVE_INVENTORY + newIndex];
	return _gameData[UAKM_SAVE_CURRENT_ITEM];
}

int UAKMGame::selectPreviousItem() {
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	int currentItem = _gameData[UAKM_SAVE_CURRENT_ITEM];
	int newIndex = indexOfItemId(currentItem) - 1;
	if (newIndex < -1) {
		newIndex = count - 1;
	}
	_gameData[UAKM_SAVE_CURRENT_ITEM] = newIndex < 0 ? 0xff : _gameData[UAKM_SAVE_INVENTORY + newIndex];
	return _gameData[UAKM_SAVE_CURRENT_ITEM];
}

int UAKMGame::indexOfItemId(int item) {
	int count = _gameData[UAKM_SAVE_ITEM_COUNT];
	for (int i = 0; i < count; i++) {
		if (_gameData[UAKM_SAVE_INVENTORY + i] == item) {
			return i;
		}
	}
	return -1;
}

uint8 UAKMGame::getHintState(int index) {
	int val = 0;
	if (index >= 0 && index < 1352) {
		int byte = index / 4;
		int shift = (index & 3) * 2;
		val = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
	}
	return val;
}

void UAKMGame::setHintState(int index, uint8 state, int score) {
	if (index >= 0 && index < 1352) {
		int byte = index / 4;
		int shift = (index & 3) * 2;
		int oldState = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
		_gameData[UAKM_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
		if (oldState == 0 && score > 0) {
			addScore(score);
		}

		for (int i = 0; i < 9; i++) {
			if (kHintStatePairs[i * 2] == index) {
				int pair = kHintStatePairs[i * 2 + 1];
				byte = pair / 4;
				shift = (pair & 3) * 2;
				oldState = (_gameData[UAKM_SAVE_HINT_STATES + byte] >> shift) & 3;
				_gameData[UAKM_SAVE_HINT_STATES + byte] |= (state & 3) << shift;
				if (oldState == 0) {
					addScore(1);
				}
			}
		}
	}
}

uint8 UAKMGame::getHintCategoryState(int index) {
	return (index >= 0 && index < 86) ? _gameData[UAKM_SAVE_HINT_CATEGORY_STATES + index] : 0;
}

void UAKMGame::setHintCategoryState(int index, uint8 state) {
	if (index >= 0 && index < 86) {
		_gameData[UAKM_SAVE_HINT_CATEGORY_STATES + index] = state;
	}
}

void UAKMGame::setTimer(int timer, int duration) {
	if (timer >= 0 && timer < 32) {
		_gameData[UAKM_SAVE_TIMERS + timer] = 1;
		setWord(UAKM_SAVE_TIMERS_INITIAL + timer * 2, duration);
		setWord(UAKM_SAVE_TIMERS_CURRENT + timer * 2, duration);
		_timers[timer] = duration * TIMER_SCALE;
	}
}

int UAKMGame::getTimerState(int timer) {
	return (timer >= 0 && timer < 32) ? _gameData[UAKM_SAVE_TIMERS + timer] : 0;
}

void UAKMGame::resetTimers() {
	memset(_timers, 0, sizeof(_timers));
	memset(_gameData + UAKM_SAVE_TIMERS_INITIAL, 0, 32 * 5);
}

void UAKMGame::tick(int ticks) {
	for (int i = 0; i < 32; i++) {
		if (_gameData[UAKM_SAVE_TIMERS + i] > 0) {
			_timers[i] = MAX(0, _timers[i] - ticks);
			if (_timers[i] == 0) {
				_gameData[UAKM_SAVE_TIMERS + i] = 0;
				setWord(UAKM_SAVE_TIMERS_CURRENT + i * 2, getWord(UAKM_SAVE_TIMERS_INITIAL + i * 2));
			}
		}
	}
}

uint8 UAKMGame::getData(int offset) {
	return (offset >= 0 && offset < UAKM_SAVE_SIZE) ? _gameData[offset] : 0;
}

void UAKMGame::setData(int offset, uint8 value) {
	if (offset >= 0 && offset < UAKM_SAVE_SIZE) {
		_gameData[offset] = value;
	}
}

void UAKMGame::setData(int offset, const char *text) {
	if (offset >= 0) {
		while (offset < UAKM_SAVE_SIZE && *text) {
			_gameData[offset++] = *(text++);
		}
	}
}

void UAKMGame::setItemExamined(int itemId, int conditionalScore) {
	if (itemId >= 0 && itemId < UAKM_MAX_ITEM_COUNT) {
		int byte = itemId / 8;
		int shift = itemId & 7;

		int oldState = (_gameData[UAKM_SAVE_ITEMS_EXAMINED_FLAGS + byte] & (1 << shift));
		_gameData[UAKM_SAVE_ITEMS_EXAMINED_FLAGS + byte] |= (1 << shift);

		if (oldState == 0 && conditionalScore > 0) {
			addScore(conditionalScore);
		}
	}
}

bool UAKMGame::loadIcons() {
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

	return GameBase::loadIcons(iconsData, iconsSize);
}

} // End of namespace Murphy3d
