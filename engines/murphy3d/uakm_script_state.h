#ifndef MURPHY3D_UAKM_SCRIPT_STATE_H
#define MURPHY3D_UAKM_SCRIPT_STATE_H

#include "murphy3d/script_state.h"

namespace Murphy3d {

class UAKMScriptState : public ScriptState {
public:
	void init(uint8 *script, int length, const Common::String &file, int entry) override {
		_script = script;
		_length = length;
		_scriptFile = file;
		_scriptEntry = entry;

		_scriptEntries.clear();
		if (script != nullptr) {
			for (int i = 0; i < (length - 2); i++) {
				if (script[i] == 0xe0) {
					int id = ((int)script[i + 1]) | (((int)script[i + 2]) << 8);
					if (!_scriptEntries.contains(id)) {
						_scriptEntries[id] = i + 3;
					}
					i += 2;
				}
			}
		}
	}
};

} // End of namespace Murphy3d

#endif // MURPHY3D_UAKM_SCRIPT_STATE_H
