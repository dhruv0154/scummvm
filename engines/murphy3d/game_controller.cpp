#include "murphy3d/game_controller.h"

namespace Murphy3d {

GameBase *GameController::_game = nullptr;

Common::HashMap<int, Common::String> GameController::_fileMap;
Common::HashMap<int, Common::String> GameController::_itemMap;
Common::HashMap<int, Common::String> GameController::_askAboutMap;
Common::HashMap<int, Common::String> GameController::_lSituations;
Common::HashMap<int, Common::String> GameController::_dSituations;

bool GameController::_itemsChanged = false;
bool GameController::_askAboutChanged = false;
bool GameController::_buyChanged = false;
bool GameController::_canCancelTravel = false;

void GameController::init() {
	_fileMap.clear();
	_itemMap.clear();
	_askAboutMap.clear();
	_lSituations.clear();
	_dSituations.clear();

	_itemsChanged = false;
	_askAboutChanged = false;
	_buyChanged = false;
	_canCancelTravel = false;
}

bool GameController::startGame(GameBase *pGame) {
	_game = pGame;
	if (_game != nullptr) {
		_game->start();
		return true;
	}
	return false;
}

void GameController::setFileName(int index, const Common::String &name) {
	_fileMap[index] = name;
}

Common::String GameController::getFileName(int index) {
	return _fileMap.contains(index) ? _fileMap[index] : "";
}

void GameController::setItemName(int index, const Common::String &name) {
	_itemMap[index] = name;
}

Common::String GameController::getItemName(int index) {
	return _itemMap.contains(index) ? _itemMap[index] : "";
}

void GameController::setAskAboutName(int index, const Common::String &name) {
	_askAboutMap[index] = name;
}

Common::String GameController::getAskAboutName(int index) {
	return _askAboutMap.contains(index) ? _askAboutMap[index] : "";
}

void GameController::setSituationDescriptionL(int index, const Common::String &desc) {
	_lSituations[index] = desc;
}

Common::String GameController::getSituationDescriptionL(int index) {
	return _lSituations.contains(index) ? _lSituations[index] : "";
}

void GameController::setSituationDescriptionD(int index, const Common::String &desc) {
	_dSituations[index] = desc;
}

Common::String GameController::getSituationDescriptionD(int index) {
	return _dSituations.contains(index) ? _dSituations[index] : "";
}

void GameController::transformItem(int fromId, int toId) {
	if (_game != nullptr) {
		_game->setItemState(fromId, 2);
		_game->setItemState(toId, 1);
		if (_game->getCurrentItemId() == fromId) {
			_game->setCurrentItemId(toId);
		}
		_itemsChanged = true;
	}
}

} // End of namespace Murphy3d
