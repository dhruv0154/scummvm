#include "murphy3d/location_module.h"
#include "common/events.h"
#include "common/keyboard.h"
#include "common/system.h"
#include "murphy3d/animated_cursor.h"
#include "murphy3d/archive.h"
#include "murphy3d/caption.h"
#include "murphy3d/game_controller.h"
#include "murphy3d/item.h"
#include "murphy3d/main_menu.h"
#include "murphy3d/module_controller.h"
#include "murphy3d/murphy3d.h"
#include "murphy3d/animation_controller.h"
#include "graphics/cursorman.h"

namespace Murphy3d {

float LocationModule::_movement_forward = 0.0f;
float LocationModule::_movement_backward = 0.0f;
float LocationModule::_movement_left = 0.0f;
float LocationModule::_movement_right = 0.0f;
float LocationModule::_movement_x = 0.0f;
float LocationModule::_movement_y = 0.0f;
float LocationModule::_movement_z = 0.0f;
float LocationModule::_smooth_movement_x = 0.0f;
float LocationModule::_smooth_movement_z = 0.0f;
float LocationModule::_speed = 0.0f;

ActionType LocationModule::_currentAction = ActionType::kNone;

LocationModule::LocationModule(int locationId, int startupPosition)
	: ModuleBase(ModuleType::kLocation) {
	_locationId = locationId;
	_startupPosition = startupPosition;

	_currentObjectIndex = -1;
	_currentActions = ActionType::kNone;
	_currentAction = ActionType::kNone;
	_currentActionMousePointerIndex = static_cast<int>(CursorType::kCrosshair);

	_movement_forward = 0.0f;
	_movement_backward = 0.0f;
	_movement_left = 0.0f;
	_movement_right = 0.0f;
	_movement_x = 0.0f;
	_movement_y = 0.0f;
	_movement_z = 0.0f;
	_smooth_movement_x = 0.0f;
	_smooth_movement_z = 0.0f;
	_speed = MOVEMENT_WALK_SPEED;
	_ignoreNextMouseMove = true;

	_oldPoint = Common::Point(g_system->getWidth() / 2, g_system->getHeight() / 2);

	_scriptEngine = GameController::getScriptEngine();
	if (_scriptEngine != nullptr) {
		_scriptEngine->_pLoc = &_location;
	}

	_actionScriptState = GameController::getScriptState();
	_environmentScriptState = GameController::getScriptState();
	_queryActionScriptState = GameController::getScriptState();
	_initScriptState = GameController::getScriptState();

	_actionColour1 = _actionColour4 = 0;
	_actionColour2 = _actionColour3 = -1;
	_currentActionColour1 = _currentActionColour4 = 0;
	_currentActionColour2 = _currentActionColour3 = 0xffffff00;
	_unavailableActionColour1 = _unavailableActionColour4 = 0;
	_unavailableActionColour2 = _unavailableActionColour3 = 0xff808080;

	resize(g_system->getWidth(), g_system->getHeight());
}

LocationModule::~LocationModule() {
	if (g_system) {
		g_system->lockMouse(false);
	}
	if (_actionScriptState != nullptr && _actionScriptState->_script != nullptr) {
		delete[] _actionScriptState->_script;
		_actionScriptState->_script = nullptr;
	}
	if (_environmentScriptState != nullptr) {
		_environmentScriptState->_script = nullptr;
	}
	if (_queryActionScriptState != nullptr) {
		_queryActionScriptState->_script = nullptr;
	}
	if (_initScriptState != nullptr) {
		_initScriptState->_script = nullptr;
	}
	delete _actionScriptState;
	delete _environmentScriptState;
	delete _queryActionScriptState;
	delete _initScriptState;
	delete _scriptEngine;
}

void LocationModule::initialize() {
	centerMouse();
	g_system->lockMouse(true);
	CursorMan.showMouse(true);
	_ignoreNextMouseMove = true;

	GameController::resetTimers();

	if (ModuleController::_pMap != nullptr) {
		MapData *mapEntry = ModuleController::_pMap->get(_locationId);
		_location._mapEntry = mapEntry;
		if (_scriptEngine != nullptr) {
			_scriptEngine->_mapEntry = mapEntry;
		}

		if (mapEntry != nullptr) {
			Common::String scriptFile = GameController::getFileName(mapEntry->scriptFileIndex);
			uint8 *scriptData = nullptr;
			int scriptLen = 0;

			if (!scriptFile.empty()) {
				Archive scriptArch;
				if (scriptArch.open(Common::Path(scriptFile))) {
					Common::SeekableReadStream *s = scriptArch.getStream(mapEntry->scriptFileEntry);
					if (s) {
						scriptLen = s->size();
						scriptData = new uint8[scriptLen];
						s->read(scriptData, scriptLen);
						delete s;
					}
				}
			}
			AnimationController::clear();
			loadLocation(mapEntry->locationFileIndex, scriptData, scriptLen, scriptFile, mapEntry->scriptFileEntry);
			setLocationPosition(ModuleController::_pMap->getStartupPosition(_locationId, _startupPosition));

			if (_initScriptState != nullptr && _scriptEngine != nullptr && scriptData != nullptr) {
				_initScriptState->init(scriptData, scriptLen, scriptFile, mapEntry->scriptFileEntry);
				_scriptEngine->execute(_initScriptState, GameController::getLocationInitializationScriptId());
			}
		}
	}
}

void LocationModule::render() {
	if (AnimationController::noVideoAnim()) {
		bool waitingForInput = (_initScriptState->_waitingForInput || _actionScriptState->_waitingForInput || _environmentScriptState->_waitingForInput);
		if (_initScriptState->_waitingForInput) {
			_scriptEngine->resume(_initScriptState, true);
		} else if (_actionScriptState->_waitingForInput) {
			_scriptEngine->resume(_actionScriptState, true);
		} else if (_environmentScriptState->_waitingForInput) {
			if (!_environmentScriptState->_waitingForExternalModule) {
				_scriptEngine->resume(_environmentScriptState, true);
				_location.render();
			}
		} else if (_initScriptState->_executionPointer < 0) {
			if (!AnimationController::hasAnim()) {
				_location.animate();
			}
			_location.render();
		}

		if (!waitingForInput && !_environmentScriptState->_waitingForMediaToFinish && !_actionScriptState->_waitingForMediaToFinish && !_initScriptState->_waitingForMediaToFinish) {
			_scriptEngine->execute(_environmentScriptState, GameController::getLocationEnvironmentScriptId());
		} else if (!AnimationController::hasAnim() || AnimationController::isDone()) {
			if (_initScriptState->_waitingForMediaToFinish) {
				_scriptEngine->resume(_initScriptState, true);
			} else if (_actionScriptState->_waitingForMediaToFinish) {
				_scriptEngine->resume(_actionScriptState, true);
			} else if (_environmentScriptState->_waitingForMediaToFinish) {
				_scriptEngine->resume(_environmentScriptState, true);
			}
		}
	}

	if (ModuleController::_currentModule != this) {
		return;
	}

	AnimationController::updateAndRender();
	if (AnimationController::isDone()) {
		if (_actionScriptState->_waitingForMediaToFinish) {
			_scriptEngine->resume(_actionScriptState, true);
		} else {
			AnimationController::clear();
		}
	}

	if (!AnimationController::hasAnim()) {
		CursorMan.showMouse(true);

		float mx = _movement_x;
		float mz = _movement_z;
		float tmz = mz;

		if (mx != 0.0f || mz != 0.0f) {
			mx = _speed * mx;
			mz = _speed * mz;

			float totalmovement = sqrt(mx * mx + mz * mz);
			if (totalmovement > _speed) {
				mx /= (totalmovement / _speed);
				mz /= (totalmovement / _speed);
			}
		}

		_smooth_movement_x = _smooth_movement_x * 0.85f + mx * 0.15f;
		_smooth_movement_z = _smooth_movement_z * 0.85f + mz * 0.15f;

		if (fabs(_smooth_movement_x) < 0.01f && _movement_x == 0.0f) {
			_smooth_movement_x = 0.0f;
		}
		if (fabs(_smooth_movement_z) < 0.01f && _movement_z == 0.0f) {
			_smooth_movement_z = 0.0f;
		}

		int currentItemId = GameController::getCurrentItemId();
		if (_currentAction == ActionType::kUse && currentItemId == -1) {
			_currentAction = ActionType::kNone;
		}

		_location.move(_smooth_movement_x, _movement_y, _smooth_movement_z, tmz);

		if (_location._pointingChanged) {
			int objectId = -1, subObjectId = -1;
			int hitObject = _location.getPickObject(objectId, subObjectId);
			if (hitObject == -1) {
				_currentObjectIndex = -1;
				_currentActions = ActionType::kNone;
				if (_currentAction != ActionType::kUse) {
					_currentAction = ActionType::kNone;
				}
				_currentActionMousePointerIndex = static_cast<int>(CursorType::kCrosshair);
			} else if (_scriptEngine != nullptr && _queryActionScriptState != nullptr) {
				bool resetAction = (hitObject != _currentObjectIndex);
				_currentObjectIndex = hitObject;
				_currentActions = _scriptEngine->getCurrentActions(_queryActionScriptState, _currentObjectIndex);

				if (_currentAction != ActionType::kUse && resetAction) {
					if (_currentActions != ActionType::kNone) {
						_currentAction = ActionType::kLook;
						while (((int)_currentActions & (int)_currentAction) == 0) {
							_currentAction = static_cast<ActionType>((int)_currentAction << 1);
						}
						selectMouseAction();
					} else {
						_currentAction = ActionType::kNone;
						_currentActionMousePointerIndex = static_cast<int>(CursorType::kCrosshair);
					}
				}
			}
		}

		int actions = (int)_currentActions;
		bool useYellow = true;
		if (currentItemId >= 0) {
			actions |= (int)ActionType::kUse;
			useYellow = (_currentObjectIndex >= 0);
		}
		int action = (int)_currentAction;
		int mask = (int)ActionType::kLook;

		float space = (g_system->getWidth() - _actionText[6].getWidth() - 20.0f) / 6.0f;
		for (int i = 0; i < 7; i++) {
			if (mask == action && AnimationController::noAnimOrWave() && useYellow) {
				_actionText[i].setColours(_currentActionColour1, _currentActionColour2, _currentActionColour3, _currentActionColour4);
			} else if ((actions & mask) != 0) {
				_actionText[i].setColours(_actionColour1, _actionColour2, _actionColour3, _actionColour4);
			} else {
				_actionText[i].setColours(_unavailableActionColour1, _unavailableActionColour2, _unavailableActionColour3, _unavailableActionColour4);
			}
			mask <<= 1;

			if (g_engine && g_engine->_renderer) {
				_actionText[i].render(g_engine->_renderer, space * i, 10.0f);
			}
		}

		if (AnimationController::noAnimOrWave()) {
			if (currentItemId >= 0) {
				Item::renderItemImage(currentItemId, (float)(g_system->getWidth() - 98), (float)(g_system->getHeight() - 78), false);
			}

			if (_currentAction == ActionType::kUse && currentItemId >= 0) {
				ModuleController::_cursors[static_cast<int>(CursorType::kCrosshair)].render();
				Item::renderItemImage(currentItemId, (float)(g_system->getWidth() / 2), (float)(g_system->getHeight() / 2), false);
			} else {
				ModuleController::_cursors[_currentActionMousePointerIndex].render();
			}
		}
	} else {
		CursorMan.showMouse(false);
	}
}

void LocationModule::selectMouseAction() {
	switch (_currentAction) {
	case ActionType::kLook:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kLook);
		break;
	case ActionType::kMove:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kMove);
		break;
	case ActionType::kGet:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kGrab);
		break;
	case ActionType::kOnOff:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kOnOff);
		break;
	case ActionType::kTalk:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kTalk);
		break;
	case ActionType::kOpen:
		_currentActionMousePointerIndex = static_cast<int>(CursorType::kOpen);
		break;
	default:
		break;
	}
}

