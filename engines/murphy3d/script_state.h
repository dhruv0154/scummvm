#ifndef MURPHY3D_SCRIPT_STATE_H
#define MURPHY3D_SCRIPT_STATE_H

#include "common/hashmap.h"
#include "common/scummsys.h"
#include "common/str.h"
#include "murphy3d/enums.h"

namespace Murphy3d {

class ScriptState {
public:
	ScriptState();
	virtual ~ScriptState() = default;

	virtual void init(uint8 *script, int length, const Common::String &file, int entry) = 0;
	void clear();

	int getInt(int offset, int size);
	int read8();
	int read8s();
	int read16();
	int read16s();
	int read32();
	float read16_16();
	float read12_4();

	Common::String _scriptFile;
	int _scriptEntry = 0;

	int getScript(int id);

	int _executionPointer = 0;
	bool _waitingForMediaToFinish = false;
	bool _waitingForInput = false;
	bool _waitingForExternalModule = false;
	int _selectedOption = 0;
	int _selectedValue = 0;

	bool _askAbout = false;
	bool _offer = false;
	bool _buy = false;
	bool _askingAboutBuyables = false;

	int _topItemOffset = 0;

	ActionType _allowedAction = ActionType::kNone;
	bool _queryAction = false;
	ActionType _currentAction = ActionType::kNone;

	int _lastDialoguePoint = -1;
	int _frameTrigger = -1;

	uint8 *_script = nullptr;
	int _length = 0;

	bool _debugMode = false;

	InteractionMode _mode = InteractionMode::kNone;

	int _parameter = -1;

protected:
	Common::HashMap<int, int> _scriptEntries;

	inline bool canRead(int bytes) const {
		return (_script != nullptr) && (_executionPointer + bytes <= _length);
	}
};

} // End of namespace Murphy3d

#endif
