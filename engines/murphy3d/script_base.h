#ifndef MURPHY3D_SCRIPT_BASE_H
#define MURPHY3D_SCRIPT_BASE_H

#include "murphy3d/enums.h"
#include "murphy3d/location.h"
#include "murphy3d/map.h"
#include "murphy3d/script_state.h"

namespace Murphy3d {

class ScriptBase {
public:
	ScriptBase();
	virtual ~ScriptBase();

	virtual void execute(ScriptState *pState, int id) = 0;
	virtual void resume(ScriptState *pState, bool breakWait = false) = 0;
	virtual ActionType getCurrentActions(ScriptState *pState, int currentObjectIndex) { return ActionType::kNone; }

	virtual void permformAction(ScriptState *pState, int id, ActionType action, int item) = 0;
	virtual void selectDialogueOption(ScriptState *pState, int option) = 0;

	MapData *_mapEntry = nullptr;
	Location *_pLoc = nullptr;
};

} // End of namespace Murphy3d

#endif