void LocationModule::cycleActions(bool allowUse) {
	int actions = (int)_currentActions;
	if (allowUse && GameController::getCurrentItemId() >= 0) {
		actions |= (int)ActionType::kUse;
		if (_currentAction == ActionType::kNone) {
			_currentAction = static_cast<ActionType>((int)ActionType::kUse >> 1);
		}
	}

	if (actions != 0) {
		_currentAction = static_cast<ActionType>((int)_currentAction << 1);
		while (_currentAction != ActionType::kNone && (actions & (int)_currentAction) == 0) {
			if (_currentAction == ActionType::kUse && allowUse && GameController::getCurrentItemId() >= 0) {
				break;
			}
			_currentAction = static_cast<ActionType>((int)_currentAction << 1);
			if (_currentAction == ActionType::kTerminate) {
				_currentAction = ActionType::kLook;
			}
		}
		selectMouseAction();
	}
}

void LocationModule::loadLocation(int locationFileIndex, uint8 *scriptData, int scriptLength, const Common::String &file, int entry) {
	_movement_x = _movement_y = _movement_z = 0.0f;

	if (_location.load(locationFileIndex)) {
		if (_queryActionScriptState != nullptr) {
			_queryActionScriptState->clear();
			_queryActionScriptState->init(scriptData, scriptLength, file, entry);
			_queryActionScriptState->_debugMode = false;
		}
		if (_actionScriptState != nullptr) {
			_actionScriptState->clear();
			_actionScriptState->init(scriptData, scriptLength, file, entry);
		}
		if (_environmentScriptState != nullptr) {
			_environmentScriptState->clear();
			_environmentScriptState->init(scriptData, scriptLength, file, entry);
			_environmentScriptState->_debugMode = false;
		}

		_oldPoint = Common::Point(g_system->getWidth() / 2, g_system->getHeight() / 2);

		if (GameController::getCurrentItemId() >= 0) {
			_currentActions = static_cast<ActionType>((int)_currentActions | (int)ActionType::kUse);
		}
	}
}

