#include "murphy3d/script_state.h"

namespace Murphy3d {

ScriptState::ScriptState() {
	_script = nullptr;
	_length = 0;
	_debugMode = false;
	clear();
}

void ScriptState::clear() {
	_executionPointer = 0;
	_waitingForMediaToFinish = false;
	_waitingForInput = false;
	_waitingForExternalModule = false;
	_selectedOption = 0;
	_selectedValue = 0;

	_askAbout = false;
	_offer = false;
	_buy = false;
	_askingAboutBuyables = false;
	_topItemOffset = 0;

	_mode = InteractionMode::kNone;

	_allowedAction = ActionType::kNone;
	_queryAction = false;
	_currentAction = ActionType::kNone;

	_lastDialoguePoint = -1;
	_frameTrigger = -1;

	_parameter = -1;
}

int ScriptState::getScript(int id) {
	if (_scriptEntries.contains(id) && _scriptEntries[id] > 0) {
		return _scriptEntries[id];
	}
	return -1;
}

int ScriptState::getInt(int offset, int size) {
	if (_script == nullptr || offset < 0 || offset + size > _length) {
		return 0;
	}
	int result = 0;
	for (int i = 0; i < size; i++) {
		result |= ((int)_script[offset + i]) << (i * 8);
	}
	return result;
}

int ScriptState::read8() {
	if (!canRead(1))
		return 0;
	return _script[_executionPointer++];
}

int ScriptState::read8s() {
	if (!canRead(1))
		return 0;
	return static_cast<int8>(_script[_executionPointer++]);
}

int ScriptState::read16() {
	if (!canRead(2))
		return 0;
	int ret = getInt(_executionPointer, 2);
	_executionPointer += 2;
	return ret;
}

int ScriptState::read16s() {
	if (!canRead(2))
		return 0;
	int val = getInt(_executionPointer, 2);
	_executionPointer += 2;
	return static_cast<int16>(val);
}

int ScriptState::read32() {
	if (!canRead(4))
		return 0;
	int ret = getInt(_executionPointer, 4);
	_executionPointer += 4;
	return ret;
}

float ScriptState::read16_16() {
	if (!canRead(4))
		return 0.0f;
	int i = getInt(_executionPointer, 4);
	_executionPointer += 4;
	return static_cast<float>(i) / 65536.0f;
}

float ScriptState::read12_4() {
	if (!canRead(2))
		return 0.0f;
	int i = getInt(_executionPointer, 2);
	_executionPointer += 2;
	int16 signedVal = static_cast<int16>(i);
	return static_cast<float>(signedVal) / 16.0f;
}

} // End of namespace Murphy3d