void LocationModule::pause() {
	_movement_forward = 0.0f;
	_movement_backward = 0.0f;
	_movement_left = 0.0f;
	_movement_right = 0.0f;
	_movement_x = 0.0f;
	_movement_y = 0.0f;
	_movement_z = 0.0f;
	_smooth_movement_x = 0.0f;
	_smooth_movement_z = 0.0f;

	g_system->lockMouse(false);
}

void LocationModule::resume() {
	centerMouse();
	g_system->lockMouse(true);
	CursorMan.showMouse(true);
	_ignoreNextMouseMove = true;

	if (_scriptEngine != nullptr && _actionScriptState != nullptr &&
		(_actionScriptState->_waitingForInput || _actionScriptState->_waitingForExternalModule)) {
		_scriptEngine->resume(_actionScriptState, true);
	}
}

void LocationModule::resize(int width, int height) {
	if (g_engine && g_engine->_font) {
		_actionText[0].setText("Look");
		_actionText[1].setText("Move");
		_actionText[2].setText("Get");
		_actionText[3].setText("On/off");
		_actionText[4].setText("Talk");
		_actionText[5].setText("Open");
		_actionText[6].setText("Use");
	}
}

void LocationModule::cursor(float x, float y, bool relative) {
	if (!AnimationController::noAnimOrWave())
		return;

	if (_ignoreNextMouseMove) {
		_ignoreNextMouseMove = false;
		return;
	}

	float centerX = static_cast<float>(g_system->getWidth()) / 2.0f;
	float centerY = static_cast<float>(g_system->getHeight()) / 2.0f;

	float delta_x = relative ? x : (x - centerX);
	float delta_y = relative ? y : (y - centerY);

	// if this EVENT_MOUSEMOVE was just centerMouse() warping to (320, 240), ignore it
	if (delta_x == 0.0f && delta_y == 0.0f) {
		return;
	}

	float scale = 4.0f;
	_location.deltaAngles((delta_y / 2000.0f) * scale, (delta_x / 2000.0f) * scale);

	if (!relative) {
		centerMouse();
	}
}

void LocationModule::beginAction() {
	if (!AnimationController::hasAnim()) {
		if (_currentAction != ActionType::kNone) {
			Caption::clearCaptions(pDisplayCaptions);

			if (_currentObjectIndex >= 0 && _scriptEngine != nullptr) {
				int currentItemId = GameController::getCurrentItemId();
				_scriptEngine->permformAction(_actionScriptState, _currentObjectIndex, _currentAction, _currentAction == ActionType::kUse ? currentItemId : -1);

				if (_currentAction != ActionType::kUse || GameController::getCurrentItemId() != currentItemId) {
					cycleActions(false);
				}
				_location._pointingChanged = true;
			}
		}
	} else {
		AnimationController::skip();
		Caption::clearCaptions(pDisplayCaptions);

		if (_actionScriptState != nullptr && _actionScriptState->_waitingForMediaToFinish && _scriptEngine != nullptr) {
			_scriptEngine->resume(_actionScriptState, true);
		}
	}
}

void LocationModule::back() {
	ModuleBase *mainMenu = ModuleController::get(ModuleType::kMainMenu);
	if (mainMenu != nullptr) {
		ModuleController::sendToFront(mainMenu);
	}
}

void LocationModule::cycle() {
	if (AnimationController::hasAnim()) {
		AnimationController::skip();
		Caption::clearCaptions(pDisplayCaptions);

		if (_actionScriptState != nullptr && _actionScriptState->_waitingForMediaToFinish && _scriptEngine != nullptr) {
			_scriptEngine->resume(_actionScriptState, true);
		}
	} else if (_currentActions != ActionType::kNone || GameController::getCurrentItemId() >= 0) {
		cycleActions(true);
	}
}

void LocationModule::moveForward(float v) {
	_movement_forward = v;
	_movement_z = _movement_backward - _movement_forward;
}

void LocationModule::moveBack(float v) {
	_movement_backward = v;
	_movement_z = _movement_backward - _movement_forward;
}

void LocationModule::moveLeft(float v) {
	_movement_left = v;
	_movement_x = _movement_left - _movement_right;
}

void LocationModule::moveRight(float v) {
	_movement_right = v;
	_movement_x = _movement_left - _movement_right;
}

void LocationModule::moveUp(float y) {
	_movement_y = -y / 10.0f;
}

void LocationModule::moveDown(float y) {
	_movement_y = y / 10.0f;
}

void LocationModule::run(bool isRunning) {
	_speed = isRunning ? MOVEMENT_RUN_SPEED : MOVEMENT_WALK_SPEED;
}

void LocationModule::next() {
	cycleItems(1);
}

void LocationModule::prev() {
	cycleItems(-1);
}

void LocationModule::inventory() {
	// TODO: ModuleController::push(new InventoryModule());
}

void LocationModule::travel() {
	// TODO: ModuleController::push(new UAKMTravelModule());
}

void LocationModule::cycleItems(int direction) {
	if (direction > 0) {
		GameController::selectNextItem();
	} else {
		GameController::selectPreviousItem();
	}

	if (GameController::getCurrentItemId() == -1 && _currentAction == ActionType::kUse) {
		cycleActions(false);
	}
}

void LocationModule::hints() {
	//ModuleController::push(GameController::getHintModule());
}

void LocationModule::mouseMove(Common::Point pt) {
	cursor((float)pt.x, (float)pt.y, false);
}

void LocationModule::mouseMoveRel(Common::Point rel) {
	cursor((float)rel.x, (float)rel.y, true);
}

void LocationModule::mouseDown(Common::Point pt, int btn) {
	if (btn == 1) {
		beginAction();
	} else if (btn == 2) {
		cycle();
	}
}

void LocationModule::mouseWheel(int scroll) {
	if (scroll > 0) {
		next();
	} else if (scroll < 0) {
		prev();
	}
}

void LocationModule::keyDown(uint32 key, uint32 lParam) {
	if (key == Common::KEYCODE_ESCAPE) {
		back();
	} else if (key == Common::KEYCODE_SPACE) {
		if (AnimationController::hasAnim()) {
			cycle();
		} else {
			beginAction();
		}
	} else if (key == Common::KEYCODE_w || key == Common::KEYCODE_UP) {
		moveForward(1.0f);
	} else if (key == Common::KEYCODE_s || key == Common::KEYCODE_DOWN) {
		moveBack(1.0f);
	} else if (key == Common::KEYCODE_a || key == Common::KEYCODE_LEFT) {
		moveLeft(1.0f);
	} else if (key == Common::KEYCODE_d || key == Common::KEYCODE_RIGHT) {
		moveRight(1.0f);
	} else if (key == Common::KEYCODE_z || key == Common::KEYCODE_PAGEUP) {
		moveUp(1.0f);
	} else if (key == Common::KEYCODE_c || key == Common::KEYCODE_PAGEDOWN) {
		moveDown(1.0f);
	} else if (key == Common::KEYCODE_q) {
		prev();
	} else if (key == Common::KEYCODE_e) {
		next();
	} else if (key == Common::KEYCODE_LSHIFT || key == Common::KEYCODE_RSHIFT) {
		run(true);
	}
}

void LocationModule::keyUp(uint32 key, uint32 lParam) {
	if (key == Common::KEYCODE_w || key == Common::KEYCODE_UP) {
		moveForward(0.0f);
	} else if (key == Common::KEYCODE_s || key == Common::KEYCODE_DOWN) {
		moveBack(0.0f);
	} else if (key == Common::KEYCODE_a || key == Common::KEYCODE_LEFT) {
		moveLeft(0.0f);
	} else if (key == Common::KEYCODE_d || key == Common::KEYCODE_RIGHT) {
		moveRight(0.0f);
	} else if (key == Common::KEYCODE_z || key == Common::KEYCODE_PAGEUP ||
			   key == Common::KEYCODE_c || key == Common::KEYCODE_PAGEDOWN) {
		moveUp(0.0f);
	} else if (key == Common::KEYCODE_LSHIFT || key == Common::KEYCODE_RSHIFT) {
		run(false);
	}
}

} // End of namespace Murphy3d
